<?php

/**
 * @file
 * Provides support for core actions.
 */

/**
 * Returns a prepared list of available actions.
 *
 * Actions are fetched by invoking actions_get_info().
 *
 * @param $operation_id
 *   The full, prefixed operation_id of the operation (in this case, action)
 *   to return, or NULL to return an array with all operations.
 */
function views_bulk_operations_operation_action_list($operation_id = NULL) {
  $operations = &backdrop_static(__FUNCTION__);

  if (!isset($operations)) {
    // Combined list of all actions and advanced actions.
    $actions_list = views_bulk_operations_actions_get_info();
    // Actions aren't usable in a VBO context. Add as needed
    $hidden_actions = array();

    $operations = array();
    foreach ($actions_list as $key => $action) {
      // Actions are keyed by callback.
      // Advanced actions are keyed by aid and store the callback separately.
      $callback = isset($action['callback']) ? $action['callback'] : $key;
      // This action needs to be skipped.
      if (in_array($callback, $hidden_actions)) {
        continue;
      }

      // All operations must be prefixed with the operation type.
      $new_operation_id = 'action::' . $key;

      $operations[$new_operation_id] = array(
        'operation_type' => 'action',
        'type' => $action['type'],
        // Keep the unprefixed key around, for internal use.
        'key' => $key,
        'callback' => $callback,
        'label' => isset($action['label']) ? $action['label'] : '',
        'parameters' => isset($action['parameters']) ? $action['parameters'] : array(),
        'configurable' => !empty($action['configurable']) || !empty($action['vbo_configurable']),
        'aggregate' => !empty($action['aggregate']),
        'behavior' => isset($action['behavior']) ? $action['behavior'] : array(),
        'permissions' => isset($action['permissions']) ? $action['permissions'] : NULL,
        'pass rows' => !empty($action['pass rows']),
        'file' => !empty($action['file']) ? $action['file'] : NULL, 
        'vbo' => !empty($action['vbo']) ? $action['vbo'] : NULL,
      );
    }
  }

  if (isset($operation_id)) {
    return isset($operations[$operation_id]) ? $operations[$operation_id] : FALSE;
  }
  else {
    return $operations;
  }
}

/**
 * Get a list of advanced actions (created through the Action UI).
 * Backdrop does not provide Action UI
 */
// function views_bulk_operations_operation_advanced_action_list() {
//   $actions = array();
//   $static_actions = actions_get_info();
//   $result = db_query("SELECT * FROM {actions} WHERE parameters > ''");
//   foreach ($result as $action) {
//     $parameters = unserialize($action->parameters);
//     $actions[$action->aid] = array(
//       'label' => isset($action->label) ? $action->label : '',
//       'callback' => $action->callback,
//       'type' => $action->type,
//       'configurable' => FALSE,
//       'parameters' => $parameters,
//     );
//     foreach (array('aggregate', 'behavior', 'permissions', 'pass rows') as $attribute) {
//       if (isset($static_actions[$action->callback][$attribute])) {
//         $actions[$action->aid][$attribute] = $static_actions[$action->callback][$attribute];
//       }
//     }
//     if (isset($static_actions[$action->callback]['parameters'])) {
//       $actions[$action->aid]['parameters'] = array_merge($actions[$action->aid]['parameters'], $static_actions[$action->callback]['parameters']);
//     }
//   }
//   return $actions;
// }
