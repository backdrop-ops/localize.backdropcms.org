<?php

/**
 * @file
 * Action to set the state of a user in a group.
 */

/**
 * Action info for removing a user from a group.
 */
function og_membership_delete_action_info() {
  return array('og_membership_delete_action' => array(
      'type' => 'og_membership',
      'label' => t('Remove from group'),
    ));
}

/**
 * Action callback for removing a user from a group
 * 
 * @param OgMembership $og_membership
 *   An OgMembership entity
 * @param array $context
 *   The context variable.
 */
function og_membership_delete_action($og_membership, &$context) {
  // Save the list of og_memberships to be deleted in the session.
  // Append to the existing list if within the last minute, otherwise start a
  // new list.
  $last_action_time = 0;
  if (isset($_SESSION['og_membership_delete_action'])) {
    $last_action_time = $_SESSION['og_membership_delete_action']['timestamp'];
  }
  if (REQUEST_TIME - $last_action_time > 60) {
    $_SESSION['og_membership_delete_action'] = array(
      'timestamp' => REQUEST_TIME,
      'ids' => array(),
    );
  }

  $_SESSION['og_membership_delete_action']['ids'][] = $og_membership->id;
  $context['redirect'] = "group/{$og_membership->group_type}/{$og_membership->gid}/admin/people/delete-membership/action";
}

/** 
 * Confirm form to delete membership in bulk
 */
function og_ui_delete_membership_bulk_form($form, &$form_state) {
  if (isset($form_state['og_membership_ids'])) {
    $og_membership_ids = $form_state['og_memberehips_ids'];
  }
  elseif (isset($_SESSION['og_membership_delete_action']['timestamp']) && (REQUEST_TIME - $_SESSION['og_membership_delete_action']['timestamp'] < 6000)) {
    $og_membership_ids = $_SESSION['og_membership_delete_action']['ids'];
    $form_state['og_membership_ids'] = $og_membership_ids;
    $form_state['cache'] = TRUE;
    unset($_SESSION['og_membership_delete_action']);
  }
  else {
    $og_membership_ids = array();
  }

  $form['#tree'] = TRUE;

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
  );

  $og_memberships = og_membership_load_multiple($og_membership_ids);

  foreach ($og_memberships as $og_membership) {
    $user = user_load($og_membership->etid);
    $form['og_membership_list']['#items'][] = check_plain($user->name);
  }

  $confirm_question = format_plural(count($og_membership_ids), 'Are you sure you want to remove this user?', 'Are you sure you want to remove these users?');
  return confirm_form($form, $confirm_question, '<front>', t('This action cannot be undone.'), t('Remove'), t('Cancel'));
}

/** 
 * Submit handler for bulk user removal.
 */
function og_ui_delete_membership_bulk_form_submit($form, &$form_state) {
  og_membership_delete_multiple($form_state['og_membership_ids']);
  // cache_clear_all();
  $count = count($form_state['og_membership_ids']);

  backdrop_set_message(format_plural($count, 'Removed 1 user.', 'Removed @count users.'));
  $form_state['redirect'] = '<front>';
}
