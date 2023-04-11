<?php

/**
 * @file
 *   Localization packager module code library.
 */

// This library relies on l10n_community export.inc
module_load_include('inc', 'l10n_community', 'export');

/**
 * Check release translations and repackage if needed.
 *
 * For each release we store packaging data in {l10n_packager_release}
 * - 'checked' is the last time all languages for this release were checked.
 * - 'updated' is the last time a file was updated for this release.
 *
 * We don't updated the 'checked' field untile we've checked all the languages
 * for this release so we can keep track of releases and files and package a
 * few languages on every cron.
 *
 * @param $release
 *   Release object.
 * @param $force
 *   Force repackage even when strings not updated.
 * @param $limit
 *   Maximum number of files to create.
 * @param $language
 *   Optional language object to check only this one.
 * @param $cron
 *   In a cron run, a release may be packaged partially, for some languages.
 */
function l10n_packager_release_check($release, $force = FALSE, $limit = 0, $language = NULL, $cron = FALSE) {
  $check_languages = isset($language) ? array($language->langcode => $language) : l10n_community_get_languages();
  $updated = array();
  // We get update time before creating files so the release checked time
  // is <= file timestamp.
  $timestamp = REQUEST_TIME;

  $files = l10n_packager_release_get_files($release->rid);
  $last_updated = l10n_packager_translation_last_updated($release->rid);

  // Get only the languages we have translations for, that need updating
  $languages = array();
  foreach ($check_languages as $langcode => $language) {
    if (!empty($last_updated[$langcode]) && ($force || empty($files[$langcode]) || ($last_updated[$langcode] > $files[$langcode]->checked))) {
      $languages[$langcode] = $language;
    }
  }

  // For this special case we check we didn't stop before in the middle of a
  // release. Otherwise it could stick on a release forever when forcing.
  if ($force && $cron && $release->checked < $release->updated) {
    foreach ($files as $lang => $file) {
      if (!empty($file->checked) && ($file->checked > $release->checked)) {
        unset($languages[$lang]);
      }
    }
  }

  // Repackage this relase for the remaining language list.
  while ((!$limit || $limit > count($updated)) && ($language = array_shift($languages))) {
    $langcode = $language->langcode;
    // Warning: this may upload release data with or without file.
    $existing = !empty($files[$langcode]) ? $files[$langcode] : NULL;
    $updated[$langcode] = l10n_packager_release_package($release, $language, $existing, $timestamp);
  }

  // Update the release data.
  if (!count($languages)) {
    // We only mark the release checked if there are no languages left.
    $release->checked = $timestamp;
  }
  if ($updated) {
    $release->updated = $timestamp;
  }

  if (isset($release->status)) {
    // Just update status of existing record.
    backdrop_write_record('l10n_packager_release', $release, 'rid');
  }
  else {
    // The first time we checked this release, we need to add a new record.
    $release->status = L10N_PACKAGER_ACTIVE;
    backdrop_write_record('l10n_packager_release', $release);
  }
  return $updated;
}

/**
 * Generate a new file for a given release or update an existing one.
 *
 * @param $release
 *   Release object with uri and rid properties.
 * @param $language
 *   Language object.
 * @param $file
 *   Release file object to be updated.
 * @param $timestamp
 *   Timestamp to mark the files, for it to be consistent across tables.
 *
 * @return
 *   Drupal file object or FALSE on error.
 */
function l10n_packager_release_package($release, $language, $file = NULL, $timestamp = NULL) {

  $timestamp = $timestamp ? $timestamp : REQUEST_TIME;
  $variables = array(
    '%release' => l10n_packager_release_name($release),
    '%language' => $language->name,
  );

  if (!$file) {
    $file = new stdClass();
    $file->rid = $release->rid;
    $file->language = $language->langcode;
  }

  // Generate PO file. Always export in compact form.
  $export_result = l10n_community_export($release->uri, $release->rid, $language, FALSE, TRUE);

  if (!empty($export_result) && is_array($export_result)) {

    // If we got an array back from the export build, tear that into pieces.
    list($mime_type, $export_name, $serve_name, $sid_count) = $export_result;

    // Get the destination file path.
    $file_path = l10n_packager_get_filepath($release, $language);
    // Build the full path and make sure the directory exits.
    $file_path = l10n_packager_create_path($file_path);

    // Now build the Drupal file object or update the old one.
    if (!empty($file->fid) && !empty($file->uri)) {
      file_unmanaged_delete($file->uri);
    }

    // Check / upate / create all file data.
    $file->status = FILE_STATUS_PERMANENT;
    $file->timestamp = $file->checked = $timestamp;
    $file->filename = basename($file_path);
    $file->filemime = $mime_type;
    $file->uri = $file_path;
    file_unmanaged_move($export_name, $file->uri, FILE_EXISTS_REPLACE);
    $file->filesize = filesize(backdrop_realpath($file->uri));
    $file->sid_count = $sid_count;
    // Create actual symlink to latest
    l10n_packager_create_latest_symlink($file_path, $release, $language);

    // Create / update file record and link to release.
    backdrop_write_record('file_managed', $file, !empty($file->fid) ? 'fid' : array());
    backdrop_write_record('l10n_packager_file', $file, !empty($file->drid) ? 'drid' : array());
    module_invoke_all('l10n_packager_done', $file);
    return $file;

  }
  else {
    watchdog('l10n_packager', 'Failed packaging release %release for language %language.', $variables);
    return FALSE;
  }
}

// == Batch functions ==========================================================

/**
 * Create batch for repackaging releases.
 *
 * @param $rid
 *   Release id or array of release ids.
 * @param $languages
 *   Array of language codes to repackage or none.
 *
 * @return
 *   Batch array.
 */
function l10n_packager_release_batch($rid, $languages = NULL) {
  $rids = is_array($rid) ? $rid : array($rid);

  // All languages if no languages passed
  $languages = !empty($languages) ? $languages : array_keys(l10n_community_get_languages());
  foreach ($rids as $rid) {
    foreach ($languages as $langcode) {
      $operations[] = array('_l10n_packager_batch_repackage', array($rid, $langcode));
    }
  }

  if (!empty($operations)) {
    return _l10n_packager_build_batch($operations, t('Repackaging translation files.'));
  }
}

/**
 * Get batch stub.
 */
function _l10n_packager_build_batch($operations = array(), $title = '') {
  $batch = array(
    'title' => $title ? $title : t('Translations packager.'),
    'operations' => $operations,
    // Tell the batch engine which file to load before calling operations.
    'file' => backdrop_get_path('module', 'l10n_packager') . '/l10n_packager.inc',
  );
  return $batch;
}

/**
 * Batch callback to repackage a release.
 *
 * @param $rid
 *   Release id.
 * @param $langcode
 *   Language object to package.
 */
function _l10n_packager_batch_repackage($rid, $langcode) {
  if ($release = l10n_packager_get_release($rid)) {
    $languages = l10n_community_get_languages();
    $language = $languages[$langcode];
    $updates = l10n_packager_release_check($release, TRUE, 0, $language);
    if ($file = current($updates)) {
      backdrop_set_message(t("Repackaged release %release for %language. Created file %filename.", array('%release' => l10n_packager_release_name($release), '%filename' => $file->filename, '%language' => t($language->name))));
    }
  }
}

/**
 * Create a symlink to the latest version of this locale for the project.
 *
 * The symlink name has the pattern [project]-[branch].[langcode].po and will
 * be placed is the same directory as the translation file.
 *
 * @param $file
 *   The translation file to be symlinked.
 * @param $release
 *   Object containing the file's release information.
 * @param $language
 *   Language object.
 *
 * @return bool
 *   Returns TRUE if a symlink was created.
 */
function l10n_packager_create_latest_symlink($file, $release, $language) {
  // If there is a minor version number, remove it. “Branch” is only
  // '{major}.x' or '{compatibility}-{major}.x'. So a new dev branch can fall
  // back to the latest translation for the major branch. For example, 9.1.x,
  // when there are no tagged 9.1.* releases, can get the 9.0.0-beta1
  // translations.
  $abbreviated_release = clone $release;
  $abbreviated_release->branch = preg_replace('/\.[0-9]+\.x$/', '.x', $abbreviated_release->branch);

  $target = backdrop_realpath($file);
  $latest_file = dirname($target) . '/' . l10n_packager_get_filepath($abbreviated_release, $language, '%project-%branch.%language.po');

  if (file_exists($latest_file)) {
    unlink($latest_file);
    $latest_file_object = new stdClass();
    $latest_file_object->uri = l10n_packager_directory() . '/' . l10n_packager_get_filepath($abbreviated_release, $language, '%core/%project/%project-%branch.%language.po');
    // Allow modules to react to the symlink, such as purge a CDN.
    module_invoke_all('l10n_packager_done', $latest_file_object);
  }

  return symlink(basename($target), $latest_file);
}
// == Utility functions ========================================================

/**
 * Get release object with packager data and some project data.
 */
function l10n_packager_get_release($rid) {
  if (is_object($rid)) {
    return $rid;
  }
  else {
    $result = db_query("
SELECT r.rid, r.pid, r.title, pr.checked, pr.updated, pr.status, p.uri
FROM {l10n_server_project} p
INNER JOIN {l10n_server_release} r ON p.pid = r.pid
LEFT JOIN {l10n_packager_release} pr ON r.rid = pr.rid
WHERE r.rid = :rid",
              array(':rid' => $rid));
    $release = $result->fetchObject();
    l10n_packager_release_set_branch($release);
    return $release;
  }
}

/**
 * Get release name.
 */
function l10n_packager_release_name($rid) {
  if ($release = l10n_packager_get_release($rid)) {
    return $release->uri . '-' . $release->title;
  }
  else {
    return '';
  }
}

/**
 * Get timestamp of the last updated string for a release, for each language.
 */
function l10n_packager_translation_last_updated($rid) {
  $result = db_query("SELECT t.language, MAX(t.time_changed) AS latest_time FROM {l10n_server_translation} t INNER JOIN {l10n_server_line} l ON t.sid = l.sid WHERE t.is_active = 1 AND t.is_suggestion = 0 AND l.rid = :rid GROUP BY t.language",
            array(':rid' => $rid));
  return $result->fetchAllKeyed();
}

/**
 * Get files for a release, indexed by language.
 */
function l10n_packager_release_get_files($rid) {
  $result = db_query('SELECT * FROM {l10n_packager_file} r LEFT JOIN {file_managed} f ON r.fid = f.fid WHERE r.rid = :rid',
            array(':rid' => $rid));
  return $result->fetchAllAssoc('language');
}

/**
 * Determine the branch for a release. We should get this into a cross-site
 * packaging module.
 */
function l10n_packager_release_set_branch(&$release) {
  // Set branch to everything before the last dot, and append an x. For
  // example, 6.1, 6.2, 6.x-dev, 6.0-beta1 all become 6.x. 8.7.13 becomes
  // 8.7.x. 6.x-1.0-beta1 becomes 6.x-1.x. 2.1.0-rc1 becomes 2.1.x.
  $release->branch = preg_replace('#\.[^.]*$#', '.x', $release->title);

  // Stupid hack for backdrop core.
  if ($release->uri === 'backdrop') {
    // Version has -extra removed, 6.0-beta1 becomes 6.0.
    $release->version = explode('-', $release->title)[0];
    // Major version is the first component before the .
    $major = explode('.', $release->branch)[0];
    if ($major >= 8) {
      // In D8 & later, start removing “API compatibility” part of the path.
      $release->core = 'all';
    }
    else {
      $release->core = $major . '.x';
    }
  }
  else {
    // Modules are like: 6.x-1.0, 6.x-1.x-dev, 6.x-1.0-beta1, 2.0.0, 5.x-dev,
    // 2.1.x-dev, 2.1.0-rc1. If there is a core API compatibility component,
    // split it off. version here is the main version number, without the
    // -{extra} component, like -beta1 or -rc1.
    preg_match('#^(?:(?<core>(?:4\.0|4\.1|4\.2|4\.3|4\.4|4\.5|4\.6|4\.7|5|6|7|8|9)\.x)-)?(?<version>[0-9.x]*)(?:-.*)?$#', $release->title, $match);
    $release->core = $match['core'] ?: 'all';
    $release->version = $match['version'];
  }
}
