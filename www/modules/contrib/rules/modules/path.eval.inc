<?php
/**
 * @file
 * Contains rules integration for the path module needed during evaluation.
 *
 * @addtogroup rules
 *
 * @{
 */

/**
 * Action implementation: Path alias.
 */
function rules_action_path_alias($source, $alias, $langcode = LANGUAGE_NONE) {
  if (!$alias) {
    path_delete(array('source' => $source, 'langcode' => $langcode));
  }
  elseif (!$source) {
    path_delete(array('alias' => $alias, 'langcode' => $langcode));
  }
  // Only set the alias if the alias is not taken yet.
  elseif (!path_load(array('alias' => $alias, 'langcode' => $langcode))) {
    // Update the existing path or create a new one.
    if ($path = path_load(array('source' => $source, 'langcode' => $langcode))) {
      $path['alias'] = $alias;
    }
    else {
      $path = array('source' => $source, 'alias' => $alias, 'langcode' => $langcode);
    }
    path_save($path);
  }
  else {
    rules_log('The configured alias %alias already exists. Aborting.', array('%alias' => $alias));
  }
}

/**
 * Action Implementation: Set the URL alias for a node.
 */
function rules_action_node_path_alias($node, $alias) {
  $langcode = isset($node->langcode) ? $node->langcode : LANGUAGE_NONE;
  // Only set the alias if the alias is not taken yet.
  if (($path = path_load(array('alias' => $alias, 'langcode' => $langcode))) && (empty($node->path['pid']) || $node->path['pid'] != $path['pid'])) {
    rules_log('The configured alias %alias already exists. Aborting.', array('%alias' => $alias));
    return FALSE;
  }
  // Turn off auto to assure that we can save the custom url alias.
  $node->path['auto'] = FALSE;
  $node->path['alias'] = $alias;
}

/**
 * Action Implementation: Set the URL alias for a node.
 */
function rules_action_taxonomy_term_path_alias($term, $alias) {
  // Only set the alias if the alias is not taken yet.
  if (($path = path_load(array('alias' => $alias, 'langcode' => LANGUAGE_NONE))) && (empty($term->path['pid']) || $term->path['pid'] != $path['pid'])) {
    rules_log('The configured alias %alias already exists. Aborting.', array('%alias' => $alias));
    return FALSE;
  }
  // Turn off auto to assure that we can save the custom url alias.
  $term->path['auto'] = FALSE;
  $term->path['alias'] = $alias;
}

/**
 * Condition implementation: Check if the path has an alias.
 */
function rules_condition_path_has_alias($source, $langcode = LANGUAGE_NONE) {
  return (bool) backdrop_lookup_path('alias', $source, $langcode);
}

/**
 * Condition implementation: Check if the URL alias exists.
 */
function rules_condition_path_alias_exists($alias, $langcode = LANGUAGE_NONE) {
  return (bool) backdrop_lookup_path('source', $alias, $langcode);
}

/**
 * Cleans the given path.
 *
 * A path is cleaned by replacing non ASCII characters in the path with the
 * replacement character.
 *
 * Path cleaning may be customized by overriding the configuration variables:
 * @code rules_clean_path @endcode,
 * @code rules_path_replacement_char @endcode and
 * @code rules_path_transliteration @endcode
 * in the site's settings.php file.
 * @deprecated Path module is now built into Backdrop and so
 * rules_path_clean_path() is the preferred path-cleaning method.
 */
function rules_path_default_cleaning_method($path) {
  watchdog_deprecated_function('rules', __FUNCTION__);
  $replace = config_get('rules.settings','rules_path_replacement_char');
  if ($replace) {
    // If transliteration is enabled, transliterate the alias first.
    if (config_get('rules.settings','rules_path_transliteration')) {
      $path = transliteration_get($path);
    }
    $preg_args = array(
      config_get('rules.settings','rules_path_regex'),
      $replace,
    );
    // Replace it and remove trailing and leading replacement characters.
    $output = trim(call_user_func_array('preg_replace', $preg_args), $replace);

    if (config_get('rules.settings','rules_path_lower_case')) {
      $output = backdrop_strtolower($output);
    }
    return $output;
  }
  else {
    return $path;
  }
}

/**
 * Cleans the given string so it can be used as part of a URL path.
 */
function rules_clean_path($path) {
  $function = config_get('rules.settings','rules_path_cleaning_callback');
  if (!function_exists($function)) {
    rules_log('An invalid URL path cleaning callback has been configured. Falling back to the default cleaning method.', array(), RulesLog::WARN);
    $function = 'rules_path_default_cleaning_method';
  }
  return $function($path);
}

/**
 * Path cleaning callback.
 *
 * @see rules_admin_settings()
 */
function rules_path_clean_path($path) {
  module_load_include('inc', 'path');
  return path_clean_string($path);
}

/**
 * @} End of "addtogroup rules"
 */
