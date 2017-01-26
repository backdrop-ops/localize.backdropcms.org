<?php

/**
 * Syncronize the project list.
 */
function l10n_drupal_rest_refresh_project_list() {
  $connector_name = 'l10n_drupal_rest_restapi';
  $projects = $releases = array();
  $project_count = $release_count = 0;

  // Only sync releases which are at most one day older then our last
  // sync date. This ensures time zone issues and releases published while the
  // previous cron run will not be a problem, but we only look at a relatively
  // small list of releases at any given time. We only sync tagged releases,
  // which will not get rebuilt later anytime.
  $last_sync = variable_get('l10n_drupal_rest_last_sync', 0);
  $before = $last_sync - 60 * 60 * 24;

  // Fetch projects and releases since last sync.
  $file_path = 'temporary://releases.tsv';
  $url = variable_get('l10n_drupal_rest_refresh_url', L10N_DRUPAL_REST_REFRESH_URL);
  // Add a timestamp GET parameter to prevent CDN caching.
  $url = url($url, array('query' => array('time' => time())));
  // This will take some time, so we need to increase timeout.
  $response = drupal_http_request($url, array(), 'GET', NULL, 3, 300);
  if ($response->code == 200) {
    // Save as temporary file and release the memory.
    $file = file_save_data($response->data, $file_path, FILE_EXISTS_REPLACE);
    unset($response);
    _l10n_drupal_rest_read_tsv($file_path, $before, $projects, $releases);
    // Remove file
    file_delete($file);
  }
  else {
    watchdog($connector_name, 'Releases URL %url is unreacheable.', array(
      '%url' => $url,
    ));
    return;
  }

  // Record all non-existing projects in our local database.
  foreach ($projects as $project_name => $project_title) {
    if ($existing_project = db_select('l10n_server_project', 'p')
      ->fields('p')
      ->condition('uri', $project_name)
      ->execute()
      ->fetchAssoc()
    ) {
      // Check that the title is correct
      if ($existing_project['title'] != $project_title) {
        db_update('l10n_server_project')
          ->fields(array('title' => $project_title))
          ->condition('uri', $project_name)
          ->execute();
        watchdog($connector_name, 'Project %n renamed to %t.', array(
          '%t' => $project_title,
          '%n' => $project_name,
        ));
      }
    }
    else {
      $project_count++;
      db_insert('l10n_server_project')->fields(array(
        'uri'              => $project_name,
        'title'            => $project_title,
        'last_parsed'      => REQUEST_TIME,
        'home_link'        => 'http://drupal.org/project/' . $project_name,
        'connector_module' => $connector_name,
        'status'           => 1,
      ))->execute();
      watchdog($connector_name, 'Project %t (%n) added.', array(
        '%t' => $project_title,
        '%n' => $project_name,
      ));
    }
  }

  // Record all releases in our local database.
  foreach ($releases as $release) {
    $download_link = "http://ftp.drupal.org/files/projects/{$release['machine_name']}-{$release['version']}.tar.gz";
    if ($existing_release = db_select('l10n_server_release', 'r')
      ->fields('r')
      ->condition('download_link', $download_link)
      ->execute()
      ->fetchAssoc()
    ) {
      // @TODO What happens to unpublished releases? drop data outright?
    }
    else {
      $release_count++;
      // Get the project pid
      $pid = db_select('l10n_server_project', 'p')
        ->fields('p', array('pid'))
        ->condition('uri', $release['machine_name'])
        ->execute()
        ->fetchField();

      // @TODO What about filehash?
      $filehash = '';
      // New published release, not recorded before.
      db_insert('l10n_server_release')->fields(array(
        'pid'           => $pid,
        'title'         => $release['version'],
        'download_link' => $download_link,
        'file_date'     => $release['created'],
        'file_hash'     => $filehash,
        'last_parsed'   => 0,
        'weight'        => 0,
      ))->execute();
      watchdog($connector_name, 'Release %t from project %n added.', array(
        '%t' => $release['version'],
        '%n' => $release['machine_name'],
      ));
      // Update last sync date with the date of this release if later.
      $last_sync = max($last_sync, $release['created']);
    }
  }

  // Report some informations.
  if ($release_count || $project_count) {
    watchdog($connector_name, 'Fetched info about %p projects and %r releases.',
      array(
        '%p' => $project_count,
        '%r' => $release_count,
      ));
  }
  else {
    watchdog($connector_name, 'No new info about projects and releases.');
  }

  // Set last sync time to limit number of releases to look at next time.
  variable_set('l10n_drupal_rest_last_sync', $last_sync);
}

/**
 * Parse the release file for projects and releases newer than before
 *
 * @param $file_path
 * @param $before
 * @param $projects
 * @param $releases
 * @return mixed
 */
function _l10n_drupal_rest_read_tsv($file_path, $before, &$projects, &$releases) {
  $headers = array();
  if (($handle = fopen($file_path, "r")) !== FALSE) {
    while (($data = fgetcsv($handle, 1000, "\t")) !== FALSE) {
      // Get headers
      if (empty($headers)) {
        $headers = array_flip($data);
        continue;
      }
      // Filter out sandboxes and malformed releases.
      if (count($data) < 5 || is_numeric($data[$headers['project_machine_name']])) {
        continue;
      }
      $time = strtotime($data[$headers['created']]);
      if ($before < $time) {
        $machine_name = trim($data[$headers['project_machine_name']]);
        $title = trim($data[$headers['project_name']]);
        // A first array for projects.
        $projects[$machine_name] = $title;
        // A second array for releases.
        $releases[] = array(
          'created' => $time,
          'machine_name' => $machine_name,
          'title'        => $title,
          'version'      => $data[$headers['version']],
        );
      }
      else {
        fclose($handle);
        return TRUE;
      }
    }
  }
  return FALSE;
}
