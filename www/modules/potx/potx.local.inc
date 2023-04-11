<?php

/**
 * Initialize potx to run locally, e.g. by drush.
 *
 * @param string $module_path
 *   Path to the module that is being parsed by potx.
 */
function potx_local_init($module_path = NULL) {
  global $_potx_found_modules;
  global $_potx_schema_lookup;
  global $_potx_reverse_lookup_built;

  $_potx_found_modules = array();
  $_potx_schema_lookup = array();
  $_potx_reverse_lookup_built = FALSE;

  global $potx_callbacks;

  $potx_callbacks = array(
      'schema_store_lookup' => '_potx_schema_store_lookup',
      'load_module_metadata' => '_potx_load_module_metadata',
      'store_module_metadata' => '_potx_store_module_metadata',
      'schema_store' => '_potx_schema_store',
      'schema_load' => '_potx_schema_load',
  );

  if ($module_path !== NULL) {
    _potx_find_all_modules($module_path);
  }
}

/**
 * Find all available modules based on Backdrop's directory structure.
 *
 * @param string $module_path
 *   Path to the module that is being parsed by potx.
 */
function _potx_find_all_modules($module_path) {
  global $_potx_found_modules;

  $module_path = realpath($module_path);
  if (substr($module_path, -1) != '/') {
    $module_path = $module_path . '/';
  }

  // The list of directories to check in the path, to find out if we are in a Backdrop install directory.
  $checks = array('/sites/', '/core/', '/profiles/', '/modules/', '/themes/');

  // The list of paths that could contain "modules/" or "themes/" subdirectories.
  $search_paths = array();

  foreach ($checks as $check) {
    if (preg_match("!$check!", $module_path)) {
      $parts = explode($check, $module_path, 2);

      // The installed Backdrop root.
      $root = $parts[0];

      // The Backdrop core directory contains a config/schema subdirectory,
      // which is not part of any module or theme.
      $_potx_found_modules['core']['path'] = $root . '/core';

      $search_paths[] = $root;
      $search_paths[] = $root . '/core';

      $profiles = glob($root . '/profiles/*', GLOB_ONLYDIR);
      $sites = glob($root . '/sites/*', GLOB_ONLYDIR);

      if (is_array($profiles)) {
        $search_paths = array_merge($search_paths, $profiles);
      }
      if (is_array($sites)) {
        $search_paths = array_merge($search_paths, $sites);
      }

      break;
    }
  }

  foreach ($search_paths as $search_path) {
    foreach (array('/modules', '/themes') as $sub) {
      if (is_dir($search_path . $sub)) {
        _potx_find_modules($search_path . $sub);
      }
    }
  }
}

/**
 * Recursively find all modules in a given path, by looking for .info.yml files.
 *
 * @param string $path
 *   The search path
 */
function _potx_find_modules($path) {
  global $_potx_found_modules;

  $subdirs = glob($path . '/*', GLOB_ONLYDIR);

  if (is_array($subdirs)) {
    foreach ($subdirs as $dir) {
      if (!preg_match("!(^|.+/)(CVS|\.svn|\.git|tests|vendor)$!", $dir)) {
        $module_name = basename($dir);
        $info_path = $dir . '/' . $module_name . '.info.yml';

        if (file_exists($info_path)) {
          $_potx_found_modules[$module_name]['path'] = $dir;
        }

        _potx_find_modules($dir);
      }
    }
  }
}

/**
 * Store a config schema file's matching keys (e.g. field.field.*.*.*), and
 *  the module that contains the schema, in a lookup table.
 *
 * @param array $keys
 *   List of config matching keys in a config schema file.
 *
 * @param string $module_name
 *   Name of the module containing the schema.
 */
function _potx_schema_store_lookup($keys, $module_name) {
  global $_potx_schema_lookup;
  $_potx_schema_lookup = array_merge($_potx_schema_lookup, array_fill_keys($keys, $module_name));
}

/**
 * Load a module's metadata, including its dependencies and list of config
 *  schema files.
 *
 * @param string $module_name
 *   The module's name.
 *
 * @return bool
 *   TRUE if the module was found, FALSE otherwise.
 */
function _potx_load_module_metadata($module_name) {
  global $_potx_found_modules;
  global $_potx_module_metadata;

  if (!isset($_potx_found_modules[$module_name])) {
    return FALSE;
  }

  $module_path = $_potx_found_modules[$module_name]['path'];

  if ($module_name === 'core') {
    $_potx_module_metadata['core']['dependencies'] = array();
  }
  else {
    // Add dependencies from .info files?
  }

  return TRUE;
}

/**
 * Store the metadata for a module, including its dependencies.
 *
 * Not used locally by potx.
 *
 * @param string $module_name
 *   The module's name
 * @param array $metadata
 *   The module's metadata
 */
function _potx_store_module_metadata($module_name, $metadata) {
  // Intentionally Left Empty.
}

/**
 * Store a module's processed schema, in a cache (for local potx), or database
 *  (for l10n_server).
 *
 * The processed schema is found in the "$_potx_module_schema" global.
 *
 * @param string $module_name
 *   The module name.
 */
function _potx_schema_store($module_name) {
  global $_potx_module_schema;
  global $_potx_schema_cache;

  $_potx_schema_cache[$module_name] = $_potx_module_schema;
}

/**
 * Load a module's processed schema, from cache (for local potx), or database
 *  (for l10n_server).
 *
 * @param string $module_name
 *   The module's name.
 */
function _potx_schema_load($module_name) {
  global $_potx_schema_cache;
  if (isset($_potx_schema_cache[$module_name])) {
    return $_potx_schema_cache[$module_name];
  }

  return NULL;
}
