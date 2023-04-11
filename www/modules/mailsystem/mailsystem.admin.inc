<?php

/**
 * @file
 * Administrative interface for the mail_system variable.
 */

/**
 * Form constructor for the mailsystem settings form.
 */
function mailsystem_admin_settings() {
  $args = array(
    '!hook_mail' => 'https://api.backdropcms.org/api/backdrop/core%21modules%21system%21system.api.php/function/hook_mail/1',
    '@mail' => 'mail()',
    '!default_class' => url('https://api.backdropcms.org/api/backdrop/core%21modules%21system%21system.mail.inc/class/DefaultMailSystem/1'),
    '@default_class' => mailsystem_default_value(),
  );

  $mail_system = mailsystem_read_settings();
  $delivery_classes = mailsystem_admin_get_delivery_classes();
  $formatter_classes = mailsystem_admin_get_formatter_classes();
  $mail_modules = mailsystem_admin_get_mail_modules();

  $form['mailsystem'] = array(
    '#type' => 'fieldset',
    '#title' => t('Mail System Settings'),
    '#description' => t('Backdrop provides a default method for formatting and delivering emails across the site. The default method formats the emails as plain text and delivers the emails using PHP\'s mail function. Contributed modules may provide additional methods for formatting, delivering, or both. For instance, a module can provide formatting as rich-text emails with images and attachments. Or a module can integrate with a more robust third-party email service for delivering the emails.'),
    '#collapsible' => FALSE,
    '#tree' => TRUE,
  );
  $module_info = '<ul>';
  foreach ($delivery_classes['descriptions'] as $module => $description) {
    $module_info .= '<li><strong>' . $module . '</strong>: ';
    $module_info .= $description . '</li>';
  }
  $module_info .= '</ul>';
  $form['mailsystem']['modules'] = array(
    '#type' => 'help',
    '#markup' => t('You can choose from any of the following enabled modules which implement email functionality:') . $module_info,
  );

  // Generate a list of themes which may used to render emails.
  $theme_options = array('current' => t('Current'), 'default' => t('Default'));
  if (module_exists('domain_theme')) {
    $theme_options['domain'] = t('Domain Theme');
  }
  // Get a list of all themes.
  $themes = list_themes();
  foreach ($themes as $name => $theme) {
    if ($theme->status == 1) {
      $theme_options[$name] = $theme->info['name'];
    }
  }
  $form['mailsystem']['mailsystem_theme'] = array(
      '#type' => 'select',
      '#title' => t('Theme to render rich text emails'),
      '#description' => t('Select the theme that will be used to render rich text emails. This can be either the current theme, the default theme, the domain theme or any active theme.'),
      '#options' => $theme_options,
      '#default_value' => config_get('mailsystem.settings', 'mailsystem_theme'),
  );

  $form['mailsystem'][mailsystem_default_id()] = array(
    '#type' => 'fieldset',
    '#title' => t('Site-wide default mail system'),
  );
  $form['mailsystem'][mailsystem_default_id()]['mail'] = array(
    '#type' => 'select',
    '#title' => t('Delivery'),
    '#options' => $delivery_classes['labels'],
    '#default_value' => $mail_system[mailsystem_default_id()]['mail'],
    '#description' => t('Class used to send the mail'),
  );
  $form['mailsystem'][mailsystem_default_id()]['format'] = array(
    '#type' => 'select',
    '#title' => t('Formatting'),
    '#options' => $formatter_classes['labels'],
    '#default_value' => $mail_system[mailsystem_default_id()]['format'],
    '#description' => t('Class used to format the body of the mail'),
  );

  unset($mail_system[mailsystem_default_id()]);

  $form['mailsystem']['custom-settings'] = array(
    '#type' => 'fieldset',
    '#title' => 'Custom settings',
    '#description' => t('Optionally set formatting and delivery methods for each module that sends out email, for example, event calendar notifications or contact form emails. Each module that sends out emails sets an identifier, and can further set one or more optional keys for finer-grained email settings. To decide which methods to use, Backdrop goes from the specific to the general. If no specific identifiers are set up then the site-wide default is used. The search order: <ol><li>The methods associated with the module and key, if any.</li><li>The method associated with the module, if any.</li><li>The site-wide defaults.</li></ol> For more technical information see <a href="!default_class"><code>@default_class</code></a> and <a href="!hook_mail"><code>hook_mail</code></a>.', $args),
  );
  foreach ($mail_system as $id => $settings) {
    $form['mailsystem']['custom-settings'][$id] = array(
      '#type' => 'fieldset',
      '#title' => t('Custom settings for mail-id %id', array('%id' => $id)),
    );
    $form['mailsystem']['custom-settings'][$id]['mail'] = array(
      '#type' => 'select',
      '#title' => t('Delivery'),
      '#options' => $delivery_classes['labels'],
      '#default_value' => $settings['mail'],
      '#description' => t('Class used to send the mail'),
    );
    $form['mailsystem']['custom-settings'][$id]['format'] = array(
      '#type' => 'select',
      '#title' => t('Formatting'),
      '#options' => $formatter_classes['labels'],
      '#default_value' => $settings['format'],
      '#description' => t('Class used to format the body of the mail'),
    );
    $form['mailsystem']['custom-settings'][$id]['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Remove custom settings for mail-id @id', array('@id' => $id)),
      '#submit' => array('mailsystem_admin_remove_setting_submit'),
      '#limit_validation_errors' => array(
        array('mailsystem', $id),
      ),
    );
  }

  $form['mailsystem']['add-custom-settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add custom settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#element_validate' => array('mailsystem_admin_add_setting_validate'),
  );
  $form['mailsystem']['add-custom-settings']['module'] = array(
    '#type' => 'select',
    '#title' => t('Module'),
    '#options' => $mail_modules,
  );
  $form['mailsystem']['add-custom-settings']['key'] = array(
    '#type' => 'textfield',
    '#title' => t('Key'),
    '#size' => 30,
    '#description' => t('An optional key which further specifies the mail in question. You may have to examine the source code of the <a href="!hook_mail">hook_mail</a> implementation of the module in question in order to find an appropriate value', $args),
  );
  $form['mailsystem']['add-custom-settings']['mail'] = array(
    '#type' => 'select',
    '#title' => t('Delivery'),
    '#options' => $delivery_classes['labels'],
    '#description' => t('Class used to send the mail'),
  );
  $form['mailsystem']['add-custom-settings']['format'] = array(
    '#type' => 'select',
    '#title' => t('Formatting'),
    '#options' => $formatter_classes['labels'],
    '#description' => t('Class used to format the body of the mail'),
  );
  $form['mailsystem']['add-custom-settings']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add custom settings'),
    '#submit' => array('mailsystem_admin_add_setting_submit'),
    '#limit_validation_errors' => array(
      array('mailsystem', 'add-custom-settings'),
    ),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save settings'),
  );
  return $form;
}

/**
 * Form validation handler for mailsystem settings form.
 *
 * Add an additional custom setting for a module/key.
 */
function mailsystem_admin_add_setting_validate($fieldset, &$form_state) {
  // Only perform this validation if we're adding a custom setting.
  if (!isset($form_state['submit_handlers']) || !in_array('mailsystem_admin_add_setting_submit', $form_state['submit_handlers'])) {
    return;
  }

  $module = $fieldset['module']['#value'];
  $key = $fieldset['key']['#value'];
  $id = $module;
  if (!empty($key)) {
    $id .= '_' . $key;
   }

  $mail_system = mailsystem_get();
  if (isset($mail_system[$id])) {
    form_error($fieldset['module'], t('A setting already exists for this combination of module and key'));
  }

  $form_state['new_settings_id'] = $id;
}

/**
 * Form API submit callback: Add an additional custom setting for a module/key.
 */
function mailsystem_admin_add_setting_submit($form, &$form_state) {
  $values = $form_state['values']['mailsystem']['add-custom-settings'];

  $id = $form_state['new_settings_id'];
  mailsystem_admin_save_custom_setting($id, $values);

  backdrop_set_message(t('Added new custom setting for mail-id %id',
    array('%id' => $id)));
}

/**
 * Form API submit callback: Remove a custom setting.
 */
function mailsystem_admin_remove_setting_submit($form, &$form_state) {
  $parents = $form_state['triggering_element']['#parents'];
  array_pop($parents); // submit
  $id = array_pop($parents); // fieldset

  mailsystem_clear(array($id => $id));

  backdrop_set_message(t('Removed custom setting for mail-id %id',
    array('%id' => $id)));
}

/**
 * Form API submit callback: save settings.
 */
function mailsystem_admin_settings_submit($form, &$form_state) {
  config_set('mailsystem.settings', 'mailsystem_theme', $form_state['values']['mailsystem']['mailsystem_theme']);
  // Rebuild the theme registry to make changes needed by theme rendering.
  backdrop_theme_rebuild();
  unset($form_state['values']['mailsystem']['mailsystem_theme']);
  unset($form_state['values']['mailsystem']['add-custom-settings']);

  // Combine the default and the custom settings arrays.
  $settings = array(
    mailsystem_default_id() => $form_state['values']['mailsystem'][mailsystem_default_id()]
  );
  if (isset($form_state['values']['mailsystem']['custom-settings'])) {
    $settings += $form_state['values']['mailsystem']['custom-settings'];
  }

  foreach ($settings as $id => $values) {
    mailsystem_admin_save_custom_setting($id, $values);
  }

  backdrop_set_message(t('Saved Mail System configuration'));
}


/**
 * Helper function. Writes the setting values for a given mail-id.
 */
function mailsystem_admin_save_custom_setting($id, $values) {
  if ($values['format'] != $values['mail']) {
    $value = array(
      'mail' => $values['mail'],
      'format' => $values['format'],
    );
  }
  else {
    $value = $values['mail'];
  }
  mailsystem_set(array($id => $value));
}

/**
 * Retrieves a list of modules implementing hook_mail().
 *
 * @return array
 *   An associative array of module descriptions keyed by the corresponding
 *   module machine name.
 */
function mailsystem_admin_get_mail_modules() {
  $modules = module_implements('mail');
  $result = array();

  foreach ($modules as $module) {
    $info = system_get_info('module', $module);
    $description = empty($info['package']) ? t('Other') : $info['package'];
    $description .= ' » ' . $info['name'];
    $result[$module] = $description;
  }

  return $result;
}

/**
 * Returns a list of classes suitable for sending mail.
 *
 * @return array
 *   List of the names of classes implementing MailSystemInterface.
 */
function mailsystem_admin_get_delivery_classes() {
  $delivery_classes = mailsystem_get_classes();
  unset($delivery_classes['MailsystemDelegateMailSystem']);
  return $delivery_classes;
}

/**
 * Returns a list of classes suitable for formatting email.
 *
 * @todo: Currently we consider all classes capable of both mail delivery and
 * formatting. If required in the future the mechanism could be extended such
 * that specific classes only serve either as formatter or delivery-system if
 * appropriate.
 *
 * @return array
 *   List of the names of classes implementing MailSystemInterface.
 */
function mailsystem_admin_get_formatter_classes() {
  return mailsystem_admin_get_delivery_classes();
}
