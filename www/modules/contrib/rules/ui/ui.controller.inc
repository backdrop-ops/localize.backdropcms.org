<?php
/**
 * @file
 * Contains the UI controller for Rules.
 */

/**
 * Controller class for the Rules UI.
 *
 * The Rules UI controller defines the methods other modules may use in order
 * to easily re-use the UI, regardless whether the rules admin module is
 * enabled.
 */
class RulesUIController {

  /**
   * Generates menu items to manipulate rules configurations.
   *
   * @param string $base_path
   *   The path to the overview page from where the configurations are edited.
   */
  public function config_menu($base_path) {
    $items = array();
    $base_count = count(explode('/', $base_path));
    $items[$base_path . '/manage/%rules_config'] = array(
      'title callback' => 'rules_get_title',
      'title arguments' => array('Editing !plugin "!label"', $base_count + 1),
      'page callback' => 'backdrop_get_form',
      'page arguments' => array('rules_ui_form_edit_rules_config', $base_count + 1, $base_path),
      'access callback' => 'rules_config_access',
      'access arguments' => array('update', $base_count + 1),
      'type' => MENU_VISIBLE_IN_BREADCRUMB,
      'file' => 'ui/ui.forms.inc',
      'file path' => backdrop_get_path('module', 'rules'),
    );
    $items[$base_path . '/manage/%rules_config/add/%rules_element'] = array(
      // Adding another part to the path would hit the menu path-part-limit
      // for base paths like admin/config/workflow/rules. Therefore we have to
      // use this ugly way for setting the title.
      'title callback' => 'rules_menu_add_element_title',
      // Wrap the integer in an array, so it is passed as is.
      'title arguments' => array(array($base_count + 4)),
      'page callback' => 'backdrop_get_form',
      'page arguments' => array('rules_ui_add_element', $base_count + 1, $base_count + 4, $base_count + 3, $base_path),
      'access callback' => 'rules_config_access',
      'access arguments' => array('update', $base_count + 1),
      'load arguments' => array($base_count + 1),
      'file' => 'ui/ui.forms.inc',
      'file path' => backdrop_get_path('module', 'rules'),
    );
    $items[$base_path . '/manage/%rules_config/add/event'] = array(
      'title callback' => 'rules_get_title',
      'title arguments' => array('Adding event to !plugin "!label"', $base_count + 1),
      'page callback' => 'backdrop_get_form',
      'page arguments' => array('rules_ui_add_event_page', $base_count + 1, $base_path),
      'access callback' => 'rules_config_access',
      'access arguments' => array('update', $base_count + 1),
      'load arguments' => array($base_count + 1),
      'file' => 'ui/ui.forms.inc',
      'file path' => backdrop_get_path('module', 'rules'),
    );
    $items[$base_path . '/manage/%rules_config/delete/event'] = array(
      // @todo Improve title.
      'title' => 'Remove event',
      'page callback' => 'backdrop_get_form',
      'page arguments' => array('rules_ui_remove_event', $base_count + 1, $base_count + 4, $base_path),
      'access callback' => 'rules_config_access',
      'access arguments' => array('update', $base_count + 1),
      'description' => 'Remove an event from a reaction rule.',
      'file' => 'ui/ui.forms.inc',
      'file path' => backdrop_get_path('module', 'rules'),
    );
    $items[$base_path . '/manage/%rules_config/edit/%rules_element'] = array(
      'title callback' => 'rules_get_title',
      'title arguments' => array('Editing !plugin "!label"', $base_count + 3),
      'page callback' => 'backdrop_get_form',
      'page arguments' => array('rules_ui_edit_element', $base_count + 1, $base_count + 3, $base_path),
      'access callback' => 'rules_config_access',
      'access arguments' => array('update', $base_count + 1),
      'load arguments' => array($base_count + 1),
      'file' => 'ui/ui.forms.inc',
      'file path' => backdrop_get_path('module', 'rules'),
    );
    $items[$base_path . '/manage/%rules_config/autocomplete'] = array(
      'page callback' => 'rules_ui_form_data_selection_auto_completion',
      'page arguments' => array($base_count + 3, $base_count + 4, $base_count + 5),
      'access callback' => 'rules_config_access',
      'access arguments' => array('update', $base_count + 1),
      'type' => MENU_CALLBACK,
      'file' => 'ui/ui.forms.inc',
      'file path' => backdrop_get_path('module', 'rules'),
    );
    $items[$base_path . '/manage/%rules_config/delete/%rules_element'] = array(
      'title callback' => 'rules_get_title',
      'title arguments' => array('Editing !plugin "!label"', $base_count + 3),
      'page callback' => 'backdrop_get_form',
      'page arguments' => array('rules_ui_delete_element', $base_count + 1, $base_count + 3, $base_path),
      'access callback' => 'rules_config_access',
      'access arguments' => array('update', $base_count + 1),
      'load arguments' => array($base_count + 1),
      'file' => 'ui/ui.forms.inc',
      'file path' => backdrop_get_path('module', 'rules'),
    );
    $items[$base_path . '/manage/%rules_config/%'] = array(
      'page callback' => 'backdrop_get_form',
      'page arguments' => array('rules_ui_form_rules_config_confirm_op', $base_count + 1, $base_count + 2, $base_path),
      'access callback' => 'rules_config_access',
      'access arguments' => array('update', $base_count + 1),
      'file' => 'ui/ui.forms.inc',
      'file path' => backdrop_get_path('module', 'rules'),
    );
    $items[$base_path . '/manage/%rules_config/clone'] = array(
      'title callback' => 'rules_get_title',
      'title arguments' => array('Cloning !plugin "!label"', $base_count + 1),
      'page callback' => 'backdrop_get_form',
      'page arguments' => array('rules_ui_form_clone_rules_config', $base_count + 1, $base_path),
      'access callback' => 'rules_config_access',
      'access arguments' => array('update', $base_count + 1),
      'file' => 'ui/ui.forms.inc',
      'file path' => backdrop_get_path('module', 'rules'),
    );
    $items[$base_path . '/manage/%rules_config/export'] = array(
      'title callback' => 'rules_get_title',
      'title arguments' => array('Export of !plugin "!label"', $base_count + 1),
      'page callback' => 'backdrop_get_form',
      'page arguments' => array('rules_ui_form_export_rules_config', $base_count + 1, $base_path),
      'access callback' => 'rules_config_access',
      'access arguments' => array('view', $base_count + 1),
      'file' => 'ui/ui.forms.inc',
      'file path' => backdrop_get_path('module', 'rules'),
    );
    $items[$base_path . '/manage/%rules_config/execute'] = array(
      'title callback' => 'rules_get_title',
      'title arguments' => array('Executing !plugin "!label"', $base_count + 1),
      'page callback' => 'backdrop_get_form',
      'page arguments' => array('rules_ui_form_execute_rules_config', $base_count + 1, $base_path),
      'access callback' => 'rules_config_access',
      'access arguments' => array('update', $base_count + 1),
      'file' => 'ui/ui.forms.inc',
      'file path' => backdrop_get_path('module', 'rules'),
    );
    backdrop_alter('rules_ui_menu', $items, $base_path, $base_count);

    if (module_exists('rules_scheduler')) {
      $items[$base_path . '/manage/%rules_config/schedule'] = array(
        'title callback' => 'rules_get_title',
        'title arguments' => array('Schedule !plugin "!label"', $base_count + 1),
        'page callback' => 'backdrop_get_form',
        'page arguments' => array('rules_scheduler_schedule_form', $base_count + 1, $base_path),
        'access callback' => 'rules_config_access',
        'access arguments' => array('update', $base_count + 1),
        'file' => 'rules_scheduler.admin.inc',
        'file path' => backdrop_get_path('module', 'rules_scheduler'),
      );
    }
    return $items;
  }

  /**
   * Generates the render array for an overview configuration table.
   *
   * Generates the render array for an overview configuration table for
   * arbitrary rule configs that match the given conditions.
   *
   * Note: The generated overview table contains multiple links for editing the
   * rule configurations. For the links to properly work use
   * RulesUIController::config_menu($base_path) to generate appropriate menu
   * items for the path at which the overview table is displayed.
   *
   * @param array $conditions
   *   An array of conditions as needed by rules_config_load_multiple().
   * @param array $options
   *   An array with optional options. Known keys are:
   *   - 'hide status op': If set to TRUE, enable/disable links are not added.
   *     Defaults to FALSE.
   *   - 'show plugin': If set to FALSE, the plugin is not shown. Defaults to
   *     TRUE.
   *   - 'show events': If set to TRUE, the event column is shown. Defaults to
   *     TRUE if only reaction rules are listed.
   *   - 'show execution op': If set to TRUE an operation for execution a
   *     component is shown for components, as well as a link to schedule a
   *     component if the rules scheduler module is enabled.
   *   - 'base path': Optionally, a different base path to use instead of the
   *     currently set RulesPluginUI::$basePath. If no base path has been set
   *     yet, the current path is used by default.
   *
   * @return array
   *   A renderable array.
   */
  public function overviewTable($conditions = array(), $options = array()) {
    $options += array(
      'hide status op' => FALSE,
      'show plugin' => TRUE,
      'show events' => isset($conditions['plugin']) && $conditions['plugin'] == 'reaction rule',
      'show execution op' => !(isset($conditions['plugin']) && $conditions['plugin'] == 'reaction rule'),
    );
    // By default show only configurations owned by rules.
    $conditions += array(
      'owner' => 'rules',
    );
    if (!empty($options['base path'])) {
      RulesPluginUI::$basePath = $options['base path'];
    }
    elseif (!isset(RulesPluginUI::$basePath)) {
      // Default to the current path, only if no path has been set yet.
      RulesPluginUI::$basePath = current_path();
    }

    $entities = entity_load_multiple('rules_config', FALSE, $conditions);
    ksort($entities);

    // Prepare some variables used by overviewTableRow().
    $this->event_info = rules_fetch_data('event_info');
    $this->cache = rules_get_cache();

    $rows = array();
    foreach ($entities as $id => $entity) {
      if (user_access('bypass rules access') || $entity->access()) {
        $rows[] = $this->overviewTableRow($conditions, $id, $entity, $options);
      }
    }
    // Assemble the right table header.
    $header = array(
      t('Name'),
      t('Event'),
      t('Plugin'),
      t('Status'),
      t('Operations'),
    );
    if (!$options['show events']) {
      // Remove the event heading as there is no such column.
      unset($header[1]);
    }
    if (!$options['show plugin']) {
      unset($header[2]);
    }

    $table = array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
      '#empty' => t('None.'),
    );
    $table['#attributes']['class'][] = 'rules-overview-table';
    $table['#attached']['css'][] = backdrop_get_path('module', 'rules') . '/ui/rules.ui.css';

    // @todo Hide configs where access() is FALSE.
    return $table;
  }

  /**
   * Generates the row for a single rules config.
   *
   * @param array $conditions
   *   An array of conditions as needed by rules_config_load_multiple().
   * @param array $options
   *   An array with optional options. Known keys are:
   *   - 'hide status op': If set to TRUE, enable/disable links are not added.
   *     Defaults to FALSE.
   *   - 'show plugin': If set to FALSE, the plugin is not shown. Defaults to
   *     TRUE.
   *   - 'show events': If set to TRUE, the event column is shown. Defaults to
   *     TRUE if only reaction rules are listed.
   *   - 'show execution op': If set to TRUE an operation for execution a
   *     component is shown for components, as well as a link to schedule a
   *     component if the rules scheduler module is enabled.
   *   - 'base path': Optionally, a different base path to use instead of the
   *     currently set RulesPluginUI::$basePath. If no base path has been set
   *     yet, the current path is used by default.
   */
  protected function overviewTableRow($conditions, $name, $config, $options) {
    // Build content includes the label, as well as a short overview including
    // the machine name.
    $row[] = array('data' => $config->buildContent());

    // Add events if the configs are assigned to events.
    if ($options['show events']) {
      $events = array();
      if ($config instanceof RulesTriggerableInterface) {
        foreach ($config->events() as $event_name) {
          $event_handler = rules_get_event_handler($event_name, $config->getEventSettings($event_name));
          $events[] = $event_handler->summary();
        }
      }
      $row[] = implode(", ", $events);
    }
    if ($options['show plugin']) {
      $plugin = $config->plugin();
      $row[] = isset($this->cache['plugin_info'][$plugin]['label']) ? $this->cache['plugin_info'][$plugin]['label'] : $plugin;
    }

    $row[] = array('data' => array(
      '#theme' => 'entity_ui_entity_status',
      '#status' => $config->status,
    ));

    // Add operations depending on the options and the exportable status.
    $operations = array();
    if (!$config->hasStatus(ENTITY_FIXED)) {
      $operations['edit'] = array(
        'title' => t('Edit'),
        'href' => RulesPluginUI::path($name),
        'attributes' => array(
          'class' => array(
              'edit',
              'action',
          ),
        ),
      );
      if (module_exists('rules_i18n')) {
        $operations['translate'] = array(
          'title' => t('Translate'),
          'href' => RulesPluginUI::path($name, 'translate'),
          'attributes' => array(
            'class' => array(
                'translate',
                'action',
            ),
          ),
        );
      }
    }

    if (!$options['hide status op']) {
      // Add either an enable or disable link.
      $text = $config->active ? t('disable') : t('enable');
      $label = $config->active ? 'Disable' : 'Enable';
      $link_path = RulesPluginUI::path($name, $text);
      if (!$config->hasStatus(ENTITY_FIXED)) {
        $operations[$text] = array(
          'title' => t($label),
          'href' => $link_path,
          'attributes' => array(
            'class' => array(
                $text,
                'action',
            ),
          ),
          'query' => backdrop_get_destination(),
        );
      }
      $operations['clone'] = array(
        'title' => t('Clone'),
        'href' => RulesPluginUI::path($name, 'clone'),
        'attributes' => array(
          'class' => array(
              'clone',
              'action',
          ),
        ),
      );
    }

    // Add execute link for for components.
    if ($options['show execution op']) {
      if (!($config instanceof RulesTriggerableInterface)) {
        $operations['execute'] = array(
          'title' => t('Execute'),
          'href' => RulesPluginUI::path($name, 'execute'),
          'attributes' => array(
            'class' => array(
                'execute',
                'action',
            ),
          ),
          'query' => backdrop_get_destination(),
        );
      }
      if (module_exists('rules_scheduler')) {
        $operations['schedule'] = array(
          'title' => t('Schedule'),
          'href' => RulesPluginUI::path($name, 'schedule'),
          'attributes' => array(
            'class' => array(
                'schedule',
                'action',
            ),
          ),
          'query' => backdrop_get_destination(),
        );
      }
    }
    if (!$config->hasStatus(ENTITY_IN_CODE) && !$config->hasStatus(ENTITY_FIXED)) {
      $operations['delete'] = array(
        'title' => t('Delete'),
        'href' => RulesPluginUI::path($name, 'delete'),
        'attributes' => array(
          'class' => array(
              'delete',
              'action',
          ),
        ),
        'query' => backdrop_get_destination(),
      );
    }
    elseif ($config->hasStatus(ENTITY_OVERRIDDEN) && !$config->hasStatus(ENTITY_FIXED)) {
      $operations['revert'] = array(
        'title' => t('Revert'),
        'href' => RulesPluginUI::path($name, 'revert'),
        'attributes' => array(
          'class' => array(
              'revert',
              'action',
          ),
        ),
        'query' => backdrop_get_destination(),
      );
    }
    $operations['export'] = array(
      'title' => t('Export'),
      'href' => RulesPluginUI::path($name, 'export'),
      'attributes' => array(
        'class' => array(
            'export',
            'action',
        ),
      ),
    );
    $row['operations'] = array(
      'data' => array(
        '#type' => 'dropbutton',
        '#links' => $operations,
      ),
    );
    return $row;
  }

}
