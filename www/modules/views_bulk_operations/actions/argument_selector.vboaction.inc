<?php

/**
 * @file
 * Passes selected ids as arguments to a page.
 * The ids might be entity ids or revision ids, depending on the type of the
 * VBO field.
 */

/**
 * Implementation of hook_vboaction_info().
 */
function views_bulk_operations_argument_selector_vboaction_info() {
  return array(
    'views_bulk_operations_argument_selector_vboaction' => array(
      'label' => t('Pass ids as arguments to a page'),
      'type' => 'entity',
      'aggregate' => TRUE,
      'configurable' => FALSE,
      //'hooks' => array(),
      'vbo' => TRUE,
      //'triggers' => array('any'),
    ),
  );
}

/**
* Implementation of a Drupal action.
* Passes selected ids as arguments to a view.
*/
function views_bulk_operations_argument_selector_vboaction($entities, $context = array()) {
  $base_url = $context['settings']['url'];
  $arguments = implode(',', array_keys($entities));
  // Add a trailing slash if missing.
  if (substr($base_url, -1, 1) != '/') {
    $base_url .= '/';
  }
  backdrop_goto($base_url . $arguments);
}

function views_bulk_operations_argument_selector_vboaction_views_bulk_operations_form($options) {
  $form['url'] = array(
    '#title' => t('URL'),
    '#type' => 'textfield',
    '#description' => t('Enter a URL that the user will be sent to. A comma-separated list of selected ids will be appended.'),
    '#default_value' => isset($options['url']) ? $options['url'] : '',
    '#field_prefix' => url(NULL, array('absolute' => TRUE)) . (config_get('system_core', 'clean_rul') ? '' : '?q='),
  );
  return $form;
}
