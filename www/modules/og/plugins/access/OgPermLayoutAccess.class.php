<?php
/**
 * @file
 * Plugin to provide access control based upon OG group status.
 */

class OgPermLayoutAccess extends LayoutAccessNegatable {

  /**
   * Constructor for a Layout membership in group rule.
   */
  function __construct($plugin_name, array $data = array()) {
    parent::__construct($plugin_name, $data);
    $this->settings += array(
      'perm' => '',
    );
  }

  /**
   * {@inheritdoc}
   */
  function form(&$form, &$form_state) {
    parent::form($form, $form_state);
    $perms = array();
    // Get list of permissions
    foreach (og_get_permissions() as $perm => $value) {
      // By keeping them keyed by module we can use optgroups with the
      // 'select' type.
      $perms[$value['module']][$perm] = $value['title'];
    }
  
    $form['perm'] = array(
      '#type' => 'select',
      '#options' => $perms,
      '#title' => t('Group permission'),
      '#default_value' => $this->settings['perm'],
      '#description' => t('Only users with the selected permission flag, in the specified group, will be able to access this.'),
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
      return !og_user_access('node', $node->nid, $this->settings['perm'], $account);
    }
    else {    
      return og_user_access('node', $node->nid, $this->settings['perm'], $account);
    }
  }

  /**
   * {@inheritdoc}
   */
  function summary() {
    if (empty($this->settings['perm'])) {
      return t('Error, unset permission');
    }

    $permissions = og_get_permissions();
 
    return $this->settings['negate'] ? t('User does not have "@perm" permission in group.', array('@perm' => $permissions[$this->settings['perm']]['title'],)) : t('User has "@perm" permission in group.', array('@perm' => $permissions[$this->settings['perm']]['title']));
  }
}
