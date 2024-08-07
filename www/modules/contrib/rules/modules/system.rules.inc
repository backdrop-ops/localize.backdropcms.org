<?php
/**
 * @file
 * Rules integration for the system module.
 *
 * @addtogroup rules
 *
 * @{
 */

/**
 * Implements hook_rules_file_info() on behalf of the system module.
 */
function rules_system_file_info() {
  return array('modules/system.eval');
}

/**
 * Implements hook_rules_event_info() on behalf of the system module.
 */
function rules_system_event_info() {
  return array(
    'init' => array(
      'label' => t('Backdrop is initializing'),
      'group' => t('System'),
      'help' => t("Be aware that some actions might initialize the theme system. After that, it's impossible for any module to change the used theme."),
      'access callback' => 'rules_system_integration_access',
    ),
    'cron' => array(
      'label' => t('Cron maintenance tasks are performed'),
      'group' => t('System'),
      'access callback' => 'rules_system_integration_access',
    ),
    'watchdog' => array(
      'label' => t('System log entry is created'),
      'variables' => array(
        'log_entry' => array(
          'type' => 'log_entry',
          'label' => t('Log entry'),
        ),
      ),
      'group' => t('System'),
      'access callback' => 'rules_system_integration_access',
    ),
  );
}

/**
 * Implements hook_rules_data_info() on behalf of the system module.
 *
 * @see rules_core_modules()
 */
function rules_system_data_info() {
  return array(
    'log_entry' => array(
      'label' => t('Watchdog log entry'),
      'wrap' => TRUE,
      'property info' => _rules_system_watchdog_log_entry_info(),
    ),
  );
}

/**
 * Defines property info for watchdog log entries.
 *
 * Used by the log entry data type to provide a useful metadata wrapper.
 */
function _rules_system_watchdog_log_entry_info() {
  return array(
    'type' => array(
      'type' => 'text',
      'label' => t('The category to which this message belongs'),
    ),
    'message' => array(
      'type' => 'text',
      'label' => t('Log message'),
      'getter callback' => 'rules_system_log_get_message',
      'sanitized' => TRUE,
    ),
    'severity' => array(
      'type' => 'integer',
      'label' => t('Severity'),
      'options list' => 'watchdog_severity_levels',
    ),
    'request_uri' => array(
      'type' => 'uri',
      'label' => t('Request uri'),
    ),
    'link' => array(
      'type' => 'text',
      'label' => t('An associated, HTML formatted link'),
    ),
  );
}

/**
 * Implements hook_rules_action_info() on behalf of the system module.
 */
function rules_system_action_info() {
  return array(
    'backdrop_message' => array(
      'label' => t('Show a message on the site'),
      'group' => t('System'),
      'parameter' => array(
        'message' => array(
          'type' => 'text',
          'label' => t('Message'),
          'sanitize' => TRUE,
          'translatable' => TRUE,
        ),
        'type' => array(
          'type' => 'token',
          'label' => t('Message type'),
          'options list' => 'rules_action_backdrop_message_types',
          'default value' => 'info',
          'optional' => TRUE,
        ),
        'repeat' => array(
          'type' => 'boolean',
          'label' => t('Repeat message'),
          'description' => t("If set to 'False' and the message has been already shown, then the message won't be repeated."),
          'default value' => TRUE,
          'optional' => TRUE,
          'restriction' => 'input',
        ),
      ),
      'base' => 'rules_action_backdrop_message',
      'access callback' => 'rules_system_integration_access',
    ),
    'backdrop_watchdog' => array(
      'label' => t('Logs a message to the system dblog'),
      'group' => t('System'),
      'parameter' => array(
        'type' => array(
          'type' => 'text',
          'label' => t('Type'),
          'description' => t('The category to which this message belongs, displayed in the first column of the dblog.'),
        ),
        'message' => array(
          'type' => 'text',
          'label' => t('Message'),
          'description' => t('The text of the message.'),
          'translatable' => TRUE,
        ),
        'severity' => array(
          'type' => 'token',
          'label' => t('Severity'),
          'options list' => 'watchdog_severity_levels',
          'default value' => WATCHDOG_NOTICE,
          'optional' => TRUE,
        ),
        'link_text' => array(
          'type' => 'text',
          'label' => t('Link text'),
          'description' => t('The text to display for the link (optional)'),
          'optional' => TRUE,
        ),
        'link_path' => array(
          'type' => 'uri',
          'label' => t('Link path'),
          'description' => t('A Backdrop path or path alias. Enter (optional) queries after "?" and (optional) anchor after "#".'),
          'optional' => TRUE,
        ),
      ),
      'base' => 'rules_action_backdrop_watchdog',
      'access callback' => 'rules_system_integration_access',
    ),
    'redirect' => array(
      'label' => t('Page redirect'),
      'group' => t('System'),
      'parameter' => array(
        'url' => array(
          'type' => 'uri',
          'label' => t('URL'),
          'description' => t('A Backdrop path, path alias, or external URL to redirect to. Enter (optional) queries after "?" and (optional) anchor after "#".'),
        ),
        'force' => array(
          'type' => 'boolean',
          'label' => t('Force redirect'),
          'restriction' => 'input',
          'description' => t("Force the redirect even if another destination parameter is present. Per default Backdrop would redirect to the path given as destination parameter, in case it is set. Usually the destination parameter is set by appending it to the URL, e.g. !example_url", array('!example_url' => 'http://example.com/user/login?destination=node/2')),
          'optional' => TRUE,
          'default value' => TRUE,
        ),
        'destination' => array(
          'type' => 'boolean',
          'label' => t('Append destination parameter'),
          'restriction' => 'input',
          'description' => t('Whether to append a destination parameter to the URL, so another redirect issued later on would lead back to the origin page.'),
          'optional' => TRUE,
          'default value' => FALSE,
        ),
      ),
      'base' => 'rules_action_backdrop_goto',
      'access callback' => 'rules_system_integration_access',
    ),
    'breadcrumb_set' => array(
      'label' => t('Set breadcrumb'),
      'group' => t('System'),
      'parameter' => array(
        'titles' => array(
          'type' => 'list<text>',
          'label' => t('Titles'),
          'description' => t('A list of titles for the breadcrumb links.'),
          'translatable' => TRUE,
        ),
        'paths' => array(
          'type' => 'list<text>',
          'label' => t('Paths'),
          'description' => t('A list of Backdrop paths for the breadcrumb links, matching the order of the titles.'),
        ),
      ),
      'base' => 'rules_action_breadcrumb_set',
      'access callback' => 'rules_system_integration_access',
    ),
    'mail' => array(
      'label' => t('Send mail'),
      'group' => t('System'),
      'parameter' => array(
        'to' => array(
          'type' => 'text',
          'label' => t('To'),
          'description' => t('The e-mail address or addresses where the message will be sent to. The formatting of this string must comply with <a href="http://www.faqs.org/rfcs/rfc2822.html">RFC 2822</a>. Some examples are:') . theme('item_list', array('items' => array(
            t('user@example.com'),
            t('user@example.com, anotheruser@example.com'),
            t('User &lt;user@example.com&gt;'),
            t('User &lt;user@example.com&gt;, Another User &lt;anotheruser@example.com&gt;'),
          ))),
        ),
        'cc' => array(
          'type' => 'text',
          'label' => t('CC'),
          'description' => t("CC recipient(s) (same form as To)"),
          'translatable' => TRUE,
          'optional' => TRUE,
          'allow null' => TRUE,
        ),
        'bcc' => array(
          'type' => 'text',
          'label' => t('BCC'),
          'description' => t("BCC recipient(s) (same form as To)"),
          'translatable' => TRUE,
          'optional' => TRUE,
          'allow null' => TRUE,
        ),
        'reply_to' => array(
          'type' => 'text',
          'label' => t('Reply-to'),
          'description' => t('Reply-to address (same form as To)'),
          'translatable' => TRUE,
          'optional' => TRUE,
          'allow null' => TRUE,
        ),
        'subject' => array(
          'type' => 'text',
          'label' => t('Subject'),
          'description' => t("The mail's subject."),
          'translatable' => TRUE,
        ),
        'message' => array(
          'type' => 'text',
          'label' => t('Message'),
          'description' => t("The mail's message body."),
          'translatable' => TRUE,
        ),
        'from' => array(
          'type' => 'text',
          'label' => t('From'),
          'description' => t("The mail's from address. Leave it empty to use the site-wide configured address."),
          'optional' => TRUE,
        ),
        'langcode' => array(
          'type' => 'token',
          'label' => t('Language'),
          'description' => t('If specified, the language used for getting the mail message and subject.'),
          'options list' => 'entity_plus_metadata_language_list',
          'optional' => TRUE,
          'default value' => LANGUAGE_NONE,
          'default mode' => 'selector',
        ),
      ),
      'base' => 'rules_action_mail',
      'access callback' => 'rules_system_integration_access',
    ),
    'mail_to_users_of_role' => array(
      'label' => t('Send mail to all users of a role'),
      'group' => t('System'),
      'parameter' => array(
        'roles' => array(
          'type' => 'list<integer>',
          'label' => t('Roles'),
          'options list' => 'entity_plus_metadata_user_roles',
          'description' => t('Select the roles whose users should receive the mail.'),
        ),
        'cc' => array(
          'type' => 'text',
          'label' => t('CC'),
          'description' => t("CC recipient(s) (same form as To)"),
          'translatable' => TRUE,
          'optional' => TRUE,
          'allow null' => TRUE,
        ),
        'bcc' => array(
          'type' => 'text',
          'label' => t('BCC'),
          'description' => t("BCC recipient(s) (same form as To)"),
          'translatable' => TRUE,
          'optional' => TRUE,
          'allow null' => TRUE,
        ),
        'reply_to' => array(
          'type' => 'text',
          'label' => t('Reply-to'),
          'description' => t('Reply-to address (same form as To)'),
          'translatable' => TRUE,
          'optional' => TRUE,
          'allow null' => TRUE,
        ),
        'subject' => array(
          'type' => 'text',
          'label' => t('Subject'),
          'description' => t("The mail's subject."),
        ),
        'message' => array(
          'type' => 'text',
          'label' => t('Message'),
          'description' => t("The mail's message body."),
        ),
        'from' => array(
          'type' => 'text',
          'label' => t('From'),
          'description' => t("The mail's from address. Leave it empty to use the site-wide configured address."),
          'optional' => TRUE,
        ),
      ),
      'base' => 'rules_action_mail_to_users_of_role',
      'access callback' => 'rules_system_integration_access',
    ),
    'block_ip' => array(
      'label' => t('Block IP address'),
      'group' => t('System'),
      'parameter' => array(
        'ip_address' => array(
          'type' => 'ip_address',
          'label' => t('IP address'),
          'description' => t('If not provided, the IP address of the current user will be used.'),
          'optional' => TRUE,
          'default value' => NULL,
        ),
      ),
      'base' => 'rules_action_block_ip',
      'access callback' => 'rules_system_integration_access',
    ),
  );
}

/**
 * Help callback for the "Send mail to users of a role" action.
 */
function rules_action_mail_to_users_of_role_help() {
  return t('WARNING: This may cause problems if there are too many users of these roles on your site, as your server may not be able to handle all the mail requests all at once.');
}

/**
 * System integration access callback.
 */
function rules_system_integration_access($type, $name) {
  return user_access('administer site configuration');
}

/**
 * Options list callback defining backdrop_message types.
 */
function rules_action_backdrop_message_types() {
  return array(
    'info'       => t('Info'),
    'status'     => t('Status'),
    'error'      => t('Error'),
    'warning'    => t('Warning'),
  );
}

/**
 * Implements hook_rules_evaluator_info() on behalf of the system module.
 */
function rules_system_evaluator_info() {
  return array(
    'token' => array(
      'class' => 'RulesTokenEvaluator',
      'type' => array('text', 'uri', 'list<text>', 'list<uri>'),
      'weight' => 0,
    ),
  );
}

/**
 * @} End of "addtogroup rules"
 */
