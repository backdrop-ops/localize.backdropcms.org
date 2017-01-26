<?php
/**
 * @file
 * dsb config file.
 */

$config['build tag'] = 'l10n';

$config['domain'] = 'l10n.dev';

$config['makefile'] = 'dsb.l10n.make';

/**
 * Server setup script.
 *
 * Creates an instance of the server.
 */
$config['scenarios']['server'] = array(
  'custom variables' => array(),
  'post install script' => array(),
  'cleanup script' => array(),
  'hosts' => array(
    'server' => array(
      'setup script' => array(
        'drush en -y l10n_server l10n_community l10n_gettext',
        // Enable l10n text import connector.
        'drush vset l10n_server_connector_l10n_gettext_uploads_enabled 1',
        // Get a simplenews translation release file.
        'wget -O files/simplenews-7.x-1.0.de.po http://ftp.drupal.org/files/translations/7.x/simplenews/simplenews-7.x-1.0.de.po',
        // Setup the server configuration.
        "drush php-script \$WORKSPACE/scripts/dsb_server_setup.inc",
      ),
    ),
  ),
);

/**
 * Client server setup.
 *
 * Creates an instance of each server and client.
 */
$config['scenarios']['client-server'] = array(
  'custom variables' => array(),
  'post install script' => array(
    // Add API key of the server site in the client site's admin users profile.
    "\$WORKSPACE/scripts/dsb_post_install.sh",
    // @todo dsb should support post install scripts per host.
  ),
  'cleanup script' => array(),
  'hosts' => array(
    'server' => array(
      'setup script' => array(
        'drush en -y l10n_server l10n_community l10n_gettext l10n_remote',
        // Enable l10n text import connector.
        'drush vset l10n_server_connector_l10n_gettext_uploads_enabled 1',
        // Get a simplenews translation release file.
        'wget -O files/simplenews-7.x-1.0.de.po http://ftp.drupal.org/files/translations/7.x/simplenews/simplenews-7.x-1.0.de.po',
        // Setup the server configuration.
        "drush php-script \$WORKSPACE/scripts/dsb_server_setup.inc",
      ),
    ),
    'client' => array(
      'setup script' => array(
        'drush en -y l10n_client simplenews',
        // Connect client to server.
        'drush vset l10n_client_server http://server.$DOMAIN',
        // Enable client to send suggestions to server.
        'drush vset l10n_client_use_server 1',
        // Setup the client.
        "drush php-script \$WORKSPACE/scripts/dsb_client_setup.inc",
      ),
    ),
  ),
);

/**
 * Automated tests.
 */
$config['scenarios']['test'] = array(
  'cleanup script' => array(
    'chmod -R 777 $BUILD_TARGET',
    'rm -R $BUILD_TARGET',
  ),
  'hosts' => array(
    'test' => array(),
  ),
  'tests' => array(
    'all' => array(
      'test objects' => array('Localization server'),
    ),
  ),
);

// Include local config file with config overrides.
if (file_exists(dirname(__FILE__) . '/local.dsb.config.php')) {
  include dirname(__FILE__) . '/local.dsb.config.php';
}
