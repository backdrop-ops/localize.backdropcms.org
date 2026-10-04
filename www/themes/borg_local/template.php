<?php
/**
 * @file
 * Theme function overrides.
 */

/*******************************************************************************
 * Prepare variables for templates.
 ******************************************************************************/

/**
 * Preprocess header templates.
 * @see header.tpl.php
 */
function borg_local_preprocess_header(&$variables) {
  // Add grid system classes.
  $variables['branding_classes'] = array('col-xs-9', 'col-sm-8', 'col-md-6', 'col-lg-4');
  // Note: these may be added to the Primary Menu block instead.
  $variables['navigation_classes'] = array('col-xs-3', 'col-sm-4', 'col-md-6', 'col-lg-8');
}
