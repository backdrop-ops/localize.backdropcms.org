<?php
/**
 * @file
 * Plugin to provide access control based upon OG group status.
 */

class OgMemberLayoutAccess extends LayoutAccessNegatable {

  /**
   * Constructor for a Layout membership in group rule.
   */
  function __construct($plugin_name, array $data = array()) {
    parent::__construct($plugin_name, $data);
    $this->settings += array(
      'state' => array(),
    );
  }

  /**
   * {@inheritdoc}
   */
  function form(&$form, &$form_state) {
    parent::form($form, $form_state);
    $form['state'] = array(
      '#type' => 'select',
      '#options' => og_group_content_states(),
      '#title' => t('State in group'),
      '#multiple' => TRUE,
      '#default_value' => $this->settings['state'],
      '#description' => t('Only users with the specified state in group will be able to access this.'),
      '#required' => TRUE,
    );
  
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  function checkAccess() {
    $user_context = $this->contexts['user'];
    $node_context = $this->contexts['node'];
    if (empty($user_context) || empty($user_context->data)) {
      return;
    }

    if (empty($node_context) || empty($node_context->data)) {
      return;
    }

    $account = clone $user_context->data;
    $node = $node_context->data;
    if ($this->settings['negate']) {
      return !og_is_member('node', $node->nid, 'user', $account, $this->settings['state']);
    }
    else {
      return og_is_member('node', $node->nid, 'user', $account, $this->settings['state']);
    }
  }

  /**
   * {@inheritdoc}
   */
  function summary() {
    if (empty($this->settings['state'])) {
      return t('Error, unset state');
    }

    $states = og_group_content_states();
    $state = array();
    foreach ($this->settings['state'] as $value) {
      $state[] = $states[$value];
    }
  
    return $this->settings['negate'] ? t('User does not have "@state" state in group.', array('@state' => implode(', ', $state),)) : t('User has "@state" state in group.', array('@state' => implode(', ', $state),));
  }
}
