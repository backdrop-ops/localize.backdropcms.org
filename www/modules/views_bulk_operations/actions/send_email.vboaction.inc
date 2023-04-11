<?php

/**
 * @file
 * Implements a message display action.
 */

/**
 * Implements hook_vboaction_info().
 */
function views_bulk_operations_send_email_vboaction_info() {
  return array(
    'views_bulk_operations_send_email_vboaction' => array(
      'type' => 'system',
      'label' => t('Send e-mail'),
      'configurable' => TRUE,
      // 'triggers' => array('any'),
      'vbo' => TRUE,
    ),
  );
}

/**
 * Return a form definition so the Send email action can be configured.
 *
 * @param $context
 *   Default values (if we are editing an existing action instance).
 *
 * @return
 *   Form definition.
 */
function views_bulk_operations_send_email_vboaction_form($context, $form_state) {
  // Set default values for form.
  if (!isset($context['recipient'])) {
    $context['recipient'] = '';
  }
  if (!isset($context['subject'])) {
    $context['subject'] = '';
  }
  if (!isset($context['message'])) {
    $context['message'] = '';
  }

  $form['recipient'] = array(
    '#type' => 'textfield',
    '#title' => t('Recipient'),
    '#default_value' => $context['recipient'],
    '#maxlength' => '254',
    '#description' => t('The email address to which the message should be sent OR enter [node:author:mail], [comment:author:mail], etc. if you would like to send an e-mail to the author of the original post.'),
  );
  $form['subject'] = array(
    '#type' => 'textfield',
    '#title' => t('Subject'),
    '#default_value' => $context['subject'],
    '#maxlength' => '254',
    '#description' => t('The subject of the message.'),
  );
  $form['message'] = array(
    '#type' => 'textarea',
    '#title' => t('Message'),
    '#default_value' => $context['message'],
    '#cols' => '80',
    '#rows' => '20',
    '#description' => t('The message that should be sent. You may include placeholders like [node:title], [user:name], and [comment:body] to represent data that will be different each time message is sent. Not all placeholders will be available in all contexts.'),
  );
  return $form;
}

/**
 * Validate views_bulk_operations_send_email_vboaction form submissions.
 */
function views_bulk_operations_send_email_vboaction_validate($form, $form_state) {
  $form_values = $form_state['values'];
  // Validate the configuration form.
  if (!valid_email_address($form_values['recipient']) && strpos($form_values['recipient'], ':mail') === FALSE) {
    // We want the literal %author placeholder to be emphasized in the error message.
    form_set_error('recipient', t('Enter a valid email address or use a token e-mail address such as %author.', array('%author' => '[node:author:mail]')));
  }
}

/**
 * Process views_bulk_operations_send_email_vboaction form submissions.
 */
function views_bulk_operations_send_email_vboaction_submit($form, $form_state) {
  $form_values = $form_state['values'];
  // Process the HTML form to store configuration. The keyed array that
  // we return will be serialized to the database.
  $params = array(
    'recipient' => $form_values['recipient'],
    'subject'   => $form_values['subject'],
    'message'   => $form_values['message'],
  );
  return $params;
}

/**
 * Sends an e-mail message.
 *
 * @param object $entity
 *   An optional node object, which will be added as $context['node'] if
 *   provided.
 * @param array $context
 *   Array with the following elements:
 *   - 'recipient': E-mail message recipient. This will be passed through
 *     token_replace().
 *   - 'subject': The subject of the message. This will be passed through
 *     token_replace().
 *   - 'message': The message to send. This will be passed through
 *     token_replace().
 *   - Other elements will be used as the data for token replacement.
 *
 */
function views_bulk_operations_send_email_vboaction($entity, $context) {
  if (empty($context['node'])) {
    $context['node'] = $entity;
  }

  $recipient = token_replace($context['recipient'], $context);

  // If the recipient is a registered user with a language preference, use
  // the recipient's preferred language. Otherwise, use the system default
  // language.
  $recipient_account = user_load_by_mail($recipient);
  if ($recipient_account) {
    $language = user_preferred_language($recipient_account);
  }
  else {
    $language = language_default();
  }
  $params = array('context' => $context);

  if (backdrop_mail('system', 'action_send_email', $recipient, $language, $params)) {
    watchdog('action', 'Sent email to %recipient', array('%recipient' => $recipient));
  }
  else {
    watchdog('error', 'Unable to send email to %recipient', array('%recipient' => $recipient));
  }
}
