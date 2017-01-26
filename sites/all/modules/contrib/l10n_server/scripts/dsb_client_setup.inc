<?php

/**
 * @file
 * Script executed by drush dsb.
 *
 * The script sets up an additional language and enables URL detection. This is
 * necessary in order to use the l10n_client.
 *
 * This script requires the context of drush-script.
 */

// Add german language.
locale_add_language('de');
db_update('languages')
  ->fields(array('plurals' => 2))
  ->condition('language',  'de')
  ->execute();
drush_log('Enabled language german.', 'success');

// Enable URL detection method.
variable_set('language_negotiation_language', array (
  'locale-url' => array(
    'callbacks' => array(
      'language' => 'locale_language_from_url',
      'switcher' => 'locale_language_switcher_url',
      'url_rewrite' => 'locale_language_url_rewrite_url',
    ),
    'file' => 'includes/locale.inc',
  ),
  'language-default' => array(
    'callbacks' => array(
      'language' => 'language_from_default',
    ),
  ),
));
variable_set('language_negotiation_language_content', array(
  'locale-interface' => array(
    'callbacks' => array(
      'language' => 'locale_language_from_interface',
    ),
    'file' => 'includes/locale.inc',
  ),
));
variable_set('language_negotiation_language_url', array(
  'locale-url' => array(
    'callbacks' => array(
      'language' => 'locale_language_from_url',
      'switcher' => 'locale_language_switcher_url',
      'url_rewrite' => 'locale_language_url_rewrite_url',
    ),
    'file' => 'includes/locale.inc',
  ),
  'locale-url-fallback' => array(
    'callbacks' => array(
      'language' => 'locale_language_url_fallback',
    ),
    'file' => 'includes/locale.inc',
  ),
));

drush_log('Language detection was set to URL first.', 'success');
