<?php

/**
 * @file
 * Action to set the state of a user in a group.
 */

/**
 * Action info for modifying membership status of a user in a group.
 */
function og_set_state_action_info() {
  return array('og_set_state_action' => array(
      'type' => 'og_membership',
      'label' => t('Modify membership status'),
    ));
}

/**
 * Action callback.
 * 
 * Cache the selected entity ids and redirect to the configuration form.
 * 
 * @param OgMembership $og_membership
 *   The membership entity.
 * 
 * @param array $context
 *   The context passed by action_execute().
 */
function og_set_state_action($og_membership, &$context) {
  // Save the list of og_memberships to be deleted in the session.
  // Append to the existing list if within the last minute, otherwise start a
  // new list.
  $last_action_time = 0;
  if (isset($_SESSION['og_set_state_action'])) {
    $last_action_time = $_SESSION['og_set_state_action']['timestamp'];
  }
  if (REQUEST_TIME - $last_action_time > 60) {
    $_SESSION['og_set_state_action'] = array(
      'timestamp' => REQUEST_TIME,
      'ids' => array(),
    );
  }

  $_SESSION['og_set_state_action']['ids'][] = $og_membership->id;
  $context['redirect'] = "group/{$og_membership->group_type}/{$og_membership->gid}/admin/people/membership-set-state/action";
}

/**
 * Configuration form for the action.
 */
function og_set_state_action_form($form, &$form_state) {
  if (isset($form_state['og_membership_ids'])) {
    $og_membership_ids = $form_state['og_memberehips_ids'];
  }
  elseif (isset($_SESSION['og_set_state_action']['timestamp']) && (REQUEST_TIME - $_SESSION['og_set_state_action']['timestamp'] < 6000)) {
    $og_membership_ids = $_SESSION['og_set_state_action']['ids'];
    $form_state['og_membership_ids'] = $og_membership_ids;
    $form_state['cache'] = TRUE;
    unset($_SESSION['og_set_state_action']);
  }
  else {
    $og_membership_ids = array();
  }

  if (empty($og_membership_ids)) {
    $destination = isset($_GET['destination']) ? $_GET['destination'] : '<front>';
    if (isset($_GET['destination'])) {
      $markup = t('<a href="!url">Go back</a>.', array('!url' => url($_GET['destination'])));
    } else {
      $markup = t('Return to the <a href="!url">front page</a>.', array('!url' => url('<front>')));
    }
    $form['empty']['#markup'] = '<p>' . $markup . '</p>';
    backdrop_set_message(t('No users have been selected for removal.'), 'error');
    return $form;
  }

  $form['og_membership_list'] = array(
    '#theme' => 'item_list',
    '#items' => array(),
    '#title' => 'You selected the following items:',
  );

  $og_memberships = og_membership_load_multiple($og_membership_ids);

  foreach ($og_memberships as $og_membership) {
    $user = user_load($og_membership->etid);
    $form['og_membership_list']['#items'][] = check_plain($user->name);
  }

  $form['state'] = array(
    '#type' => 'select',
    '#title' => t('State'),
    '#description' => t('Choose the state to set for the selected users in the group.'),
    '#options' => og_group_content_states(),
    '#default_value' => OG_STATE_ACTIVE,
    '#required' => TRUE,
  );

  $form['actions'] = array(
    '#type' => 'actions',
  );

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Confirm'),
  );

  $form['actions']['cancel'] = array(
    '#markup' => t('<a href="!url">Cancel</a>', array('!url' => url($_GET['destination']))),
  );

  return $form;
}

/**
 * Submit handler.
 */
function og_set_state_action_form_submit($form, &$form_state) {
  $og_memberships = og_membership_load_multiple($form_state['og_membership_ids']);

  $state = $form_state['values']['state'];

  foreach ($og_memberships as $og_membership) {
    if ($og_membership->state == $state) {
      continue;
    }
  
    // Don't process the group manager, if exists.
    $group = entity_load($og_membership->group_type, $og_membership->gid);
    if (!empty($group->uid) && $group->uid == $og_membership->etid && $og_membership->entity_type == 'user') {
      continue;
    }
  
    $og_membership->state = $state;
    $og_membership->save();
  }
  
  $count = count($form_state['og_membership_ids']);

  backdrop_set_message(format_plural($count, 'Proccessed 1 user.', 'Processed @count users.'));
  $form_state['redirect'] = '<front>';
}
