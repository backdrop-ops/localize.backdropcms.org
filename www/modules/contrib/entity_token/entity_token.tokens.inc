<?php

/**
 * @file
 * Provides tokens for entity properties which have no token yet.
 */

/**
 * Implements hook_token_info_alter().
 * 
 * Since Backdrop's core already provides all token types for core entities
 * and all tokens for fields, we are very selective with the list
 * of tokens we provide here. We provide tokens not included in core.
 */
function entity_token_token_info_alter(&$info) {

  // Provide D7 Entity Tokens legacy tokens not provided by Backdrop core.
  // Tokens for type node.
  $info['tokens']['node']['is_new'] = array(
    'name' => 'Is new',
    'description' => t('Whether the node is new and not saved to the database yet.'),
    'type' => 'boolean',
    'entity-token' => TRUE,
  );
  $info['tokens']['node']['status'] = array(
    'name' => 'Status',
    'description' => t('Whether the node is published or unpublished.'),
    'entity-token' => TRUE,
  );
  $info['tokens']['node']['promoted'] = array(
    'name' => 'Promoted to frontpage',
    'description' => t('Whether the node is promoted to the frontpage.'),
    'entity-token' => TRUE,
  );
  $info['tokens']['node']['sticky'] = array(
    'name' => 'Sticky in lists',
    'description' => t('Whether the node is displayed at the top of lists in which it appears.'),
    'entity-token' => TRUE,
  );
  $info['tokens']['node']['revision'] = array(
    'name' => 'Creates revision',
    'description' => t('Whether saving this node creates a new revision.'),
    'entity-token' => TRUE,
  );

  // Tokens for type comment.
  if (module_exists('comment')) {
    $info['tokens']['comment']['subject'] = array(
      'name' => 'Subject',
      'description' => t('The subject of the comment.'),
      'entity-token' => TRUE,
    );
    $info['tokens']['comment']['status'] = array(
      'name' => 'Status',
      'description' => t('Whether the comment is published or unpublished.'),
      'entity-token' => TRUE,
    );
    $info['tokens']['comment']['comment_body'] = array(
      'name' => 'Comment',
      'description' => t('Field "comment_body". The following properties may be appended to the token: value (Text), format (Text format)'),
      'entity-token' => TRUE,
    );
  }

  // Tokens for type user.
  $info['tokens']['user']['status'] = array(
    'name' => 'Status',
    'description' => t('Whether the user is active or blocked.'),
    'entity-token' => TRUE,
  );
  $info['tokens']['user']['last_access'] = array(
    'name' => 'Last access',
    'description' => t('The date the user last accessed the site.'),
    'type' => 'date',
    'entity-token' => TRUE,
  );
  $info['tokens']['user']['theme'] = array(
    'name' => 'Default theme',
    'description' => t('The user\'s default theme.'),
    'entity-token' => TRUE,
  );

  $info['tokens']['current-user']['status'] = array(
    'name' => 'Status',
    'description' => t('Whether the user is active or blocked.'),
    'entity-token' => TRUE,
  );

  // Tokens for type site
  $info['tokens']['site']['current_user'] = array(
    'name' => 'Logged in user',
    'description' => t('The currently logged in user. The following properties may be appended to the token: name, mail, url, roles, status.'),
    'type' => 'user',
    'entity-token' => TRUE,
  );

  if (module_exists('taxonomy')) {
    // Tokens for type term.
    $info['tokens']['term']['parents_all'] = array(
      'name' => 'All parent terms',
      'description' => t('Ancestors of the term, i.e. parent of all above hierarchy levels.'),
      'type' => 'list<term>',
      'entity-token' => TRUE,
    );
    $info['tokens']['term']['weight'] = array(
      'name' => 'Weight',
      'description' => t('The weight of the term, which is used for ordering terms during display.'),
      'entity-token' => TRUE,
    );
  }

  // Add types and tokens for all properties of entities that don't have one.
  $entity_info = entity_get_info();
  $field_info_map = field_info_field_map();
  foreach ($entity_info as $entity_name => $entity_info_array) {
    // If the entity doesn't have any properties, skip.
    if (!count(entity_plus_get_all_property_info($entity_name))) {
      continue;
    }

    $token_type_for_entity = isset($entity_info_array['token type']) ? $entity_info_array['token type'] : $entity_name;

    // Create token types for all non-core entities that don't already exist.
    if (!_entity_token_is_core_entity($entity_name) && !isset($info['types'][$token_type_for_entity])) {
      $info['types'][$token_type_for_entity] = array(
        'name' => $entity_info[$entity_name]['label'],
        'description' => t('Tokens related to the "@name" entities.', array('@name' => $entity_info[$entity_name]['label'])),
        'needs-data' => $token_type_for_entity,
      );
    }

    // Create token definitions for entity properties and fields.
    foreach (entity_plus_get_all_property_info($entity_name) as $name => $property) {
      // Skip touching the definitions for image and date fields, 
      // as these are created in core.
      if (!empty($property['field']) && in_array($field_info_map[$name]['type'], array('date', 'datetime', 'datestamp', 'image',))) {
        continue;
      }

      // Only create a token if it doesn't already exist.
      $name_with_dashes = str_replace('_', '-', $name);
      if (!isset($info['tokens'][$token_type_for_entity][$name]) && !isset($info['tokens'][$token_type_for_entity][$name_with_dashes])) {
        $info['tokens'][$token_type_for_entity][$name] = array(
          'name' => $property['label'],
          'description' => !empty($property['description']) ? t($property['description']) : '',
          'type' => entity_token_determine_type($property),
          'entity-token' => TRUE,
        );
      }
      
      // Add explanatory description for multi-value fields. Add extracted type so that info tree is built correctly.
      if (!empty($property['field']) && $extracted_type = entity_plus_property_list_extract_type(entity_token_determine_type($property))) {
        $info['tokens'][$token_type_for_entity][$name]['dynamic'] = TRUE;
        $info['tokens'][$token_type_for_entity][$name]['description'] .= ' ' . t('Replace the ? with the delta for multi-value fields.');
        $info['tokens'][$token_type_for_entity][$name]['type'] = $extracted_type;
      }

      // If field provides a structure (e.g. formatted text fields)
      // make the token type a struct, and add the keys to
      // the token description.
      if (isset($property['property info'])) {
        // There are instances when properties use underscores and existing tokens use
        // dashes (e.g. ubercart shipping_address). If that's the case, use the existing name with dashes.
        $name = isset($info['tokens'][$token_type_for_entity][$name_with_dashes]) ? $name_with_dashes : $name;

        $info['tokens'][$token_type_for_entity][$name]['type'] = 'struct';
        $help = array();
        foreach ($property['property info'] as $key => $property_info) {
          $help[] = $key . ' (' . $property_info['label'] . ')';
        }
        $info['tokens'][$token_type_for_entity][$name]['description'] .= ' ' . t('The following properties may be appended to the token: @keys',
          array('@keys' => implode(', ', $help))
        );
      }
    }
  }
}

/** 
 * Implements hook_tokens().
 * This function processes:
 *  - A few unimplemented simple tokens for various entities (legacy from D7 entity_token).
 *  - Field tokens that contain chains (fields without chains are fulfilled by field_tokens()), 
 *    except chained image and date fields that are processed by core.
 *  - Chained field tokens (except chained date and image field tokens).
 *  - Tokens types such as list<something> that are multi-value.
 */
function entity_token_tokens($type, $tokens, array $data = array(), array $options = array()) {
  $replacements = array();
  $field_info_map = field_info_field_map();
  $token_info = token_get_info();

  // Process tokens when passing data for an entity
  if (isset($data[$type]) && !empty($data[$type]) && $type !== 'wrapped-field' && is_a($data[$type], 'Entity')) {
    $entity_type = $data[$type]->entityType();
    $wrapper = entity_metadata_wrapper($entity_type, $data[$type]);
    $entity_properties = $wrapper->getPropertyInfo();
    $field_name_list = array();
    $property_name_list = array();

    foreach ($tokens as $name => $original) {
      $property_name = isset($entity_properties[$name]) ? $name : str_replace('-', '_', $name);
      // Fulfill simple, unchained, D7 legacy tokens for core entities, and unchained tokens for custom entities.
      if ((isset($token_info['tokens'][$type][$property_name]['entity-token']) && $token_info['tokens'][$type][$property_name]['entity-token']) || 
        (isset($token_info['tokens'][$type][$name]['entity-token']) && $token_info['tokens'][$type][$name]['entity-token'])) {
        
        // Skip if this is an unchained field - those are fulfilled by core.
        if(!empty($field_info_map[$property_name])) {
          // Intercept unchained field tokens with dashes and resend them for core replacement.
          if ($name !== $property_name) {
            $replacements += token_generate($type, array($property_name => $original), $data, $options);
          }
          continue;
        }
        try {
          if (isset($wrapper->$property_name)) {
            $replacement = _entity_token_get_token($wrapper->$property_name, $options);
            if (isset($replacement)) {
              $replacements[$original] = $replacement;
            }
          }
        }
        catch (EntityMetadataWrapperException $e) {
          // If getting values generate an error, do nothing. This may happen if the property info is not defined.
        }
      }
      // Care for chained field tokens, and for chained non-field tokens of non-core entities.
      elseif (strpos($name, ':') !== FALSE) {
        $name_parts = explode(':', $name);
        // Check if this is a field or a property
        $field_name = str_replace('-', '_', $name_parts[0]);
        if (!empty($name_parts[0]) && isset($wrapper->{$field_name}) ) {

          // Only add fields to the list. Do not add image or date fields as those are now fulfilled by core.
          if (!empty($field_info_map[$field_name])) {
            if (strpos($field_info_map[$field_name]['type'], 'date') === FALSE || strpos($field_info_map[$field_name]['type'], 'image') === FALSE) {
              // Collect the name of the field
              $field_name_list[$name_parts[0]] = $field_name;
            }
          } 
          // Care for non-field chained tokens, but ONLY if this is "ours".
          elseif ((isset($token_info['tokens'][$type][$field_name]['entity-token']) && $token_info['tokens'][$type][$field_name]['entity-token']) || 
            (isset($token_info['tokens'][$type][$name_parts[0]]['entity-token']) && $token_info['tokens'][$type][$name_parts[0]]['entity-token'])
            ) {
            $property_name_list[$name_parts[0]] = $field_name;
          }
        }
      }
    }

    // Now process tokens for each of the chained fields collected above
    foreach ($field_name_list as $token_name => $field_name ) {
      $chained_tokens = token_find_with_prefix($tokens, $token_name);
      $replacements += token_generate('wrapped-field', $chained_tokens, array('wrapped-field' => $wrapper->$field_name), $options);
    } 

    // Now process tokens for each of the chained properties collected above
    foreach ($property_name_list as $token_name => $property_name) {
      $chained_type = entity_token_determine_type($entity_properties[$property_name]);
      $property_tokens = token_find_with_prefix($tokens, $token_name);
      $replacements += token_generate($chained_type, $property_tokens, array($chained_type => $wrapper->{$property_name}->value()), $options);
    }
  } // End of process for $data that's an entity keyed by entity type.
  
  // Process tokens of type 'wrapped-field', generated by the above code.
  // The data passed here is a metadata wrapper of a field.
  // At the moment only CHAINED tokens are passed here (except those for image and date fields, which are handled by core).
  // since unchained field tokens are always handled by core.
  // The $tokens variable starts with the first property or delta to be fulfilled.
  elseif ($type == 'wrapped-field' && isset($data[$type]) && !empty($data[$type]) && is_a($data[$type], 'EntityMetadataWrapper')){
    $wrapper = $data['wrapped-field'];

    $token_type_from_wrapper = entity_token_determine_type($wrapper->info());
    foreach ($tokens as $name => $original) {
      // $name is comprised delta:property:etc or property:etc
      $name_parts = explode(':', $name);
      $replacement = '';
      if (empty($name_parts[0]) && $name_parts[0] != '0') {
        continue;
      }
      if ($token_type_from_wrapper == 'file') {
        // Get the file object from the wrapper. We send the object (not the wrapper) to core
        // file tokens fulfilled in system_tokens().
        $new_data = $wrapper->file->value();

      } elseif ($token_type_from_wrapper == 'struct') {
        // If this is a struct we'll pass the wrapper verbatim
        $new_data = $wrapper; 

        // @todo Find a better way to check for list.
      } elseif (strpos($token_type_from_wrapper, 'list')!== FALSE) {
        // Token types that include list are multi-value fields.
        // Only pass it if the delta is numeric.
        if (is_numeric($name_parts[0])) {
          $new_data = $wrapper;
        }

      } elseif (is_a($wrapper->value(), 'Entity')) {
        // Care for entity reference fields. Pass the entity itself.
        $new_data = $wrapper->value();
      }

      if (!empty($new_data)) {
        // Generate tokens for struct and list<something>
        $replacements += token_generate($token_type_from_wrapper, array($name => $original), array($token_type_from_wrapper => $new_data), $options);
      }
    }
  } // End of process for wrapped-field

  // Process tokens that are of type list<something>. 
  // These are regular arrays, arrays of entities, or arrays of structures.
  // We get here after the 'wrapped-field' type has processed
  // In this case, $data keyed by list<something> and 
  // subarray of entities keyed by deltas
  elseif (!empty($data[$type]) && strpos($type, 'list') !== FALSE  && is_a($data[$type], 'EntityMetadataWrapper')) {
    $token_type = entity_plus_property_list_extract_type($type);

    foreach ($tokens as $name => $original) {
      $name_parts = explode(':', $name);
      $chained_tokens = token_find_with_prefix($tokens, $name_parts[0]);

      // If there are not chained tokens after the delta, we attempt to fulfill
      // the value replacement here 
      if (empty($chained_tokens)) {
        if (is_numeric($name)) { // be sure this is a real delta
          try {
            $wrapper = $data[$type]->get($name_parts[0]);
                
            if (isset($wrapper)) {
              $replacement = _entity_token_get_token($wrapper, $options);
              if (isset($replacement)) {
                $replacements[$original] = $replacement;
              }
            }
          }
          catch (EntityMetadataWrapperException $e) {
            // If getting values generate an error, do nothing. This may happen if the property info is not defined
          }
        }
      } 
      
      // If there are chained elements after the delta, pass that to token_generate 
      elseif (is_numeric($name_parts[0])) {
        $pass_data = '';
        // Check to be sure the delta value exists.
        try {
          switch ($token_type) {
            case 'struct':
              // We pass the wrapper to the element.
              $pass_data = $data[$type]->get($name_parts[0]);
              break;
  
            case 'file':
              // We need to pass the file object.
              $pass_data = $data[$type]->get($name_parts[0])->file->value();
              break;

            default:
              // We pass the value, since most likely this is not fulfilled in this module
              $pass_data = $data[$type]->get($name_parts[0])->value();
              break;
          }
        } catch (EntityMetadataWrapperException $e) {
          // Do nothing if the property doesn't exist
        }
        
        if (!empty($pass_data)) {
          $replacements += token_generate($token_type, $chained_tokens, array($token_type => $pass_data), $options);
        }
      }
    }
  } // End of process for token types that are list<something>

  // Process wrapped struct token types
  // These are associative arrays of properties
  elseif ($type == 'struct' && isset($data[$type]) && !empty($data[$type])) {
    $wrapper = $data['struct'];
    if (is_a($wrapper, 'EntityMetadataWrapper')) {
      foreach ($tokens as $name => $original) {
        $value = '';
        $name_parts = explode(':', $name);
        foreach ($name_parts as $name_part) {
          if (!isset($wrapper->{$name_part})) {
            continue 2;
          }
          $wrapper = $wrapper->{$name_part};
        }
        try {
          $value = _entity_token_get_token($wrapper, $options);
        } catch (EntityMetadataWrapperException $e) {

        }
        if (!empty($value) || (string) $value === '0') {
          $replacements[$original] = $value;
        }
      }
    }
  }
  // Provide for the site tokens not provided in core.
  elseif ($type == 'site') {
    $wrapper = entity_plus_metadata_site_wrapper();
    $chained_token_names = array();
    foreach ($tokens as $name => $original) {
      $property_name = str_replace('-', '_', $name);
      if (!empty($token_info['tokens'][$type][$property_name]['entity-token'])) {;
        try {
          if (isset($wrapper->$property_name)) {
            $replacement = _entity_token_get_token($wrapper->$name, $options);
            if (isset($replacement)) {
              $replacements[$original] = $replacement;
            }
          }
        } 
        catch (EntityMetadataWrapperException $e) {

        }
      }
      // Care for chained site tokens
      elseif (strpos($name, ':') !== FALSE) {
        $name_parts = explode(':', $name);
        // Check if this is a property
        $property_name = str_replace('-', '_', $name_parts[0]);
        if (!empty($property_name) && isset($wrapper->{$property_name}) ) {
          // Collect the name of the property
          $chained_token_names[$name_parts[0]] = TRUE;
        }
      }
    }
    foreach (array_keys($chained_token_names) as $token_name) {
      $chained_tokens = token_find_with_prefix($tokens, $token_name);
      if (isset($wrapper->$property_name)) {
        $replacements += token_generate('wrapped-field', $chained_tokens, array('wrapped-field' => $wrapper->$property_name), $options);
      }
    } 
  }

  return $replacements;
}

/**
 * Gets and formats the token replacement
 */
function _entity_token_get_token($wrapper, $options) {
  if (!$wrapper || $wrapper->value() === NULL) {
    // Do not provide a replacement if there is no value.
    return NULL;
  }

  if (empty($options['sanitize'])) {
    // When we don't need sanitized tokens decode already sanitizies texts.
    $options['decode'] = TRUE;
  }
  $langcode = isset($options['language']) ? $options['language']->langcode : NULL;

  // If there is a label for a property, e.g. defined by an options list or an
  // entity label, or a taxonomy term, make use of it.
  if ($label = $wrapper->label()) {
    return empty($options['sanitize']) ? $label : check_plain($label);
  }

  switch ($wrapper->type()) {
    case 'integer':
      return $wrapper->value();
    case 'decimal':
      return number_format($wrapper->value(), 2);
    case 'date':
      return format_date($wrapper->value(), 'medium', '', NULL, $langcode);
    case 'duration':
      return format_interval($wrapper->value(), 2, $langcode);
    case 'boolean':
      return $wrapper->value() ? t('true') : t('false');
    case 'uri':
    case 'text':
      return $wrapper->value($options);
  }

  // Care for outputing list values.
  if ($wrapper instanceof EntityListWrapper) {
    $output = array();
    foreach ($wrapper as $item) {
      $output[] = _entity_token_get_token($item, $options);
    }
    return implode(', ', $output);
  }
  // Else we do not have a good string to output, e.g. for struct values. Just
  // output the string representation of the wrapper.
 return (string) $wrapper;
}

/**
 * Helper to determine the token type based on the property info
 */
function entity_token_determine_type($property_info) {
  if (empty($property_info)) {
    return;
  }
  $property_type = !empty($property_info['type']) ? $property_info['type'] : 'text';
  $is_list = strpos($property_type, 'list') !== FALSE;
  if ($is_list) {
    $property_type = entity_plus_property_list_extract_type($property_type);
  } 
  $token_type = _entity_token_get_token_type_from_property_type($property_type);

  return $is_list ? "list<$token_type>" : $token_type;
}

/** 
 * Helper to return the known types based on field type
 */
function _entity_token_get_token_type_from_property_type($field_type) {
  switch ($field_type) {
    case 'taxonomy_term':
      return 'term';
      break;

    case 'datetime':
    case 'date':
    case 'datestamp':
      return 'date';
      break;

    case 'field_item_file':
      return 'file';
      break;

    case 'text_formatted':
    case 'field_item_link':
      return 'struct';
      break;

    case 'field_item_image':
      return 'image';
      break;

    default:
      return $field_type;
  }
}

/** 
 * Check if entity type is one of the core entities.
 * 
 * @param string $entity_type
 *   The entity type.
 * 
 * @return bool
 *   Whether the entity type is one of the core entities.
 */
function _entity_token_is_core_entity($entity_type) {
  return in_array($entity_type, array(
    'node',
    'file',
    'comment',
    'user',
    'taxonomy_term',
    // Since Ubercart defines tokens quite extensively
    // we treat its entities as core to avoid duplications.
    'uc_order',
    'uc_cart_item',
    )
  );
}
