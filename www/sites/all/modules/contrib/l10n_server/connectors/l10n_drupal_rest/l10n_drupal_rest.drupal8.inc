<?php

/**
 * @file
 *   Drupal 8 status update pages.
 */

function l10n_drupal_rest_drupal8_status() {
  module_load_include('inc', 'l10n_community', 'pages');
  drupal_set_title(t('Drupal 8 translation status'));

  $project = l10n_community_get_highlighted_project();
  $rid = db_query("SELECT pf.rid FROM {l10n_server_release} sr INNER JOIN {l10n_packager_file} pf ON sr.rid = pf.rid WHERE pid = :pid AND sr.title LIKE '8.%' ORDER BY pf.rid DESC LIMIT 0, 1", array(':pid' => $project->pid))->fetchField();
  $release_title = db_query("SELECT title FROM {l10n_server_release} WHERE rid = :rid", array(':rid' => $rid))->fetchField();
  list($num_source, $string_counts) = l10n_community_get_l10n_packager_string_count($project->pid, $rid);

  $output = '<p>' . t("Translation status of Drupal 8 per language based on the @num source strings found in the latest Drupal 8 release (@release). Each language links to the list of untranslated strings in the release for that group. The contributor count is displayed overall in the team, not just contributors specific to this release.", array('@num' => $num_source, '@release' => $release_title)) . '</p>';

  $languages = l10n_community_get_languages();

  $table_rows = array();
  foreach ($languages as $langcode => $language) {
    $stats = l10n_community_get_stats($langcode);
    $columns = theme('l10n_community_progress_columns', array(
      'sum'            => $num_source,
      'translated'     => @$string_counts[$langcode]['translations'],
      'has_suggestion' => @$string_counts[$langcode]['suggestions']
    ));
    $progress = array_shift($columns);
    $table_rows[] = array(
      array(
        'data' => l(t($language->name), 'translate/languages/' . $langcode . '/translate', array('query' => array('project' => 'drupal', 'status' => '1', 'release' => $rid))),
        'sortdata' => t($language->name),
        'class' => array('rowhead'),
      ),
      array(
        'data' => $progress,
        'sortdata' => ($num_source == 0 ? 0 : round(@$string_counts[$langcode]['translations'] / $num_source * 100, 2)),
      ),
      array(
        'data' => $stats['users'],
        'sortdata' => $stats['users'],
      ),
    );
  }

  // Default to sorting by progress descending.
  if (empty($_GET['sort']) && empty($_GET['order'])) {
    $_GET['sort'] = 'desc';
    $_GET['order'] = 'Drupal 8 core progress';
  }

  if (!empty($_GET['sort']) && !empty($_GET['order'])) {
    usort($table_rows, 'l10n_server_sort_by_column_language');
  }

  $header = array(
    array('data' => t('Language'), 'field' => 'language', 'class' => array('rowhead')),
    array('data' => t('Drupal 8 core progress'), 'field' => 'progress'),
    array('data' => t('Contributors'), 'field' => 'contributors'),
  );
  $output .= theme(
    'table',
    array(
      'header' => $header,
      'rows' => $table_rows,
      'attributes' => array('class' => array('l10n-community-overview l10n-community-highlighted'))
  ));
  return $output;
}
