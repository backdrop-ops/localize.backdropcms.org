<?php

/**
 * @file
 * This scripts takes care of the configuration of the l10n_server. It adds
 * a language, a translation project, a release for that project and finally
 * imports source strings and translations of the added release.
 *
 * This script requires the context of drush-script.
 */

// Add german language.
locale_add_language('de');
db_update('languages')
  ->fields(array('plurals' => 2))
  ->condition('language',  'de')
  ->execute();
drush_log('Enabled language german.', 'success');

// Add the Simplenews project.
$project = (object) array(
  'uri' => 'simplenews',
  'title' => 'Simplenews',
  'home_link' => 'http://wwww.drupal.org/project/simplenews',
  'connector_module' => 'l10n_gettext_uploads',
  'weight' => 0,
  'status' => 1,
);

if (drupal_write_record('l10n_server_project', $project)) {
  drush_log('Added translation project Simplenews', 'success');
}
else {
  drush_set_error('New project Simplenews was not added.');
}

// Add the 7.x-1.0 translation release.
$release = (object) array(
  'pid' => 1,
  'title' => '7.x-1.0',
  'download_link' => 'http://ftp.drupal.org/files/translations/7.x/simplenews/simplenews-7.x-1.0.de.po',
  'weight' => 0,
);
if (drupal_write_record('l10n_server_release', $release)) {
  drush_log('Succesfully added translation release simplenews 7.x-1.0', 'success');
}
else {
  drush_set_error('Translation release for Simplenews was not added.');
}

// Save file info and mark the file parsed.
$fid = db_insert('l10n_server_file')
  ->fields(array(
    'pid' => 1,
    'rid' => 1,
    'location' => 'simplenews-7.x-1.0.de.po',
    'revision' => '',
  ))
  ->execute();

// Begin building file object.
$file = new stdClass();
$file->uid      = 1;
$file->status   = 0;
$file->filename = 'simplenews-7.x-1.0.de.po';
$file->uri      = 'public://simplenews-7.x-1.0.de.po';
$file->filemime = file_get_mimetype($file->filename);

// Do the actual parsing on the local file for source strings.
if (l10n_server_parse_po($file, 'l10n_gettext_store_string', array(1, 1, 1))) {
  $sid_count = l10n_gettext_store_string();
  drush_log(format_plural($sid_count, 'The source file was successfully imported. One source string was added.', 'The source file was successfully imported. @count source strings were added.'), 'success');
  // Update the DB with the count of the imported source strings.
  db_update('l10n_server_release')
    ->fields(array('sid_count' => $sid_count))
    ->fields(array('last_parsed' => REQUEST_TIME))
    ->condition('rid', 1)
    ->execute();
}
else {
  drush_set_error('Source strings were not imported.');
}

module_load_include('inc', 'l10n_community', 'import');
// Do the actual parsing on the local file.
if (l10n_server_parse_po($file, '_l10n_community_import_one_string', array('de', 0, 1))) {
  drush_log(t('The translation was successfully imported.'), 'success');
  l10n_community_update_message();
  cache_clear_all('l10n:stats:' . $form_state['values']['langcode'], 'cache');
}
else {
  drush_set_error('Translations strings were not imported.');
}

