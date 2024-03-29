<?php
/**
 * @file
 * Rules integration for the user module.
 *
 * @addtogroup rules
 *
 * @{
 */

/**
 * Implements hook_rules_file_info() on behalf of the user module.
 */
function rules_user_file_info() {
  return array('modules/user.eval');
}

/**
 * Implements hook_rules_event_info().
 */
function rules_user_event_info() {
  return array(
    'user_insert' => array(
      'label' => t('After saving a new user account'),
      'group' => t('User account'),
      'variables' => array(
        'account' => array('type' => 'user', 'label' => t('registered user account')),
      ),
      'access callback' => 'rules_user_integration_access',
    ),
    'user_update' => array(
      'label' => t('After updating an existing user account'),
      'group' => t('User account'),
      'variables' => array(
        'account' => array('type' => 'user', 'label' => t('updated user account')),
        'account_unchanged' => array('type' => 'user', 'label' => t('unchanged user account'), 'handler' => 'rules_events_entity_unchanged'),
      ),
      'access callback' => 'rules_user_integration_access',
    ),
    'user_presave' => array(
      'label' => t('Before saving a user account'),
      'group' => t('User account'),
      'variables' => array(
        'account' => array('type' => 'user', 'label' => t('saved user account'), 'skip save' => TRUE),
        'account_unchanged' => array('type' => 'user', 'label' => t('unchanged user account'), 'handler' => 'rules_events_entity_unchanged'),
      ),
      'access callback' => 'rules_user_integration_access',
    ),
    'user_view' => array(
      'label' => t('User account page is viewed'),
      'group' => t('User account'),
      'variables' => array(
        'account' => array('type' => 'user', 'label' => t('viewed user account')),
        'view_mode' => array(
          'type' => 'text',
          'label' => t('view mode'),
          'options list' => 'rules_get_entity_view_modes',
          // Add the entity-type for the options list callback.
          'options list entity type' => 'user',
        ),
      ),
      'access callback' => 'rules_user_integration_access',
      'help' => t("Note that if backdrop's page cache is enabled, this event won't be generated for pages served from cache."),
    ),
    'user_delete' => array(
      'label' => t('After a user account has been deleted'),
      'group' => t('User account'),
      'variables' => array(
        'account' => array('type' => 'user', 'label' => t('deleted user account')),
      ),
      'access callback' => 'rules_user_integration_access',
    ),
    'user_login' => array(
      'label' => t('User has logged in'),
      'group' => t('User account'),
      'variables' => array(
        'account' => array('type' => 'user', 'label' => t('logged in user')),
      ),
      'access callback' => 'rules_user_integration_access',
    ),
    'user_logout' => array(
      'label' => t('User has logged out'),
      'group' => t('User account'),
      'variables' => array(
        'account' => array('type' => 'user', 'label' => t('logged out user account')),
      ),
      'access callback' => 'rules_user_integration_access',
    ),
  );
}

/**
 * Options list for user cancel methods.
 *
 * @todo Use for providing a user_cancel action.
 */
function rules_user_cancel_methods() {
  module_load_include('inc', 'user', 'user.pages');
  foreach (user_cancel_methods() as $method => $form) {
    $methods[$method] = $form['#title'];
  }
  return $methods;
}

/**
 * User integration access callback.
 */
function rules_user_integration_access($type, $name) {
  if ($type == 'event' || $type == 'condition') {
    return entity_plus_access('view', 'user');
  }
  // Else return admin access.
  return user_access('administer users');
}

/**
 * Implements hook_rules_condition_info() on behalf of the user module.
 */
function rules_user_condition_info() {
  return array(
    'user_has_role' => array(
      'label' => t('User account has role(s)'),
      'parameter' => array(
        'account' => array('type' => 'user', 'label' => t('User account')),
        'roles' => array(
          'type' => 'list<integer>',
          'label' => t('Roles'),
          'options list' => 'rules_user_roles_options_list',
        ),
        'operation' => array(
          'type' => 'text',
          'label' => t('Match roles'),
          'options list' => 'rules_user_condition_operations',
          'restriction' => 'input',
          'optional' => TRUE,
          'default value' => 'AND',
          'description' => t('If matching against all selected roles, the user account must have <em>all</em> the roles selected.'),
        ),
      ),
      'group' => t('User account'),
      'access callback' => 'rules_user_integration_access',
      'base' => 'rules_condition_user_has_role',
    ),
    'user_is_blocked' => array(
      'label' => t('User account is blocked'),
      'parameter' => array(
        'account' => array('type' => 'user', 'label' => t('User account')),
      ),
      'group' => t('User account'),
      'access callback' => 'rules_user_integration_access',
      'base' => 'rules_condition_user_is_blocked',
    ),
  );
}

/**
 * User account has role condition help callback.
 */
function rules_condition_user_has_role_help() {
  return t('Whether the user account has the selected role(s).');
}

/**
 * Options list callback for the operation parameter of condition user has role.
 */
function rules_user_condition_operations() {
  return array(
    'AND' => t('all'),
    'OR' => t('any'),
  );
}

/**
 * Implements hook_rules_action_info() on behalf of the user module.
 */
function rules_user_action_info() {
  $defaults = array(
    'parameter' => array(
      'account' => array(
        'type' => 'user',
        'label' => t('User account'),
        'description' => t('The user account whose roles should be changed.'),
        'save' => TRUE,
      ),
      'roles' => array(
        'type' => 'list<integer>',
        'label' => t('Roles'),
        'options list' => 'rules_user_roles_options_list',
      ),
    ),
    'group' => t('User account'),
    'access callback' => 'rules_user_role_change_access',
  );
  $items['user_add_role'] = $defaults + array(
    'label' => t('Add user account role'),
    'base' => 'rules_action_user_add_role',
  );
  $items['user_remove_role'] = $defaults + array(
    'label' => t('Remove user account role'),
    'base' => 'rules_action_user_remove_role',
  );
  $defaults = array(
    'parameter' => array(
      'account' => array(
        'type' => 'user',
        'label' => t('User account'),
        'save' => TRUE,
      ),
    ),
    'group' => t('User account'),
    'access callback' => 'rules_user_integration_access',
  );
  $items['user_block'] = $defaults + array(
    'label' => t('Block a user account'),
    'base' => 'rules_action_user_block',
  );
  $items['user_unblock'] = $defaults + array(
    'label' => t('Unblock a user account'),
    'base' => 'rules_action_user_unblock',
  );
  $items['user_send_account_email'] = array(
    'label' => t('Send user account e-mail'),
    'parameter' => array(
      'account' => array(
        'type' => 'user',
        'label' => t('User account'),
      ),
      'email_type' => array(
        'type' => 'text',
        'label' => t('E-mail type'),
        'description' => t("Select the e-mail based on your site's account settings to send to the user account."),
        'options list' => 'rules_user_account_email_options_list',
      ),
    ),
    'group' => t('User account'),
    'base' => 'rules_action_user_send_account_email',
    'access callback' => 'rules_user_integration_access',
  );
  return $items;
}

/**
 * User account integration role actions access callback.
 */
function rules_user_role_change_access() {
  return entity_plus_metadata_user_roles() && user_access('administer permissions');
}

/**
 * Options list callback for user account roles.
 */
function rules_user_roles_options_list($element) {
  return entity_plus_metadata_user_roles('roles', array(), $element instanceof RulesConditionInterface ? 'view' : 'edit');
}

/**
 * Options list callback for user account e-mail types.
 *
 * @see _user_mail_notify()
 */
function rules_user_account_email_options_list() {
  return array(
    'register_admin_created' => t('Welcome (new user account created by administrator)'),
    'register_no_approval_required' => t('Welcome (no approval required)'),
    'register_pending_approval' => t('Welcome (awaiting approval)'),
    'password_reset' => t('Password recovery'),
    'status_activated' => t('Account activation'),
    'status_blocked' => t('Account blocked'),
    'cancel_confirm' => t('Account cancellation confirmation'),
    'status_canceled' => t('Account canceled'),
  );
}

/**
 * @} End of "addtogroup rules"
 */
