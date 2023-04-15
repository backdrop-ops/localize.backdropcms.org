<?php

/**
 * @file
 * Implements a message display action.
 */

/**
 * Implements hook_vboaction_info().
 */
function views_bulk_operations_goto_vboaction_info() {
  return array(
    'views_bulk_operations_goto_vboaction' => array(
      'type' => 'system',
      'label' => t('Redirect to URL'),
      'configurable' => TRUE,
      // 'triggers' => array('any'),
      'vbo' => TRUE,
      'aggregate' => TRUE,
    ),
  );
}

/**
 * Settings form for views_bulk_operations_goto_vboaction().
 */
function views_bulk_operations_goto_vboaction_form($context) {
  $form['url'] = array(
    '#type' => 'textfield',
    '#title' => t('URL'),
    '#description' => t('The URL to which the user should be redirected. This can be an internal path like node/1234 or an external URL like http://example.com.'),
    '#default_value' => isset($context['url']) ? $context['url'] : '',
    '#required' => TRUE,
  );
  return $form;
}

/** 
 * Submit handler.
 */
function views_bulk_operations_goto_vboaction_submit($form, $form_state) {
  return array(
    'url' => $form_state['values']['url']
  );
}

/**
 * Redirects to a different URL.
 *
 * @param $entity
 *   Ignored.
 * @param array $context
 *   Array with the following elements:
 *   - 'url': URL to redirect to. This will be passed through
 *     token_replace().
 *   - Other elements will be used as the data for token replacement.
 *
 */
function views_bulk_operations_goto_vboaction($entity, $context) {
  backdrop_goto(token_replace($context['url'], $context));
}