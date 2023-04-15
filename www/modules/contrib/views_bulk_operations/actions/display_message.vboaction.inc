<?php

/**
 * @file
 * Implements a message display action.
 */

/**
 * Implements hook_vboaction_info().
 */
function views_bulk_operations_display_message_vboaction_info() {
  return array(
    'views_bulk_operations_display_message_vboaction' => array(
      'type' => 'system',
      'label' => t('Display a message to the user'),
      'configurable' => TRUE,
      // 'triggers' => array('any'),
      'vbo' => TRUE,
    ),
  );
}

/**
 * Sends a message to the current user's screen.
 *
 * @param Entity $entity
 *   An optional node object, which will be added as $context['node'] if
 *   provided.
 * @param array $context
 *   Array with the following elements:
 *   - 'message': The message to send. This will be passed through
 *     token_replace().
 *   - Other elements will be used as the data for token replacement in
 *     the message.
 *
 */
function views_bulk_operations_display_message_vboaction(&$entity, $context = array()) {
  if (empty($context['node'])) {
    $context['node'] = $entity;
  }

  $context['message'] = token_replace(filter_xss_admin($context['message']), $context);
  backdrop_set_message($context['message']);
}

/**
 * Action form function.
 */
function views_bulk_operations_display_message_vboaction_form($context) {
  $form['message'] = array(
    '#type' => 'textarea',
    '#title' => t('Message'),
    '#default_value' => isset($context['message']) ? $context['message'] : '',
    '#required' => TRUE,
    '#rows' => '8',
    '#description' => t('The message to be displayed to the current user. You may include placeholders like [node:title], [user:name], and [comment:body] to represent data that will be different each time message is sent. Not all placeholders will be available in all contexts.'),
  );
  return $form;
}

/**
 * Action form submit function.
 *
 * Pass submitted message back to views_bulk_operations_change_owner.
 */
function views_bulk_operations_display_message_vboaction_submit($form, $form_state) {
  return array('message' => $form_state['values']['message']);
}
