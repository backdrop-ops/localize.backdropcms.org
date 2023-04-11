<?php

/**
 * @file
 *   Administrative functions for localization community UI.
 */

// = Settings form callback ====================================================

/**
 * Settings form callback.
 */
function l10n_community_settings_form() {
  $projects = l10n_server_get_projects();
  $config = config('l10n_community.settings');

  $form = array();
  $form['#config'] = 'l10n_community.settings';
  $community_project_per_page = $config->get('l10n_community_project_per_page');
  $community_project_per_page = empty($community_project_per_page) ? 10 : $community_project_per_page;
  $form['l10n_community_project_per_page'] = array(
    '#title' => t('Number of projects to display per page'),
    '#description' => t('Number of projects displayed per page in overviews. It is not advised to change this setting on live sites, as it changes what is displayed on each page, so links will break to existing project listings.'),
    '#type' => 'select',
    '#options' => backdrop_map_assoc(array(4, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40)),
    '#default_value' => $community_project_per_page,
  );
  $community_stats_enabled = $config->get('l10n_community_stats_enabled');
  $community_stats_enabled = is_null($community_stats_enabled) ? TRUE : $community_stats_enabled;
  $form['l10n_community_stats_enabled'] = array(
    '#title' => t('Enable global statistics for projects'),
    '#description' => t('You can disable this option in case of heavy load on your server.'),
    '#type' => 'checkbox',
    '#default_value' => $community_stats_enabled,
  );
  $community_stats_explore_projects_enabled = $config->get('l10n_community_stats_explore_projects_enabled');
  $community_stats_explore_projects_enabled = is_null($community_stats_explore_projects_enabled) ? TRUE : $community_stats_explore_projects_enabled;
  $form['l10n_community_stats_explore_projects_enabled'] = array(
    '#title' => t('Enable <em>Explore projects</em> page'),
    '#description' => t('You can disable this option in case of heavy load on your server as the <em>!link</em> page requires heavy SQL queries. This page also depends on the option above.', array('!link' => l('Explore projects', 'translate/projects'))),
    '#type' => 'checkbox',
    '#default_value' => $community_stats_explore_projects_enabled,
  );

  $community_package_count = $config->get('l10n_community_package_count');
  $community_package_count = is_null($community_package_count) ? TRUE : $community_package_count;
  $form['l10n_community_package_count'] = array(
    '#title' => t('Fetch translation counts from Localization packager'),
    '#description' => t('If l10n_packager module is enabled, translation counts will be fetched from parsed releases, thus skipping heavy SQL queries.'),
    '#type' => 'checkbox',
    '#default_value' => $community_package_count,
  );

  $form['l10n_community_import_user'] = array(
    '#title' => t('Account associated with bulk imports'),
    '#description' => t('The account associated with imports of Gettext files which have been translated by a team, so assigning the current user importing them would not be appropriate. Suggested username for this account is <em>Multiple contributors</em> to indicate the team nature.'),
    '#type' => 'textfield',
    '#default_value' => $config->get('l10n_community_import_user'),
    '#autocomplete_path' => 'user/autocomplete',
  );

  $form['l10n_community_highlighted_project'] = array(
    '#title' => t('Highlight project on the welcome page'),
    '#description' => t('Pick a project which will be highlighted on the welcome page, so progress of all translation teams can be seen instantly.'),
    '#default_value' => $config->get('l10n_community_highlighted_project'),
  );
  
  if (($count = count($projects)) <= 30) {
    // Radio box widget for as much as 5 projects, select widget for 5-30 projects.
    $form['l10n_community_highlighted_project']['#type'] = ($count <= 5 ? 'radios' : 'select');
    $form['l10n_community_highlighted_project']['#options'] = array('' => t('None'));
    foreach ($projects as $project) {
      // title used to conform to the autocomplete behavior.
      $form['l10n_community_highlighted_project']['#options'][$project->title] = $project->title;
    }
  }
  else {
    // Autocomplete field for more then 30 projects.
    $form['l10n_community_highlighted_project'] += array(
      '#type' => 'textfield',
      '#autocomplete_path' => 'translate/projects/autocomplete',
    );
    $form['l10n_community_highlighted_project']['#description'] .= ' ' . t('Can be set to empty string to not pick any project.');
  }
  return system_settings_form($form);
}
