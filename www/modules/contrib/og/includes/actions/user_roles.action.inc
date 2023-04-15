<?php

/**
 * Action info for modifying OG user roles in a group.
 */
function og_user_roles_action_info() {
  return array('og_user_roles_action' => array(
      'type' => 'og_membership',
      'label' => t('Modify OG user roles'),
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
function og_user_roles_action($og_membership, &$context) {
  // Save the list of og_memberships to be deleted in the session.
  // Append to the existing list if within the last minute, otherwise start a
  // new list.
  $last_action_time = 0;
  if (isset($_SESSION['og_user_roles_action'])) {
    $last_action_time = $_SESSION['og_user_roles_action']['timestamp'];
  }
  if (REQUEST_TIME - $last_action_time > 60) {
    $_SESSION['og_user_roles_action'] = array(
      'timestamp' => REQUEST_TIME,
      'ids' => array(),
    );
  }

  $_SESSION['og_user_roles_action']['ids'][] = $og_membership->id;
  $context['redirect'] = "group/{$og_membership->group_type}/{$og_membership->gid}/admin/people/membership-user-role/action";
}

/**
 * Configuration form for the action.
 */
function og_user_roles_action_form($form, &$form_state, $group_type, $gid) {
  if (isset($form_state['og_membership_ids'])) {
    $og_membership_ids = $form_state['og_memberehips_ids'];
  }
  elseif (isset($_SESSION['og_user_roles_action']['timestamp']) && (REQUEST_TIME - $_SESSION['og_user_roles_action']['timestamp'] < 6000)) {
    $og_membership_ids = $_SESSION['og_user_roles_action']['ids'];
    $form_state['og_membership_ids'] = $og_membership_ids;
    $form_state['cache'] = TRUE;
    unset($_SESSION['og_user_roles_action']);
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

  if (!entity_get_info($group_type)) {
    return;
  }

  $group = entity_load($group_type, $gid);
  if (!$group || !og_is_group($group_type, $group)) {
    return;
  }
  list(, , $bundle) = entity_extract_ids($group_type, $group);
  $og_roles = og_roles($group_type, $bundle, $gid, FALSE, FALSE);

  $form['add_roles'] = array(
    '#type' => 'select',
    '#multiple' => TRUE,
    '#title' => t('Add roles'),
    '#description' => t('Choose one or more roles you would like to assign to the selected users.'),
    '#options' => $og_roles,
    '#size' => 5,
  );
  $form['remove_roles'] = array(
    '#type' => 'select',
    '#multiple' => TRUE,
    '#title' => t('Remove roles'),
    '#description' => t('Choose one or more roles you would like to remove from the selected users.'),
    '#options' => $og_roles,
    '#size' => 5,
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
 * Form validation callback.
 */
function og_user_roles_action_form_validate($form, $form_state) {
  if (!$form_state['values']['add_roles'] && !$form_state['values']['remove_roles']) {
    form_set_error('add_roles', t('You have not chosen any role to add or remove. Please select something to do.'));
  }
}

/**
 * Submit handler.
 */
function og_user_roles_action_form_submit($form, &$form_state) {
  $og_memberships = og_membership_load_multiple($form_state['og_membership_ids']);

  $add_roles = array_filter($form_state['values']['add_roles']);
  $remove_roles = array_filter($form_state['values']['remove_roles']);

  foreach ($og_memberships as $og_membership) {
    if ($og_membership->entity_type != 'user') {
      return;
    }
  
    $group_type = $og_membership->group_type;
    $gid = $og_membership->gid;
  
    foreach ($add_roles as $rid) {
      og_role_grant($group_type, $gid, $og_membership->etid, $rid);
    }
  
    foreach ($remove_roles as $rid) {
      og_role_revoke($group_type, $gid, $og_membership->etid, $rid);
    }
    $og_membership->save();
  }
  
  $count = count($form_state['og_membership_ids']);

  backdrop_set_message(format_plural($count, 'Proccessed 1 user.', 'Processed @count users.'));
  $form_state['redirect'] = '<front>';
}
