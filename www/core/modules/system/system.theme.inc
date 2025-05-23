<?php
/**
 * @file
 * Theme functions for the System module.
 */

/**
 * Returns HTML for the Powered by Backdrop text.
 *
 * @ingroup themeable
 */
function theme_system_powered_by() {
  return '<span>' . t('Powered by <a href="@url">Backdrop CMS</a>', array('@url' => 'https://backdropcms.org')) . '</span>';
}

/**
 * Returns HTML for a confirmation form.
 *
 * By default this does not alter the appearance of a form at all,
 * but is provided as a convenience for themers.
 *
 * @param $variables
 *   An associative array containing:
 *   - form: A render element representing the form.
 *
 * @ingroup themeable
 */
function theme_confirm_form($variables) {
  return backdrop_render_children($variables['form']);
}

/**
 * Returns HTML for a system settings form.
 *
 * By default this does not alter the appearance of a form at all,
 * but is provided as a convenience for themers.
 *
 * @param $variables
 *   An associative array containing:
 *   - form: A render element representing the form.
 *
 * @ingroup themeable
 */
function theme_system_settings_form($variables) {
  return backdrop_render_children($variables['form']);
}

/**
 * Returns HTML for an exposed filter form.
 *
 * @param $variables
 *   An associative array containing:
 *   - form: An associative array containing the structure of the form.
 *
 * @return
 *   A string containing an HTML-formatted form.
 *
 * @ingroup themeable
 */
function theme_exposed_filters($variables) {
  $form = $variables['form'];
  $output = '';

  if (isset($form['current'])) {
    $items = array();
    foreach (element_children($form['current']) as $key) {
      $items[] = backdrop_render($form['current'][$key]);
    }
    $output .= theme('item_list', array('items' => $items, 'attributes' => array('class' => array('clearfix', 'current-filters'))));
  }

  $output .= backdrop_render_children($form);

  return '<div class="exposed-filters">' . $output . '</div>';
}

/**
 * Returns HTML for the Appearance page.
 *
 * @param $variables
 *   An associative array containing:
 *   - theme_groups: An associative array containing groups of themes.
 *
 * @ingroup themeable
 */
function theme_system_themes_page($variables) {
  $theme_groups = $variables['theme_groups'];

  $output = '<div id="system-themes-page">';

  foreach ($variables['theme_group_titles'] as $state => $title) {
    if (!count($theme_groups[$state])) {
      // Skip this group of themes if no theme is there.
      continue;
    }
    // Start new theme group.
    $output .= '<div class="system-themes-list system-themes-list-'. $state .' clearfix"><h2>'. $title .'</h2>';

    foreach ($theme_groups[$state] as $theme) {

      // Theme the screenshot.
      $screenshot = $theme->screenshot ? theme('image', $theme->screenshot) : '<div class="no-screenshot">' . t('no screenshot') . '</div>';

      // Localize the theme description.
      $description = t($theme->info['description']);
      $prefix = '';
      $depends = '';
      if (isset($theme->info['base theme'])) {
        $base_theme_name = $theme->info['base theme'];
        $base_theme_label = !empty($theme->base_themes[$base_theme_name]) ? $theme->base_themes[$base_theme_name] : $base_theme_name;
        $depends = '<br><strong>' . t('Base theme') . ':</strong> ' . check_plain($base_theme_label);
        if (isset($theme->missing_dependencies)) {
          $theme->classes[] = 'missing-dependency';
          $icon = icon('warning', array(
            'attributes' => array('width' => 24, 'height' => 24),
          ));
          $prefix = '<div class="missing-warning">' . $icon . ' ' . t('Missing dependency') . '</div>';
          if (in_array($base_theme_name, $theme->missing_dependencies)) {
            $depends .= ' <em>(' . t('missing') . ')</em>';
          }
        }
      }

      // Style theme info
      $notes = count($theme->notes) ? ' (' . join(', ', $theme->notes) . ')' : '';
      $theme->classes[] = 'theme-selector';
      $theme->classes[] = 'clearfix';
      $output .= '<div class="'. join(' ', $theme->classes) .'">' . $screenshot . '<div class="theme-info"><h3>' . $theme->info['name'] . ' ' . (isset($theme->info['version']) ? $theme->info['version'] : '') . $notes . '</h3><div class="theme-description">' . $prefix . $description . $depends . '</div>';

      // Make sure to provide feedback on compatibility.
      if (!empty($theme->incompatible_core)) {
        $output .= '<div class="incompatible">' . t('This version is not compatible with Backdrop CMS !core_version.', array('!core_version' => BACKDROP_CORE_COMPATIBILITY)) . '</div>';
      }
      elseif (!empty($theme->incompatible_php)) {
        if (substr_count($theme->info['php'], '.') < 2) {
          $theme->info['php'] .= '.*';
        }
        $output .= '<div class="incompatible">' . t('This theme requires PHP version @php_required and is incompatible with PHP version !php_version.', array('@php_required' => $theme->info['php'], '!php_version' => phpversion())) . '</div>';
      }
      else {
        $output .= theme('links', array('links' => $theme->operations, 'attributes' => array('class' => array('operations', 'clearfix'))));
      }
      $output .= '</div></div>';
    }
    $output .= '</div>';
  }
  $output .= '</div>';

  return $output;
}

/**
 * Returns HTML for the modules form.
 *
 * @param $variables
 *   An associative array containing:
 *   - form: A render element representing the form.
 *
 * @ingroup themeable
 */
function theme_system_modules_fieldset($variables) {
  $form = $variables['form'];

  // Individual table headers.
  $rows = array();
  // Iterate through all the modules, which are
  // children of this fieldset.
  foreach (element_children($form) as $key) {
    // Stick the key into $module for easier access.
    $module = $form[$key];
    // Create the row for the table.
    $row = array();
    unset($module['enable']['#title']);
    $module['#requires'] = array_filter($module['#requires']);
    $module['#required_by'] = array_filter($module['#required_by']);

    $requires = !empty($module['#requires']);
    $required_by = !empty($module['#required_by']);
    $requirements = '';
    $toggle = '';

    // Add the first column: the checkbox.
    $row[] = array('class' => array('checkbox'), 'data' => backdrop_render($module['enable']));

    // Add the second column: module label and expand/collapse functionality.
    $row[] = array('class' => array('module'), 'data' => '<label id="module-' . $key . '" for="' . $module['enable']['#id'] . '" class="module-name table-filter-text-source">' . backdrop_render($module['name']) . '</label>');

    // Add the third column: module version number.
    $row[] = array('class' => array('version'), 'data' => backdrop_render($module['version']));

    $requirements .= '<div class="requirements">';
    if ($requires) {
      $requirements .= '<div class="admin-requirements">' . t('Requires:') . ' ' . implode(', ', $module['#requires']) . '</div>';
    }
    if ($required_by) {
      $requirements .= '<div class="admin-requirements">' . t('Required by:') . ' ' . implode(', ', $module['#required_by']) . '</div>';
    }
    if (isset($module['#project']) && $module['#project'] != 'backdrop') {
      $requirements .= '<div class="admin-requirements project-links">' . t('Links:') . ' ' . l(t("BackdropCMS.org"), 'https://backdropcms.org/project/' . $module['#project']) . ', ' . l(t("GitHub"), 'https://github.com/backdrop-contrib/' . $module['#project']) . '</div>';
    }
    $requirements .= '<div class="admin-requirements table-filter-text-source">' . t('Tags:') . ' ' . backdrop_render($module['tags']) . '</div>';
    $requirements .= '</div>';

    // Build a requirements toggle.
    $toggle .= ' <a class="requirements-toggle" href="#">' . t('more') . '<span class="arrow"></span></a>';

    // Add the description, along with any modules it requires.
    $details = '<div class="text table-filter-text-source"> ' . backdrop_render($module['description']) . $toggle . '</div>' . $requirements;

    // Add the fourth column: description, along with any modules it requires.
    $row[] = array('class' => array('description'), 'data' => $details);

    // Add the fifth column: links (such as configure or permissions) in their own columns.
    $row[] = array('data' => backdrop_render($module['links']), 'class' => array('operations'));

    // Hide modules that can't be disabled at all.
    if (!$module["#required_by_distribution"]) {
      $rows[] = $row;
    }
  }

  return theme('table', array('header' => $form['#header'], 'rows' => $rows));
}

/**
 * Returns HTML for a message about incompatible modules.
 *
 * @param $variables
 *   An associative array containing:
 *   - message: The form array representing the currently disabled modules.
 *
 * @ingroup themeable
 */
function theme_system_modules_incompatible($variables) {
  return '<div class="incompatible">' . $variables['message'] . '</div>';
}

/**
 * Returns HTML for a table of currently disabled modules.
 *
 * @param $variables
 *   An associative array containing:
 *   - form: A render element representing the form.
 *
 * @ingroup themeable
 */
function theme_system_modules_uninstall($variables) {
  $form = $variables['form'];

  // No special theme override for the confirm form.
  if (isset($form['confirm'])) {
    return backdrop_render($form);
  }

  // Table headers.
  $header = array(t('Uninstall'),
    t('Name'),
    t('Description'),
  );

  // Display table.
  $rows = array();
  foreach (element_children($form['modules']) as $module) {
    if (!empty($form['modules'][$module]['#required_by'])) {
      $disabled_message = format_plural(count($form['modules'][$module]['#required_by']),
        'To uninstall @module, the following module must be uninstalled first: @required_modules',
        'To uninstall @module, the following modules must be uninstalled first: @required_modules',
        array(
          '@module' => $form['modules'][$module]['#module_name'],
          '@required_modules' => implode(', ', $form['modules'][$module]['#required_by']),
        ));
      $disabled_message = '<div class="admin-requirements">' . $disabled_message . '</div>';
    }
    else {
      $disabled_message = '';
    }
    $rows[] = array(
      array(
        'data' => backdrop_render($form['uninstall'][$module]),
        'align' => 'center',
      ),
      '<strong><label for="' . $form['uninstall'][$module]['#id'] . '">' . backdrop_render($form['modules'][$module]['name']) . '</label></strong>',
      array(
        'data' => backdrop_render($form['modules'][$module]['description']) . $disabled_message,
        'class' => array('description'),
      ),
    );
  }

  $form['modules'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('No modules are available to uninstall.'),
  );

  return backdrop_render_children($form);
}

/**
 * Returns HTML for the status report.
 *
 * This theme function is dependent on install.inc being loaded, because
 * that's where the constants are defined.
 *
 * @param $variables
 *   An associative array containing:
 *   - requirements: An array of requirements/status items. Each requirement
 *     is an associative array containing the following elements:
 *     - title: The name of the requirement.
 *     - value: (optional) The current value (version, time, level, etc).
 *     - description: (optional) The description of the requirement.
 *     - severity: (optional) The requirement's result/severity level, one of:
 *       - REQUIREMENT_INFO: Status information.
 *       - REQUIREMENT_OK: The requirement is satisfied.
 *       - REQUIREMENT_WARNING: The requirement failed with a warning.
 *       - REQUIREMENT_ERROR: The requirement failed with an error.
 *   - phase: The current phase. Possible values are one of:
 *     - install: When called from install.php.
 *     - update: When called from update.php.
 *     - runtime: Showing the normal status report page, or no phase specified.
 *
 * @ingroup themeable
 * @since 1.26.2 support for the "phase" variable added.
 */
function theme_status_report($variables) {
  $requirements = $variables['requirements'];
  $phase = isset($variables['phase']) ? $variables['phase'] : 'runtime';
  backdrop_sort($requirements, array('title' => SORT_STRING));

  // Separate requirements by severity.
  $errors = array();
  $warnings = array();
  $infos = array();
  $okays = array();
  foreach ($requirements as $requirement) {
    if (!isset($requirement['severity'])) {
      $requirement['severity'] = REQUIREMENT_OK;
    }
    switch ($requirement['severity']) {
      case REQUIREMENT_ERROR:
        $errors[] = $requirement;
        break;
      case REQUIREMENT_WARNING:
        $warnings[] = $requirement;
        break;
      case REQUIREMENT_INFO:
        $infos[] = $requirement;
        break;
      case REQUIREMENT_OK:
        $okays[] = $requirement;
        break;
    }
  }
  // Put them back together in the desired order.
  $requirements = array_merge($errors, $warnings, $infos, $okays);

  $severities = array(
    REQUIREMENT_INFO => array(
      'title' => t('Info'),
      'class' => array('report-info'),
    ),
    REQUIREMENT_OK => array(
      'title' => t('OK'),
      'class' => array('report-status'),
    ),
    REQUIREMENT_WARNING => array(
      'title' => t('Warning'),
      'class' => array('report-warning'),
    ),
    REQUIREMENT_ERROR => array(
      'title' => t('Error'),
      'class' => array('report-error'),
    ),
  );

  $system_status_report_table = array(
    'header' => array(),
    'rows' => array(),
    'attributes' => array('class' => array('system-status-report', 'phase-' . $phase)),
    'caption' => '',
    'colgroups' => array(),
    'sticky' => FALSE,
    'empty' => 'No status report. Something is definitely wrong.',
  );

  $rows = array();
  foreach ($requirements as $requirement) {
    $severity = $severities[isset($requirement['severity']) ? (int) $requirement['severity'] : REQUIREMENT_OK];
    $severity['icon'] = '<div title="' . $severity['title'] . '"><span class="element-invisible">' . $severity['title'] . '</span></div>';

    // The requirement's 'value' key is optional, provide a default value.
    $requirement['value'] = isset($requirement['value']) ? $requirement['value'] : '';

    $title = $requirement['title'] . '<br/>';
    $title .= '<span class="status-summary-mobile">' . $requirement['value'] . '</span>';

    $more = ' ';
    $details = '';
    if (!empty($requirement['description'])) {
      $more .= '<a class="description-toggle" href="#">more</a>';
      $details = '<div class="status-description">' . $requirement['description']. '</div>';
    }

    // For the "Requirements problem" page of the installer, place the label and
    // the description of each check/row in a single table cell.
    if ($phase == 'install') {
      $description = '<div class="status-summary status-value">' . $requirement['value'] . $more . '</div>' . $details;
      $data = array(
        array('data' => $severity['icon'], 'class' => 'report-icon'),
        array('data' => $title . $description, 'class' => 'status-title'),
      );
    }
    // For the site status report page, place the label and the description of
    // each check in separate table cells in the same row.
    else {
      $description = '<div class="status-summary">' . $requirement['value'] . $more . '</div>' . $details;
      $data = array(
        array('data' => $severity['icon'], 'class' => 'report-icon'),
        array('data' => $title, 'class' => 'status-title'),
        array('data' => $description, 'class' => array('status-value', 'priority-low')),
      );
    }
    $row = array(
      'data' => $data,
      'class' => $severity['class'],
    );

    $rows[] = $row;
  }

  $system_status_report_table['rows'] = $rows;

  return theme_table($system_status_report_table);
}

/**
 * Returns HTML for an administrative page.
 *
 * @param $variables
 *   An associative array containing:
 *   - blocks: An array of blocks to display. Each array should include a
 *     'title', a 'description' and a formatted 'content'.
 *
 * @ingroup themeable
 */
function theme_admin_page($variables) {
  $blocks = $variables['blocks'];
  $container = array();

  foreach ($blocks as $block) {
    if ($block_output = theme('admin_block', array('block' => $block))) {
      $container[] = $block_output;
    }
  }

  if (!empty($container)) {
    $output = '<div class="admin clearfix">';
    foreach ($container as $data) {
      $output .= $data;
    }
    $output .= '</div>';
  }
  else {
    $output = t('You do not have any administrative items.');
  }
  return $output;
}

/**
 * Returns HTML for an administrative block for display.
 *
 * @param $variables
 *   An associative array containing:
 *   - block: An array containing information about the block:
 *     - show: A Boolean whether to output the block. Defaults to FALSE.
 *     - title: The block's title.
 *     - icon: (optional) The block's icon.
 *     - content: (optional) Formatted content for the block.
 *     - description: (optional) Description of the block. Only output if
 *       'content' is not set.
 *
 * @ingroup themeable
 */
function theme_admin_block($variables) {
  $block = $variables['block'];
  $classes = array('admin-panel');
  // Construct a more specific class.
  if (isset($variables['block']['path'])) {
    $path_parts = explode('/', $variables['block']['path']);
    $class_suffix = end($path_parts);
    $classes[] = ' ' . backdrop_clean_css_identifier('admin-panel-' . $class_suffix);
  }

  $output = '';
  // Don't display the block if it has no content to display.
  if (empty($block['show'])) {
    return $output;
  }

  $output .= '<div class="' . implode(' ', $classes) . '">';
  if (!empty($block['title'])) {
    $icon = empty($block['icon']) ? '' : '<span class="admin-panel-icon">' . icon($block['icon']) . '</span>';
    $title = '<span class="admin-panel-title">' . $block['title'] . '</span>';
    $output .= '<h3>' . $icon . $title . '</h3>';
  }
  if (!empty($block['content'])) {
    $output .= '<div class="body">' . $block['content'] . '</div>';
  }
  else {
    $output .= '<div class="description">' . $block['description'] . '</div>';
  }
  $output .= '</div>';

  return $output;
}

/**
 * Returns HTML for the content of an administrative block.
 *
 * @param $variables
 *   An associative array containing:
 *   - content: An array containing information about the block. Each element
 *     of the array represents an administrative menu item, and must at least
 *     contain the keys 'title', 'href', and 'localized_options', which are
 *     passed to l(). A 'description' key may also be provided.
 *
 * @ingroup themeable
 */
function theme_admin_block_content($variables) {
  $content = $variables['content'];
  $output = '';

  if (!empty($content)) {
    $class = 'admin-list';
    $output .= '<dl class="' . $class . '">';
    foreach ($content as $item) {
      $output .= '<dt>' . l($item['title'], $item['href'], $item['localized_options']) . '</dt>';
      if (isset($item['description'])) {
        $output .= '<dd>' . filter_xss_admin($item['description']) . '</dd>';
      }
    }
    $output .= '</dl>';
  }
  return $output;
}

/**
 * Returns HTML for the output of the dashboard page.
 *
 * @param $variables
 *   An associative array containing:
 *   - menu_items: An array of modules to be displayed.
 *
 * @ingroup themeable
 */
function theme_system_admin_index($variables) {
  $menu_items = $variables['menu_items'];
  $blocks = array();

  // Iterate over all modules.
  foreach ($menu_items as $module => $block) {
    list($description, $items) = $block;

    // Output links.
    if (count($items)) {
      $block = array();
      $block['title'] = $module;
      $block['content'] = theme('admin_block_content', array('content' => $items));
      $block['description'] = t($description);
      $block['show'] = TRUE;

      if ($block_output = theme('admin_block', array('block' => $block))) {
        $blocks[] = $block_output;
      }
    }
  }

  $output = '<div class="admin clearfix">';
  foreach ($blocks as $block) {
    $output .= $block;
  }
  $output .= '</div>';

  return $output;
}

/**
 * Theme a link to a token tree either as a regular link or a dialog.
 */
function theme_token_tree_link($variables) {
  $options = array(
    'attributes' => array(),
    'query' => array(
      'token' => backdrop_get_token('token-browser'),
      'show_restricted' => $variables['show_restricted'] ? 1 : 0,
    ),
  );

  if (!empty($variables['token_types'])) {
    $options['query']['token_types'] = backdrop_json_encode($variables['token_types']);
    $options['query']['global_types'] = backdrop_json_encode($variables['global_types']);
  }

  if (!empty($variables['dialog'])) {
    backdrop_add_library('system', 'token');
    backdrop_add_library('system', 'backdrop.ajax');
    $options['attributes']['class'][] = 'use-ajax token-browser-link';
  }
  else {
    $options['attributes']['target'] = '_blank';
  }
  // Create a security token.
  $settings['tokenBrowser'] = array(
    'token' => backdrop_get_token('token-browser-endpoint'),
    'restricted' => $variables['show_restricted'],
    'click_insert' => $variables['click_insert'],
  );
  backdrop_add_js($settings, 'setting');
  return l($variables['text'], 'token/tree', $options);
}

/**
 * Provide a 'tree' display of nested tokens.
 *
 * @ingroup themeable
 */
function theme_token_tree($variables) {
  if (!empty($variables['dialog'])) {
    return theme('token_tree_link', $variables);
  }
  module_load_include('inc', 'system', 'system.pages');
  $content = system_token_browser_tree_build_content($variables['token_types'], $variables['global_types'], $variables['show_restricted']);
  // Create a security token.
  $settings['tokenBrowser'] = array(
    'token' => backdrop_get_token('token-browser-endpoint'),
    'restricted' => $variables['show_restricted'],
    'click_insert' => $variables['click_insert'],
  );
  backdrop_add_js($settings, 'setting');

  return backdrop_render($content);
}

/**
 * Prepares variables for Page Component block templates.
 */
function system_preprocess_page_components(&$variables) {
  $settings = $variables['settings'];
  $child_delta = $variables['child_delta'];
  $variables += array(
    'action_links' => NULL,
    'tabs' => NULL,
    'messages' => NULL,
    'title' => NULL,
  );

  if ($child_delta == 'title' || $child_delta == 'title_combo') {
    $variables['title'] = $settings['title'];
    $variables['title_tag'] = $settings['title_tag'];
    $variables['title_classes'] = $settings['title_classes'];
    $variables['title_attributes'] = array(
      'class' => array_filter(explode(' ', $settings['title_classes'])),
    );
  }
  if ($child_delta == 'tabs' || $child_delta == 'title_combo') {
    $menus =  array();
    switch ($settings['tab_type']) {
      case 'primary':
        $menus =  array(
          'primary' => menu_primary_local_tasks(),
        );
        break;
      case 'secondary':
        $menus =  array(
          'secondary' => menu_secondary_local_tasks(),
        );
        break;
      case 'both':
        $menus =  array(
          'primary' => menu_primary_local_tasks(),
          'secondary' => menu_secondary_local_tasks(),
        );
        break;
    }
    if ($settings['tab_type'] == 'hidden') {
      $variables['tabs'] = '';
    }
    else {
      $variables['tabs'] = theme('menu_local_tasks', $menus);
    }
  }
  if ($child_delta == 'action_links' || $child_delta == 'title_combo') {
    $variables['action_links'] = menu_local_actions();
  }

  if ($child_delta == 'messages' || $child_delta == 'title_combo') {
    $variables['messages'] = theme('status_messages');
  }

  // Add specific suggestions that can override the default implementation.
  $variables['theme_hook_suggestions'] = array(
    'page_components__' . $child_delta,
  );
}

/**
 * Returns HTML for the display of a label and its machine name.
 *
 * @param array $variables
 *   An associative array containing:
 *   - label: The human-readable label.
 *   - machine_name: (optional) The machine-safe name.
 *
 * @ingroup themeable
 */
function theme_label_machine_name($variables) {
  $output = '<div class="label">' . check_plain($variables['label']) . '</div>';

  if (!empty($variables['machine_name'])) {
    $machine_name = t('Machine name: @machine_name', array('@machine_name' => $variables['machine_name']));
    $output .= '<div class="machine-name">(' . $machine_name . ')</div>';
  }

  return $output;
}
