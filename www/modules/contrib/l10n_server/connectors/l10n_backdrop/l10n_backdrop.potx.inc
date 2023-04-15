<?php

/**
 * Initialize potx to run on l10n_server.
 */
function _l10n_backdrop_potx_init() {
  global $_potx_module_metadata;
  global $potx_callbacks;

  $_potx_module_metadata = array();

  $potx_callbacks = array(
    'schema_store_lookup' => '_l10n_backdrop_schema_store_lookup',
    'schema_reverse_lookup' => '_l10n_backdrop_schema_reverse_lookup',
    'load_module_metadata' => '_l10n_backdrop_load_module_metadata',
    'store_module_metadata' => '_l10n_backdrop_store_module_metadata',
    'schema_store' => '_l10n_backdrop_schema_store',
    'schema_load' => '_l10n_backdrop_schema_load',
  );
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
function _l10n_backdrop_schema_store_lookup($keys, $module_name) {
  if (!is_array($keys)) {
    return;
  }

  $result = db_query("SELECT * FROM {l10n_backdrop_schema_reverse_lookup} WHERE schema_key IN (:keys) AND module_name = :module_name",
      array(':keys' => $keys, ':module_name' => $module_name));

  // Exclude existing records
  foreach ($result as $record) {
    unset($keys[array_search($record->schema_key, $keys)]);
  }

  if (!is_array($keys)) {
    return;
  }

  $query = db_insert('l10n_backdrop_schema_reverse_lookup')->fields(array('schema_key', 'module_name'));
  foreach ($keys as $key) {
    $query->values(array(
      'schema_key' => $key,
      'module_name' => $module_name,
    ));
  }

  $query->execute();
}

/**
 * Find the module containing the schema for a specific config file, based on
 *  its matching candidates, using a reverse lookup table.
 *
 * @param array $matching_candidates
 *   The list of matching candidates for a config file, ordered from specific to
 *    generic.
 *
 * @return string|null
 *   The module containing the matching schema, if exists, otherwise NULL.
 *
 * @see _potx_find_matching_schema()
 */
function _l10n_backdrop_schema_reverse_lookup($matching_candidates) {
  // Search for modules that contain any schema definitions that could possibly match with this config file.
  $result = db_query("SELECT * FROM {l10n_backdrop_schema_reverse_lookup} WHERE schema_key IN (:matching_candidates)", array(':matching_candidates' => $matching_candidates));

  $mapped = array();
  foreach ($result as $record) {
    $mapped[$record->schema_key] = $record->module_name;
  }

  foreach ($matching_candidates as $candidate) {
    if (isset($mapped[$candidate])) {
      return $mapped[$candidate];
    }
  }

  return NULL;
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
function _l10n_backdrop_load_module_metadata($module_name) {
  global $_potx_module_metadata;

  $_potx_module_metadata[$module_name]['dependencies'] = array();

  $record = db_query("SELECT * FROM {l10n_backdrop_parsed_module_data} WHERE module_name = :module_name", array(':module_name' => $module_name))->fetchObject();

  if ($record === FALSE) {
    // Module metadata not found.
    return FALSE;
  }

  $record->dependencies = unserialize($record->dependencies);
  if (is_array($record->dependencies)) {
    $_potx_module_metadata[$module_name]['dependencies'] = $record->dependencies;
  }

  return TRUE;
}

/**
 * Store the metadata for a module, including its dependencies, in the database.
 *
 * @param string $module_name
 *   The module's name
 * @param array $metadata
 *   The module's metadata
 */
function _l10n_backdrop_store_module_metadata($module_name, $metadata) {
  $dependencies = isset($metadata['dependencies']) ? $metadata['dependencies'] : array();
  db_merge('l10n_backdrop_parsed_module_data')
    ->key(array('module_name' => $module_name))
    ->fields(array('dependencies' => serialize($dependencies)))
    ->execute();
}

/**
 * Store a module's processed schema in the database.
 *
 * The processed schema is found in the "$_potx_module_schema" global.
 *
 * @param string $module_name
 *   The module name.
 */
function _l10n_backdrop_schema_store($module_name) {
  global $_potx_module_schema;

  db_merge('l10n_backdrop_parsed_module_data')
    ->key(array('module_name' => $module_name))
    ->fields(array('parsed_schema' => serialize($_potx_module_schema)))
    ->execute();
}

/**
 * Load a module's processed schema from the database.
 *
 * @param string $module_name
 *   The module's name.
 */
function _l10n_backdrop_schema_load($module_name) {
  $record = db_query("SELECT * FROM {l10n_backdrop_parsed_module_data} WHERE module_name = :module_name", array(':module_name' => $module_name))->fetchObject();

  if ($record === FALSE) {
    // Module metadata not found.
    return NULL;
  }

  return unserialize($record->parsed_schema);
}
