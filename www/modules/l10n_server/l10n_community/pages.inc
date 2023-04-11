<?php

/**
 * @file
 *   Overview pages for localization community.
 */

// = Overview screens ==========================================================

/**
 * Translation status overview for all languages.
 */
function l10n_community_explore_languages() {

  // Checking whether we have languages and strings.
  if (!$languages = l10n_community_get_languages()) {
    backdrop_set_message(t('No languages to list.'), 'error');
    return '';
  }
  if (!$num_source = l10n_community_get_string_count('all')) {
    backdrop_set_message(t('No strings to translate.'), 'error');
    return '';
  }

  // Generate listing of all languages with summaries. The list of languages
  // is relatively "short", compared to projects, so we don't need a pager here.
  $table_rows = array();
  $string_counts = l10n_community_get_string_count('languages');
  foreach ($languages as $langcode => $language) {
    $all_language_plurals = state_get('locale_translation_plurals', array());
    $language_plurals = !empty($all_language_plurals[$langcode]['plurals']) ? $all_language_plurals[$langcode]['plurals'] : null; 
    if (empty($language_plurals)) {
      $table_rows[] = array(
        array(
          'data' => t($language->name),
          'sortdata' => t($language->name),
          'class' => array('rowhead'),
        ),
        array(
          'data' => t('Uninitialized plural formula. Please set up the plural formula in <a href="@language-config">the langauge configuration</a> or alternatively <a href="@import-url">import a valid interface translation</a> for Backdrop in this language.', array(
            '@import-url' => url('admin/structure/translate/import'),
            '@language-config' => url('admin/config/language'),
          )),
          'class' => array('error'),
        ),
        array('data' => ''),
      );
    }
    else {
      $stats = l10n_community_get_stats($langcode);
      $columns = theme(
        'l10n_community_progress_columns',
        array(
          'sum' => $num_source,
          'translated' => @$string_counts[$langcode]['translations'],
          'has_suggestion' => @$string_counts[$langcode]['suggestions'],
        )
      );
      $progress = array_shift($columns);
      $table_rows[] = array(
        array(
          'data' => l(t($language->name), 'translate/languages/' . $langcode),
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
  }

  if (!empty($_GET['sort']) && !empty($_GET['order'])) {
    usort($table_rows, 'l10n_server_sort_by_column_language');
  }

  $header = array(
    array(
      'data' => t('Language'),
      'class' => array('rowhead'),
      'field' => 'language',
    ),
    array('data' => t('Overall progress'), 'field' => 'progress'),
    array('data' => t('Contributors'), 'field' => 'contributors'),
  );
  return theme(
    'table',
    array(
      'header' => $header,
      'rows' => $table_rows,
      'attributes' => array('class' => array('l10n-community-overview l10n-community-highlighted')),
    ));
}

/**
 * Order listing table by column for language overview columns.
 *
 * @param $a
 * @param $b
 * @return int
 */
function l10n_server_sort_by_column_language($a, $b) {
  $sortkey = ($_GET['order'] == t('Language') ? 0 : ($_GET['order'] == t('Contributors') ? 2 : 1));
  if (@$a[$sortkey]['sortdata'] == @$b[$sortkey]['sortdata']) {
    return 0;
  }
  return ((@$a[$sortkey]['sortdata'] < @$b[$sortkey]['sortdata']) ? -1 : 1) * ($_GET['sort'] == 'asc' ? 1 : -1);
}

/**
 * Translation status overview for all projects.
 *
 * Because projects are much more numerous then languages, we need
 * a pager on this screen.
 */
function l10n_community_explore_projects() {
  $output = array();
  $config = config('l10n_community.settings');

  // Kill switch in case we don't want statistics at all.
  if (!$config->get('l10n_community_stats_enabled')) {
    return t('Statistics for projects have been disabled.');
  }

  // Initialize our $initial letter.
  if (isset($_GET['initial'])) {
    $initial = strtolower($_GET['initial']);
  }
  else {
    $initials = array_keys(l10n_server_get_project_initials());
    $initial = strtolower(array_shift($initials));
  }

  // Make a first query tp get the total number of projects.
  $pager_setting = $config->get('l10n_community_project_per_page');
  $options['initial'] = $initial;
  if (!$all_projects = l10n_server_get_projects($options)) {
    backdrop_set_message(t('No projects found.'), 'error');
    return '';
  }
  $options['pager'] = $pager_setting;
  $projects = l10n_server_get_projects($options);

  // If we have more projects compared to what we show at the moment, display
  // an autocomplete jumpbox to jump to a project.
  if (count($all_projects) > $pager_setting) {
    $output[] = backdrop_get_form('l10n_community_explore_projects_jump_form');
  }

  // Fetch all the pids, that we already have.
  $pids = array();
  foreach ($projects as $project) {
    $pids [] = $project->pid;
  }

  if ($config->get('l10n_community_stats_explore_projects_enabled')) {
    // General community statistics.
    $string_counts = array();
    $query = db_select('l10n_server_line', 'l')
      ->fields('l', array('pid'))
      ->groupBy('pid');
    $query->addExpression('COUNT(DISTINCT sid)', 'string_count');
    $query->condition('pid', $pids);
    $query->extend('PagerDefault')->limit($pager_setting);
    $result = $query->execute();
    foreach ($result as $row) {
      $string_counts[$row->pid] = array('count' => $row->string_count);
    }

    // Get the count of distinct strings having translations and suggestions
    // per project. This is run per project because big installs of the
    // module were choking on GROUP BY based solutions.
    foreach (array_keys($string_counts) as $pid) {
      $query = db_select('l10n_server_line', 'l');
      $query->condition('l.pid', $pid);
      $query->leftJoin('l10n_server_status_flag', 'tst', 'l.sid = tst.sid AND tst.has_translation = 1');
      $query->leftJoin('l10n_server_status_flag', 'tss', 'l.sid = tss.sid AND tst.has_suggestion = 1');
      $query->addExpression('COUNT(DISTINCT tst.sid)', 'translations');
      $query->addExpression('COUNT(DISTINCT tss.sid)', 'suggestions');
      $string_counts[$pid] += $query->execute()->fetchAssoc();
    }
  }

  $output[] = _l10n_community_alphabetic_pager($initial);
  $output[] = array('#theme' => 'pager');

  $languages = l10n_community_get_languages('name');
  $language_count = count($languages);

  $rows = array();
  foreach ($projects as $project) {
    $row = array();
    $row[] = array(
      'data' => l($project->title, 'translate/projects/' . $project->uri),
      'class' => array('rowhead'),
    );
    if ($config->get('l10n_community_stats_explore_projects_enabled')) {
      // Multiply summary count by languages handled, so we get an
      // accurate picture of completeness.
      $row = array_merge($row, theme('l10n_community_progress_columns', array(
        'sum' => (@$string_counts[$project->pid]['count'] * $language_count),
        'translated' => @$string_counts[$project->pid]['translations'],
        'has_suggestion' => @$string_counts[$project->pid]['suggestions'],
      )));
    }
    $rows[] = $row;
  }

  $header = array(array('data' => t('Project'), 'class' => array('rowhead')));
  if ($config->get('l10n_community_stats_explore_projects_enabled')) {
    $header = array_merge($header, array(
      t('Progress'),
      t('Untranslated'),
      t('Suggestions'),
    ));
  }
  $output[] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#attributes' => array('class' => array('l10n-community-overview')),
  );
  $output[] = array('#theme' => 'pager');
  return $output;
}

/**
 * Provide a form to jump to a specific project from the project listing.
 */
function l10n_community_explore_projects_jump_form() {
  $form = array(
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );
  $form['project'] = array(
    '#title' => t('Pick a project to jump to'),
    '#type' => 'textfield',
    '#autocomplete_path' => 'translate/project-autocomplete',
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Go'),
  );
  return $form;
}

/**
 * Form submission handler for project jumpform.
 */
function l10n_community_explore_projects_jump_form_submit($form, &$form_state) {
  if (!empty($form_state['values']['project']) && ($uri = l10n_community_project_uri_by_title($form_state['values']['project']))) {
    backdrop_goto('translate/projects/' . $uri);
  }
}

/**
 * Translation status page of all projects from a given language.
 *
 * @param $langcode
 *   Language code, for example 'hu', 'pt-br', 'de', 'it'.
 * @return array
 */
function l10n_community_overview_language($langcode = NULL) {
  if (!isset($langcode)) {
    backdrop_set_message(t('No language selected.'), 'error');
    return '';
  }

  $languages = l10n_community_get_languages();
  $content_output = array();
  // Append all modules implementing hook_l10n_community_build_language_page().
  foreach (module_implements('l10n_community_build_language_page') as $module) {
    $content_output[] = module_invoke($module, 'l10n_community_build_language_page', $languages[$langcode]);
  }
  if (empty($content_output)) {
    // Try to fill in with taxonomy.
    if (module_exists('taxonomy')) {
      if ($terms = taxonomy_term_load_multiple_by_name('l10n-server-' . $languages[$langcode]->langcode)) {
        // Found tag, look for posts.
        $result = taxonomy_select_nodes(reset($terms)->tid);
        foreach ($result as $nid) {
          $content_output[] = node_view(node_load($nid), 'teaser');
        }
      }
      if (empty($content_output)) {
        // Either no such tag, or that tag has no posts currently.
        $content_output[] = array(
          '#markup' => '<p>' . t('There are currently no posts set for display here. Mark posts to display here with the %tagname tag, using the taxonomy module.', array('%tagname' => 'l10n-server-' . $languages[$langcode]->langcode)) . '</p>',
        );
      }
    }
    else {
      // No way found to fill in content here.
      $content_output[] = array(
        '#markup' => '<p>' . t('No posts to display') . '</p>',
      );
    }
  }

  // Set title and breadcrumb. Do this late since the OG code will attempt to
  // set a title and a breadcrumb that is not fitting for us.
  backdrop_set_title(t('@language overview', array('@language' => t($languages[$langcode]->name))), PASS_THROUGH);
  backdrop_set_breadcrumb(
    array(
      l(t('Home'), NULL),
      l(t('Translate'), 'translate'),
      l(t('Explore languages'), 'translate/languages'),
    )
  );
  return $content_output;
}

/**
 * Translation status page of all languages for a given project.
 *
 * @param $uri string Project URI.
 * @return null|string
 */
function l10n_community_overview_project($uri = NULL) {

  // Add missing breadcrumb.
  backdrop_set_breadcrumb(
    array(
      l(t('Home'), NULL),
      l(t('Translate'), 'translate'),
      l(t('Explore projects'), 'translate/projects'),
    )
  );

  if (!isset($uri)) {
    backdrop_set_message(t('No project selected.'), 'error');
    return '';
  }
  backdrop_set_title(t('@project project translations', array('@project' => backdrop_get_title())), PASS_THROUGH);

  if (!$languages = l10n_community_get_languages()) {
    backdrop_set_message(t('No languages to list.'), 'error');
    return '';
  }

  $project = l10n_server_get_projects(array('uri' => $uri));
  if (!config_get('l10n_community.settings', 'l10n_community_stats_enabled')) {
    return t('Statistics for projects have been disabled.');
  }
  elseif (!$num_source = l10n_community_get_string_count('project', $project->pid)) {
    backdrop_set_message(t('No strings found to translate in this project yet.'), 'warning');
    return '';
  }

  $block = l10n_community_language_progress_for_project($project, $languages, t('Translations overview'), t('Overall status of translations'));
  return $block['content'];
}

/**
 * Generate block data about project stats.
 *
 * @param $project
 * @return array
 * @throws \Exception
 */
function l10n_community_block_stats_project($project) {
  if (!empty($project)) {
    $num_source = l10n_community_get_string_count('project', $project->pid);
    $releases = l10n_server_get_releases($project->uri, FALSE);

    $num_parsed = 0;
    foreach ($releases as $release) {
      if ($release->last_parsed > 0) {
        $num_parsed++;
      }
    }
    $num_releases = count($releases);

    $num_warnings = db_query("SELECT COUNT(DISTINCT e.eid) FROM {l10n_server_project} p LEFT JOIN {l10n_server_release} r ON p.pid = r.pid LEFT JOIN {l10n_server_error} e ON r.rid = e.rid WHERE p.uri = :uri", array(':uri' => $project->uri))->fetchField();

    // Build list of links for summary.
    $list = array();
    if (!empty($project->home_link)) {
      $options = array(
        'absolute' => TRUE,
        'external' => TRUE,
      );
      $list[] = t('Project home') . ': ' . l(check_plain($project->home_link), check_url($project->home_link), $options);
    }
    if ($num_releases == 0) {
      // If we don't know of any releases, we do not list source string or warning
      // information, since we should have all zeros there too. This summarizes
      // our stand in short, that we do not yet have data here.
      $list[] = t('No releases known yet');
    }
    else {
      $list[] = l(format_plural($num_parsed, '1 release parsed', '@count releases parsed') . ' (' . format_plural($num_releases, '1 known', '@count known') . ')', 'translate/projects/' . $project->uri . '/releases');
      $list[] = ($num_source == 0 ? t('No source strings found') : format_plural($num_source, '1 source string in total', '@count source strings in total'));
      $list[] = ($num_warnings == 0 ? t('No source code warnings') : format_plural($num_warnings, '1 source code warning', '@count source code warnings'));
    }

    return array(
      'subject' => t('Project statistics'),
      'content' => theme('item_list', array('items' => $list)),
    );
  }
}

/**
 * Generate block data about language stats.
 *
 * @param $language string
 * @return array of 'subject' and 'content'
 * @throws \Exception
 */
function l10n_community_block_stats_language($language) {
  // Provide a summary of the activity.
  $stats_numbers = l10n_community_get_stats($language->langcode);
  return array(
    'subject' => t('Translation statistics'),
    'content' => theme('item_list', array(
      'items' => array(
        format_plural($stats_numbers['users'], '1 contributor', '@count contributors'),
        format_plural($stats_numbers['strings'], '1 string to translate', '@count strings to translate'),
        format_plural($stats_numbers['translations'], '1 translation recorded', '@count translations recorded'),
        format_plural($stats_numbers['suggestions'], '1 suggestion awaiting approval', '@count suggestions awaiting approval'),
      ),
    )),
  );
}

/**
 * Generate block data about language stats.
 *
 * @param $language
 * @return array
 * @throws \Exception
 */
function l10n_community_block_contributors_language($language) {
  // Provide a summary of top contributors.
  $people = l10n_community_get_string_count('top-people', $language->langcode);
  $block = array(
    'subject' => t('Top contributors'),
  );
  $list = array();
  foreach ($people as $translator) {
    // $translator can be passed to theme('username'), since it has 'name' and 'uid'.
    $list[] = array(
      theme('username', array('account' => $translator)),
      $translator->sum,
    );
  }
  if ($list) {
    $block['content'] = theme('table', array(
      'header' => array(
        t('Name'),
        t('Translation count'),
      ),
      'rows' => $list,
    ));
  }
  else {
    $block['content'] = '<p>' . t('Nobody contributed to this translation yet.') . '</p>';
  }
  return $block;
}

/**
 * Reusable renderer for language status per project.
 *
 * @param $project
 * @param $languages
 * @param $title
 * @param $description
 * @return array
 * @throws \Exception
 */
function l10n_community_language_progress_for_project($project, $languages, $title, $description) {
  $block = array(
    'title' => $title,
    'description' => $description,
  );
  // Check for l10n_packager, for faster queries but only on latest release.
  $remove_suggestions = FALSE;
  if (module_exists('l10n_packager') && config_get('l10n_community.settings', 'l10n_community_package_count')) {
    list($num_source, $string_counts) = l10n_community_get_l10n_packager_string_count($project->pid);
    $remove_suggestions = TRUE;
  }
  else {
    $num_source = l10n_community_get_string_count('project', $project->pid);
    $string_counts = l10n_community_get_string_count('languages', $project->pid);
  }

  $table = array();
  $locale_plurals = state_get('locale_translation_plurals', array());

  foreach ($languages as $langcode => $language) {
    if (empty($locale_plurals[$language->langcode]['plurals'])) {
      $table[] = array(
        array(
          'data' => t($language->name) . ' (' . $langcode . ')',
          'class' => array('rowhead'),
        ),
        array(
          'data' => t('Uninitialized plural formula. Please set up the plural formula in <a href="@language-config">the langauge configuration</a> or alternatively <a href="@import-url">import a valid interface translation</a> for Backdrop in this language.', array(
            '@import-url' => url('admin/structure/translate/import'),
            '@language-config' => url('admin/config/language'),
          )),
          'colspan' => '3',
          'class' => array('error'),
        ),
      );
    }
    else {
      $progress = theme('l10n_community_progress_columns', array(
          'sum' => $num_source,
          'translated' => @$string_counts[$langcode]['translations'],
          'has_suggestion' => @$string_counts[$langcode]['suggestions'],
        )
      );
      if ($remove_suggestions) {
        array_pop($progress);
      }
      $table[] = array_merge(
        array(
          array(
            'data' => l(t($language->name) . ' (' . $langcode . ')', 'translate/languages/' . $langcode . '/translate', array('query' => array('project' => $project->uri))),
            'class' => array('rowhead'),
          ),
        ),
        $progress
      );
    }
  }
  $headers = theme('l10n_community_progress_headers', array('mainhead' => t('Language')));
  if ($remove_suggestions) {
    array_pop($headers);
  }
  $block['content'] = theme(
    'table',
    array(
      'header' => $headers,
      'rows' => $table,
      'attributes' => array('class' => array('l10n-community-overview')),
    ));
  $block['content'] .= '<div class="clear-block"></div>';

  return $block;
}

/**
 * Displays a page with a list of releases for a given project.
 *
 * @param $uri string Project URI.
 * @return string
 */
function l10n_community_project_releases($uri = NULL) {
  if (!$uri) {
    backdrop_set_message(t('No project selected.'), 'error');
    return '';
  }
  $project = l10n_server_get_projects(array('uri' => $uri));
  $project_name = $project->title;
  backdrop_set_title(t('@project releases', array('@project' => $project_name)), PASS_THROUGH);

  $releases = l10n_server_get_releases($uri);
  if (empty($releases)) {
    backdrop_set_message(t('No releases found for this project yet.'), 'warning');
    return '';
  }

  return l10n_community_project_releases_table($releases, $uri, $project_name);
}

/**
 * Display metadata based on a list of releases.
 *
 * @param $releases
 * @param $project_uri
 * @param $project_name
 * @param bool $link_releases
 * @return string
 * @throws \Exception
 */
function l10n_community_project_releases_table($releases, $project_uri, $project_name, $link_releases = TRUE) {
  $rows = array();
  foreach ($releases as $release) {
    $file_count = db_query('SELECT COUNT(*) FROM {l10n_server_file} WHERE rid = :rid', array(':rid' => $release->rid))->fetchField();
    $error_count = db_query('SELECT COUNT(*) FROM {l10n_server_error} WHERE rid = :rid', array(':rid' => $release->rid))->fetchField();
    $release_title = $project_name . ' ' . $release->title;
    $rows[] = array(
      $link_releases ? l($release_title, 'translate/projects/' . $project_uri . '/releases/' . $release->rid) : $release_title,
      empty($release->download_link) ? t('n/a') : l(basename($release->download_link), $release->download_link),
      empty($release->file_date) ? t('n/a') : format_date($release->file_date),
      empty($release->last_parsed) ? t('in queue') : format_date($release->last_parsed),
      $file_count ? l($file_count, 'translate/projects/' . $project_uri . '/releases/' . $release->rid, array('fragment' => 'parsed-files')) : $file_count,
      $release->sid_count,
      $error_count ? l($error_count, 'translate/projects/' . $project_uri . '/releases/' . $release->rid, array('fragment' => 'source-warnings')) : $error_count,
    );
  }
  $header = array(
    t('Release'),
    t('Download'),
    t('File date'),
    t('Last parsed'),
    t('Files'),
    t('Strings'),
    t('Warnings'),
  );
  return theme('table', array('header' => $header, 'rows' => $rows));
}

/**
 * Displays a page with detailed information about a release.
 *
 * @param $uri string Project URI.
 * @param $rid int|null Release identifier.
 * @return string
 */
function l10n_community_project_release($uri = NULL, $rid = NULL) {
  if (!isset($uri) || !isset($rid)) {
    backdrop_set_message(t('No project or release selected.'), 'error');
    return '';
  }
  $project = l10n_server_get_projects(array('uri' => $uri));
  $releases = l10n_server_get_releases($uri);
  if (empty($releases) || !isset($releases[$rid])) {
    backdrop_set_message(t('The requested release could not be found.'), 'error');
    return '';
  }
  $release = $releases[$rid];
  backdrop_set_title(t('@project @release information', array(
    '@project' => $project->title,
    '@release' => $release->title,
  )), PASS_THROUGH);

  // Reproduce the basic release information from the parent page.
  $output = '<h3>' . t('Basic release information') . '</h3>';
  $output .= l10n_community_project_releases_table(array($release), $uri, $project->title, FALSE);

  // List of release parse warnings we found.
  $output .= '<h3 id="source-warnings">' . t('Source code warnings identified') . '</h3>';
  $output .= '<p>' . t('Some of these warnings may indicate issues in how certain APIs are used, while others are false positives of legitimate use of the APIs in a non-standard way.') . '</p>';
  $result = db_query("SELECT * FROM {l10n_server_error} WHERE rid = :rid", array(':rid' => $release->rid));
  $rows = array();
  foreach ($result as $warning) {
    $rows[] = array($warning->value);
  }
  $headers = array(t('Warning value'));
  if (count($rows)) {
    $output .= theme('table', array('header' => $headers, 'rows' => $rows));
  }
  else {
    $output .= '<p>' . t('No source code warnings identified in this release.') . '</p>';
  }

  // List the files that we looked at.
  $output .= '<h3 id="parsed-files">' . t('Files parsed in this release') . '</h3>';
  $result = db_query('SELECT * FROM {l10n_server_file} WHERE rid = :rid', array(':rid' => $release->rid));
  $rows = array();
  foreach ($result as $file) {
    // Insert word break hints for the browser to avoid breaking the page layout
    // for long file names and paths.
    $rows[] = array(
      str_replace(array('/', '_'), array(
        '/<wbr />',
        '_<wbr />',
      ), $file->location),
    );
  }
  $headers = array(t('File location'));
  if (count($rows)) {
    $output .= theme('table', array('header' => $headers, 'rows' => $rows));
  }
  else {
    $output .= '<p>' . t('No files found to parse in this release.') . '</p>';
  }

  return $output;
}


// = Theme functions ===========================================================

/**
 * Progress bar and table column display for translation status information.
 *
 * @param $variables array An array containg the keys :
 *  - $sum: The number of all translatable strings to count with.
 *  - $translated: Number of strings translated (and without outstanding
 *   suggestions) out of the $sum.
 *  - $has_suggestion: Number of strings which are not translated yet (or are
 *   translated) and have at least one suggestion attached.
 * @return array
 *   An indexed array with four elements, first being the progress bar,
 *   and the other three being calculated and percentages.
 */
function theme_l10n_community_progress_columns($variables) {
  $sum = $variables['sum'];
  $translated = $variables['translated'];
  $has_suggestion = $variables['has_suggestion'];
  if ($sum == 0) {
    // We handle a project without any source strings available for translation.
    return array(
      array('data' => t('No data available yet.'), 'colspan' => 3),
    );
  }

  // Compute numbers, percentages and provide alternate text titles.
  $done_percent = round($translated / $sum * 100, 2);
  $alt_text = format_plural($sum - $translated, '!percent translated (@count string to go)', '!percent translated (@count strings to go)');
  $status = array(
    'translated' => array(
      (int) $translated,
      $done_percent,
      $alt_text,
    ),
    'untranslated' => array(
      $sum - $translated,
      100 - $done_percent,
      $alt_text,
    ),
    // suggestions are not in the bar as they overlap with both translated and
    // untranslated strings, so we cannot display them here.
  );

  // Visual summary with a progress bar.
  $bar = '<div class="l10n-community-progress">';
  foreach ($status as $key => $values) {
    if ($values[1] > 0) {
      $bar .= '<div class="l10n-community-progress-' . $key . '" style="width:' . $values[1] . '%;" title="' . strtr($values[2], array('!percent' => $done_percent . '%')) . '"></div>';
    }
  }
  $bar .= '</div>';

  return array(
    $bar,
    max($sum - $translated, 0),
    (int) $has_suggestion,
    //t('@num untranslated', array('@num' => ($sum - $translated))),
    //($has_suggestion ? format_plural($has_suggestion, '@count has suggestions', '@count have suggestions') : t('no suggestions')),
  );
}

/**
 * Header columns for the progress data.
 */
function theme_l10n_community_progress_headers($variables) {
  $mainhead = $variables['mainhead'];
  return array(
    array('data' => $mainhead, 'class' => array('rowhead')),
    t('Progress'),
    t('Untranslated'),
    t('Suggestions'),
  );
}

// = API functions =============================================================

/**
 * Get string counts for summaries.
 *
 * @param $type string
 *   Type of string count to return:
 *     - all:        count of all strings
 *     - project:    count of strings in one project (identified by $id)
 *     - languages:  array of the count of translated strings and suggestions by langcode
 *     - projects:   array of the count of translated strings and suggestions by pid
 *     - top-people: array of counts for top translators for a given language
 * @param $id int
 *   The project id when the 'project' type is used. The result set can be
 *   restricted by an ID from the oppposing type for the later two types.
 *   Eg. 'projects' summaries can be restricted to one specific language, or
 *   'languages' summeries can be restricted to one specific project. This
 *   id represents the identifier (pid or langcode) of the restricting item.
 *   For the 'all' type, this value is discarded.
 * @return array
 *
 * @todo
 *   These queries are *slooow*. The query cache helps a lot with caching the
 *   result, so the slowness only shows for the first run, but still it would
 *   be good to look into optimizing these.
 */
function l10n_community_get_string_count($type, $id = NULL) {
  switch ($type) {
    case 'all':
      // Return a count of all strings.
      return db_query("SELECT COUNT(sid) FROM {l10n_server_string}")->fetchField();

    case 'project':
      // Return a count of all strings in this project, id required.
      return db_query('SELECT COUNT(DISTINCT l.sid) FROM {l10n_server_line} l WHERE l.pid = :pid', array(':pid' => $id))->fetchField();

    case 'languages':
      if ($stats = cache_get('l10n:count:languages:' . $id, 'cache')) {
        return $stats->data;
      }
      else {
        // Summeries based on language codes, restricted to a specific project if $id is set.
        $sums = array();
        if (!isset($id)) {
          // Simple count query if we are not filtering by project.
          $count1_sql = "SELECT COUNT(sid) AS translation_count, language FROM {l10n_server_status_flag} WHERE has_translation = 1 GROUP BY language";
          $count2_sql = "SELECT COUNT(sid) AS translation_count, language FROM {l10n_server_status_flag} WHERE has_suggestion = 1 GROUP BY language";
          $count_args = array();
        }
        else {
          // More complex joins if we also need to factor the project in.
          $count1_sql = "SELECT COUNT(DISTINCT ts.sid) AS translation_count, ts.language FROM {l10n_server_line} l INNER JOIN {l10n_server_status_flag} ts ON l.sid = ts.sid WHERE l.pid = :pid AND ts.has_translation = 1 GROUP BY ts.language ORDER BY NULL";
          $count2_sql = "SELECT COUNT(DISTINCT ts.sid) AS translation_count, ts.language FROM {l10n_server_line} l INNER JOIN {l10n_server_status_flag} ts ON l.sid = ts.sid WHERE l.pid = :pid AND ts.has_suggestion = 1 GROUP BY ts.language ORDER BY NULL";
          $count_args = array(':pid' => $id);
        }
        $result = db_query($count1_sql, $count_args);
        foreach ($result as $row) {
          $sums[$row->language]['translations'] = $row->translation_count;
        }
        $result = db_query($count2_sql, $count_args);
        foreach ($result as $row) {
          $sums[$row->language]['suggestions'] = $row->translation_count;
        }
        cache_set('l10n:count:languages:' . $id, $sums, 'cache', CACHE_PERMANENT);
        return $sums;
      }
      break;

    case 'top-people':
      // Get summaries of people having most active translations per language.
      // Skip anonymous since that is used for placeholders when there was
      // no prior translations for a suggestion.
      $result = db_query_range("SELECT COUNT(DISTINCT t.sid) AS sum, u.name, u.uid FROM {l10n_server_translation} t LEFT JOIN {users} u ON t.uid_entered = u.uid WHERE t.is_active = 1 AND t.is_suggestion = 0 AND t.language = :lng AND u.uid != 0 GROUP BY t.uid_entered ORDER BY sum DESC", 0, 10, array(':lng' => $id));
      $accounts = $result->fetchAll();

      return $accounts;
  }
}

/**
 * Replacement for l10n_community_get_string_count when former stats are too
 * slow
 *
 * @param int $pid
 *   The project id to get string counts for.
 * @param int $rid
 *   (Optional) The release id to get string counts for. If not provided, the
 *   latest release of the project will be considered.
 *
 * @return array
 *    first element is the number of original strings
 *    second element is the number of translations for each language
 */
function l10n_community_get_l10n_packager_string_count($pid, $rid = NULL) {
  // Faster queries with l10n_packager, first get latest release id from the
  // l10n_packager_file table for the project requested, then get the
  // translation count of the release.
  if (empty($rid)) {
    $rid = db_query("SELECT pf.rid FROM {l10n_server_release} sr INNER JOIN {l10n_packager_file} pf ON sr.rid = pf.rid WHERE pid = :pid ORDER BY pf.rid DESC LIMIT 0, 1", array(':pid' => $pid))->fetchField();
  }
  $results = db_query("SELECT language, sid_count FROM {l10n_packager_file} WHERE rid = :rid", array(':rid' => $rid))->fetchAllKeyed();
  foreach ($results as $language => $sid_count) {
    $sums[$language]['translations'] = $sid_count;
  }
  ksort($sums);
  // Finally, get the string count of the release
  $num_source = db_query("SELECT COUNT(DISTINCT l.sid) FROM {l10n_server_line} l WHERE l.rid = :rid", array(':rid' => $rid))->fetchField();
  return array($num_source, $sums);
}

/**
 * Create an alphabetic pager for browsing projects.
 */
function _l10n_community_alphabetic_pager($initial) {
  // Create an array of pager elements, setting a special class on the active element
  $initials = l10n_server_get_project_initials();
  $items = array();
  foreach ($initials as $link => $data) {
    if ($initial === (string) $link) {
      $items[] = array(
        'class' => array('pager-current'),
        'data' => $data['title'],
      );
    }
    else {
      $items[] = array(
        'class' => array('pager-item'),
        'data' => l($data['title'], 'translate/projects', array('query' => array('initial' => $link))),
      );
    }
  }

  // Only display this pager if there was over 2 items. 'All' is the first item
  // and then if we only have one item to filter additionally, we only have one
  // possible letter. Not much value for displaying this pager.
  $output = array(
    '#theme' => 'item_list',
    '#items' => $items,
    '#attributes' => array('class' => array('pager pager-alphabetic')),
  );

  return $output;
}

/**
 * Form callback to display a confirm form for resetting releases.
 *
 * @param $langcode string
 * @param $project_uri string
 * @param $release_rid int
 * @return
 */
function l10n_community_projects_release_reset($form, &$form_state, $langcode, $project_uri, $release_rid) {
  if (l10n_community_access('start over packages')) {
    $project = l10n_server_get_projects(array('uri' => $project_uri));
    $releases = l10n_server_get_releases($project_uri, FALSE);
    if (isset($releases[$release_rid])) {
      $release = $releases[$release_rid];
      $form = array(
        'langcode' => array(
          '#type' => 'value',
          '#value' => $langcode,
        ),
        'project' => array(
          '#type' => 'value',
          '#value' => $project,
        ),
        'release' => array(
          '#type' => 'value',
          '#value' => $release,
        ),
      );
      $question = t('Are you sure you would like to start over with %project %release on this Localization server?', array(
        '%project' => $project->title,
        '%release' => $release->title,
      ));
      $description = t('This would remove almost all data associated with the release: files, line and parsing error information. Source strings found within this project and translations for them are temporarily kept. This action cannot be undone.');
      return confirm_form($form, $question, 'translate/languages/' . $langcode . '/translate', $description);
    }
    else {
      backdrop_set_message(t('The release is not found, or the selected %release release does not belongs to the %project project.', array(
        '%project' => $form_state['values']['project']->title,
        '%release' => $form_state['values']['release']->title,
      )), 'error');
      backdrop_goto('translate/languages/' . $langcode . '/translate');
    }
  }
  else {
    backdrop_set_message(t('Please, contact your localization team manager (or any other translation community manager) with their personal contact form to solve this problem.'));
    backdrop_goto('translate/languages/' . $langcode . '/translate', array(
      'query' => array(  
        'project' => $project_uri,
        'release=' => $release_rid,
      ),
    ));
  }
}

/**
 * Form submission callback for resetting projects.
 */
function l10n_community_projects_release_reset_submit($form, &$form_state) {
  $project = l10n_server_get_projects(array('uri' => $form_state['values']['project']->uri));
  $releases = l10n_server_get_releases($form_state['values']['project']->uri, FALSE);
  if (isset($releases[$form_state['values']['release']->rid])) {
    module_load_include('inc', 'l10n_server', 'l10n_server.projects');
    l10n_server_delete_release($form_state['values']['release']->rid, TRUE);
    backdrop_set_message(t('Release data for %project %release was deleted from the Localization server.', array(
      '%project' => $project->title,
      '%release' => $form_state['values']['release']->title,
    )));
  }
  else {
    backdrop_set_message(t('The release is not found, or the selected %release release does not belongs to the %project project.', array(
      '%project' => $form_state['values']['project']->title,
      '%release' => $form_state['values']['release']->title,
    )), 'error');
  }
  $form_state['redirect'] = 'translate/languages/' . $form_state['values']['langcode'] . '/translate';
}
