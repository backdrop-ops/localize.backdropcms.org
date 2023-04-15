<?php

/**
 * @file
 *   Translation template extractor module drush integration.
 */

/**
 * Implements hook_drush_command().
 *
 * @see drush_parse_command() for a list of recognized keys.
 *
 * @return
 *   An associative array describing our command.
 */
function potx_drush_command() {
  $items['potx'] = array(
    'callback' => 'potx_drush_extract',
    'description' => 'Extract translatable strings from Backdrop source code.',
    'arguments' => array(
      'mode' => 'potx output mode e.g. single multiple core',
    ),
    'options' => array(
      'modules' => 'Comma delimited list of modules to extract translatable strings from.',
      'files' => 'Comma delimited list of files to extract translatable strings from.',
      'folder' => 'Folder to begin translation extraction in. When no other option is set this defaults to current directory.',
      'api' => 'Backdrop core version to use for extraction settings.',
    ),
    'examples' => array(
      'potx single' => 'Extract translatable strings from applicable files in current directory and write to single output file',
      'potx multiple --modules=example' => 'Extract translatable strings from applicable files of example module and write to module-specific output file.',
      'potx --files=sites/all/modules/example/example.module' => 'Extract translatable strings from example.module and write to single output file.',
      'potx single --api=1 --folder=projects/backdrop/1' => 'Extract strings from folder projects/backdrop/1 using API version 1.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_NONE,
  );
  return $items;
}

/**
 * Implementation of hook_drush_help().
 *
 * This function is called whenever a drush user calls
 * 'drush help potx'.
 *
 * @param
 *   A string with the help section (prepended with 'drush:').
 *
 * @return
 *   A string with the help text for our command.
 */
function potx_drush_help($section) {
  if ($section == 'drush:potx') {
    $help = dt('Generates translation templates from the given Backdrop source code in the current working directory.');
    $help .= "\n\n". dt('Possible potx modes are:');
    $help .= "\n". dt(' single    Single file output mode, every file folded into the single output file (default).');
    $help .= "\n". dt(' multiple  Multiple file output mode, .info files folded into module .pot files.');
    $help .= "\n". dt(' core      Backdrop core output mode, .info files folded into general.pot.');
    $help .= "\n\n". dt('If no files are specified, potx will autodiscover files from the current working directory. You can specify concrete files to look at to limit the scope of the operation.');
    return $help;
  }
}

/**
 * Drush command callback.
 */
function potx_drush_extract($mode = NULL) {
  // Include library.
  include_once __DIR__ . '/potx.inc';
  include_once __DIR__ . '/potx.local.inc';

  $files = array();
  $build_mode = POTX_BUILD_SINGLE;

  if (!is_null($mode) && in_array($mode, array('core', 'multiple', 'single'))) {
    // First argument could be any of the mode names.
    $build_mode = constant('POTX_BUILD_'. strtoupper($mode));
  }
  // Silence error message reporting. Messages will be reported by at the end.
  potx_status('set', POTX_STATUS_SILENT);

  // Get Drush options.
  $modules_option = drush_get_option('modules');
  $files_option = drush_get_option('files');
  $folder_option = drush_get_option('folder');
  $api_option = drush_get_option('api');
  if (empty($api_option) || !in_array($api_option, array(1))) {
    $api_option = POTX_API_CURRENT;
  }

  potx_local_init($folder_option);

  if (!empty($modules_option)) {
    $modules = explode(',', $modules_option);
    foreach ($modules as $module) {
      $files = array_merge($files, _potx_explore_dir(backdrop_get_path('module', $module) . '/', '*', $api_option, TRUE));
    }
  }
  elseif (!empty($files_option)) {
    $files = explode(',', $files_option);
  }
  elseif (!empty($folder_option)) {
    $files = _potx_explore_dir($folder_option, '*', $api_option, TRUE);
  }
  else {
    // No file list provided so autodiscover files in current directory.
    $files = _potx_explore_dir(drush_cwd() . '/', '*', $api_option, TRUE);
  }

  foreach ($files as $file) {
    drush_print(dt("Processing $file..."));
    _potx_process_file($file, 0, '_potx_save_string', '_potx_save_version', $api_option);
  }

  _potx_build_files(POTX_STRING_RUNTIME, $build_mode);
  _potx_build_files(POTX_STRING_INSTALLER, POTX_BUILD_SINGLE, 'installer');
  $output_filenames = _potx_write_files();

  // Get warnings, if any.
  $warnings = potx_status('get');
  if (!empty($warnings)) {
    drush_print("");
    drush_print(dt("Warnings encountered (shown in -v verbose mode only)"));
    drush_print("");
    foreach ($warnings as $warning) {
      drush_log(strip_tags(htmlspecialchars_decode($warning, ENT_QUOTES)));
    }
  }

  drush_print("");
  foreach ($output_filenames as $filename) {
    drush_log(dt('Created output file @filename', array('@filename' => $filename)), 'success');
  }

  drush_print("");
  drush_print(dt("Stats"));
  $header = array(
    'files' => dt('Files'),
    'strings' => dt('Strings'),
    'warnings' => dt('Warnings'),
  );
  $rows = array(array_values($header));
  // Get saved strings.
  $strings = _potx_save_string(NULL,  NULL, NULL, 0, POTX_STRING_RUNTIME);
  $rows[] = array(count($files), count($strings), count($warnings));
  drush_print_table($rows, TRUE);

  drush_print("");
  drush_print(dt("Done"));
}
