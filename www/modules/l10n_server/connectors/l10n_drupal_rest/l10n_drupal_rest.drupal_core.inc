<?php

/**
 * @file
 *   Drupal core status update page.
 */

function l10n_drupal_rest_drupal_core_status() {
  module_load_include('inc', 'l10n_community', 'pages');
  drupal_set_title(t('Drupal core translation status'));

  $project_id = db_query_range("SELECT pid FROM {l10n_server_project} WHERE title = 'Drupal core'", 0, 1)->fetchField();
  $releases = db_query('SELECT sr.rid, sr.title FROM {l10n_server_release} sr WHERE sr.pid = :pid', [
    ':pid' => $project_id,
  ])->fetchAllKeyed();

  // Filter out alpha and beta versions. Keep RC versions.
  foreach ($releases as $rid => $title) {
    if (stripos($title, 'alpha') || stripos($title, 'beta')) {
      unset($releases[$rid]);
    }
  }
  // Order by titles using version comparison.
  uasort($releases, 'version_compare');
  // Take the release id of the highest version number. May be an RC.
  $rid = array_reverse(array_keys($releases))[0];

  list($num_source, $string_counts) = l10n_community_get_l10n_packager_string_count($project_id, $rid);

  $output = '<p>' . t('Status based on @num source strings found in Drupal @release. Links are to untranslated strings in the release for that group.', [
    '@num' => number_format($num_source),
    '@release' => $releases[$rid],
  ]) . '</p>';

  $languages = l10n_community_get_languages();

  $table_rows = [];
  foreach ($languages as $langcode => $language) {
    $stats = l10n_community_get_stats($langcode);
    $columns = theme('l10n_community_progress_columns', [
      'sum' => $num_source,
      'translated' => isset($string_counts[$langcode]['translations']) ? $string_counts[$langcode]['translations'] : 0,
      'has_suggestion' => isset($string_counts[$langcode]['suggestions']) ? $string_counts[$langcode]['suggestions'] : 0,
    ]);
    $progress = array_shift($columns);
    $table_rows[] = [
      [
        'data' => l(t($language->name), 'translate/languages/' . $langcode . '/translate', ['query' => ['project' => 'drupal', 'status' => '1', 'release' => $rid]]),
        'sortdata' => t($language->name),
        'class' => ['rowhead'],
      ],
      [
        'data' => $progress,
        'sortdata' => ($num_source == 0 ? 0 : round((isset($string_counts[$langcode]['translations']) ? $string_counts[$langcode]['translations'] : 0) / $num_source * 100, 2)),
      ],
      [
        'data' => $stats['users'],
        'sortdata' => $stats['users'],
      ],
    ];
  }

  // Default to sorting by progress descending.
  if (empty($_GET['sort']) && empty($_GET['order'])) {
    $_GET['sort'] = 'desc';
    $_GET['order'] = t('Drupal @release core progress', ['@release' => $releases[$rid]]);
  }

  if (!empty($_GET['sort']) && !empty($_GET['order'])) {
    usort($table_rows, 'l10n_server_sort_by_column_language');
  }

  $header = [
    ['data' => t('Language'), 'field' => 'language', 'class' => ['rowhead']],
    ['data' => t('Drupal @release core progress', ['@release' => $releases[$rid]]), 'field' => 'progress'],
    ['data' => t('Contributors'), 'field' => 'contributors'],
  ];
  $output .= theme(
    'table',
    [
      'header' => $header,
      'rows' => $table_rows,
      'attributes' => ['class' => ['l10n-community-overview l10n-community-highlighted']]
  ]);
  return $output;
}
