<?php
/**
 * @file
 * Contains data type related forms.
 */

/**
 * Interface for data types providing a direct input form.
 */
interface RulesDataDirectInputFormInterface {

  /**
   * Constructs the direct input form.
   *
   * @return array
   *   The direct input form.
   */
  public static function inputForm($name, $info, $settings, RulesPlugin $element);

  /**
   * Render the configured value.
   *
   * @return array
   *   A renderable array.
   */
  public static function render($value);

}

/**
 * Interface for data UI classes providing an options list.
 */
interface RulesDataInputOptionsListInterface extends RulesDataDirectInputFormInterface {

  /**
   * Returns the options list for the data type.
   *
   * For retrieving information about the used data type and parameter, the
   * helper RulesDataUI::getTypeInfo() may be used as following:
   * @code
   *   list($type, $parameter_info) = RulesDataUI::getTypeInfo($element, $name);
   * @endcode
   *
   * @param RulesPlugin $element
   *   The rules element to get the options for.
   * @param string $name
   *   The name of the parameter for which to get options.
   *
   * @return array
   *   An array of options as used by hook_options_list().
   */
  public static function optionsList(RulesPlugin $element, $name);

}

/**
 * Default UI related class for data types.
 */
class RulesDataUI {

  /**
   * Specifies the default input mode per data type.
   */
  public static function getDefaultMode() {
    return 'selector';
  }

  /**
   * Provides the selection form for a parameter.
   */
  public static function selectionForm($name, $info, $settings, RulesPlugin $element) {
    if (!isset($settings[$name . ':select'])) {
      $settings[$name . ':select'] = '';
      $vars = $element->availableVariables();
      // Default to variables with the same name as the parameter.
      if (isset($vars[$name])) {
        $settings[$name . ':select'] = $name;
      }
      // If there is only one match, use it by default.
      elseif (count($matches = RulesData::matchingDataSelector($vars, $info, '', 1, FALSE)) == 1) {
        $settings[$name . ':select'] = rules_array_key($matches);
      }
    }
    $form[$name . ':select'] = array(
      '#type' => 'rules_data_selection',
      '#title' => t('Data selector'),
      '#default_value' => $settings[$name . ':select'],
      '#required' => empty($info['optional']),
      '#autocomplete_path' => RulesPluginUI::path($element->root()->name, 'autocomplete' . '/' . $name),
      // Make the autocomplete textfield big enough so that it can display
      // descriptions without word wraps.
      '#size' => 75,
      '#description' => t("The data selector helps you drill down into the data available to Rules. <em>To make entity fields appear in the data selector, you may have to use the condition 'Entity has field' (or 'Entity is of bundle').</em> More useful tips about data selection are available in <a href='@url'>the online documentation</a>.",
        array('@url' => rules_external_help('data-selection'))),
    );

    $cache = rules_get_cache();
    $form['types_help'] = array(
      '#theme' => 'rules_settings_help',
      '#heading' => t('Data types'),
    );
    if ($info['type'] == '*') {
      $type_labels[] = t('any');
    }
    else {
      $types = is_array($info['type']) ? $info['type'] : array($info['type']);
      $type_labels = array();
      foreach ($types as $type) {
        $type_labels[] = backdrop_ucfirst(isset($cache['data_info'][$type]['label']) ? $cache['data_info'][$type]['label'] : $type);
      }
    }
    $form['types_help']['#text'] = format_plural(count($type_labels), 'Select data of the type %types.', 'Select data of the types %types.', array('%types' => implode(', ', $type_labels)));

    if (!empty($info['translatable'])) {
      if (empty($info['custom translation language'])) {
        $text = t('If a multilingual data source (i.e. a translatable field) is given, the argument is translated to the current interface language.');
      }
      else {
        $text = t('If a multilingual data source (i.e. a translatable field) is given, the argument is translated to the configured language.');
      }
      $form['translation'] = array(
        '#theme' => 'rules_settings_help',
        '#text' => $text,
        '#heading' => t('Translation'),
      );
    }
    $form['help'] = array(
      '#theme' => 'rules_data_selector_help',
      '#variables' => $element->availableVariables(),
      '#parameter' => $info,
    );

    // Add data processor.
    $settings += array($name . ':process' => array());
    $form[$name . ':process'] = array();
    RulesDataProcessor::attachForm($form[$name . ':process'], $settings[$name . ':process'], $info, $element->availableVariables());
    return $form;
  }

  /**
   * Renders the value with a label if an options list is available.
   *
   * Used for data UI classes implementing the
   * RulesDataDirectInputFormInterface.
   *
   * In case an options list is available, the usual render() method won't
   * be invoked, instead the selected entry is rendered via this method.
   *
   * @todo Refactor to avoid implementations have to care about
   * option lists when generating the form, but not when rendering values.
   */
  public static function renderOptionsLabel($value, $name, $info, RulesPlugin $element) {
    if (!empty($info['options list'])) {
      $element->call('loadBasicInclude');
      $options = entity_plus_property_options_flatten(call_user_func($info['options list'], $element, $name));
      if (!is_array($value) && isset($options[$value])) {
        $value = $options[$value];
      }
      elseif (is_array($value)) {
        foreach ($value as $key => $single_value) {
          if (isset($options[$single_value])) {
            $value[$key] = $options[$single_value];
          }
        }
        $value = implode(', ', $value);
      }
      return array(
        'content' => array('#markup' => check_plain($value)),
        '#attributes' => array('class' => array('rules-parameter-options-entry')),
      );
    }
  }

  /**
   * Returns the data type and parameter information for the given arguments.
   *
   * This helper may be used by options list callbacks operation at data-type
   * level, see RulesDataInputOptionsListInterface.
   */
  public static function getTypeInfo(RulesPlugin $element, $name) {
    $parameters = $element->pluginParameterInfo();
    return array($parameters[$name]['type'], $parameters[$name]);
  }

}

/**
 * UI for textual data.
 */
class RulesDataUIText extends RulesDataUI implements RulesDataDirectInputFormInterface {

  /**
   * Overrides RulesDataUI::getDefaultMode().
   */
  public static function getDefaultMode() {
    return 'input';
  }

  /**
   * Implements RulesDataDirectInputFormInterface::inputForm().
   */
  public static function inputForm($name, $info, $settings, RulesPlugin $element) {
    if (!empty($info['options list'])) {
      // Make sure the .rules.inc of the providing module is included as the
      // options list callback may reside there.
      $element->call('loadBasicInclude');
      $form[$name] = array(
        '#type' => 'select',
        '#options' => call_user_func($info['options list'], $element, $name),
      );
    }
    else {
      $form[$name] = array(
        '#type' => 'textarea',
        '#rows' => 3,
      );
      RulesDataInputEvaluator::attachForm($form, $settings, $info, $element->availableVariables());
    }
    $settings += array($name => isset($info['default value']) ? $info['default value'] : NULL);
    $form[$name] += array(
      '#title' => t('Value'),
      '#default_value' => $settings[$name],
      '#required' => empty($info['optional']),
      '#after_build' => array('rules_ui_element_fix_empty_after_build'),
    );
    return $form;
  }

  /**
   * Implements RulesDataDirectInputFormInterface::render().
   */
  public static function render($value) {
    return array(
      'content' => array('#markup' => check_plain($value)),
      '#attributes' => array('class' => array('rules-parameter-text')),
    );
  }

}

/**
 * UI for text tokens.
 */
class RulesDataUITextToken extends RulesDataUIText {

  /**
   * Implements RulesDataDirectInputFormInterface::inputForm().
   */
  public static function inputForm($name, $info, $settings, RulesPlugin $element) {
    $form = parent::inputForm($name, $info, $settings, $element);
    if ($form[$name]['#type'] == 'textarea') {
      $form[$name]['#element_validate'][] = 'rules_ui_element_token_validate';
      $form[$name]['#description'] = t('May only contain lowercase letters, numbers, and underscores and has to start with a letter.');
      $form[$name]['#rows'] = 1;
    }
    return $form;
  }

}

/**
 * UI for formatted text.
 */
class RulesDataUITextFormatted extends RulesDataUIText {

  /**
   * Implements RulesDataDirectInputFormInterface::inputForm().
   */
  public static function inputForm($name, $info, $settings, RulesPlugin $element) {
    $form = parent::inputForm($name, $info, $settings, $element);
    $settings += array($name => isset($info['default value']) ? $info['default value'] : array('value' => NULL, 'format' => NULL));

    $form[$name]['#type'] = 'text_format';
    $form[$name]['#base_type'] = 'textarea';
    $form[$name]['#default_value'] = $settings[$name]['value'];
    $form[$name]['#format'] = $settings[$name]['format'];
    return $form;
  }

  /**
   * Implements RulesDataDirectInputFormInterface::render().
   */
  public static function render($value) {
    return array(
      'content' => array('#markup' => check_plain($value['value'])),
      '#attributes' => array('class' => array('rules-parameter-text-formatted')),
    );
  }

}

/**
 * UI for decimal data.
 */
class RulesDataUIDecimal extends RulesDataUIText {

  /**
   * Implements RulesDataDirectInputFormInterface::inputForm().
   */
  public static function inputForm($name, $info, $settings, RulesPlugin $element) {
    $form = parent::inputForm($name, $info, $settings, $element);
    if (empty($info['options list'])) {
      $form[$name]['#type'] = 'number';
    }
    $form[$name]['#element_validate'][] = 'rules_ui_element_decimal_validate';
    $form[$name]['#rows'] = 1;
    return $form;
  }

}

/**
 * UI for integers.
 */
class RulesDataUIInteger extends RulesDataUIText {

  /**
   * Implements RulesDataDirectInputFormInterface::inputForm().
   */
  public static function inputForm($name, $info, $settings, RulesPlugin $element) {
    $form = parent::inputForm($name, $info, $settings, $element);
    if (empty($info['options list'])) {
      $form[$name]['#type'] = 'number';
      $form[$name]['#step'] = 1;
    }
    else {
      $form[$name]['#element_validate'][] = 'rules_ui_element_integer_validate';
    }
    return $form;
  }

}

/**
 * UI for IP addresses.
 */
class RulesDataUIIPAddress extends RulesDataUIText {

  /**
   * Implements RulesDataDirectInputFormInterface::inputForm().
   */
  public static function inputForm($name, $info, $settings, RulesPlugin $element) {
    $form = parent::inputForm($name, $info, $settings, $element);
    if (empty($info['options list'])) {
      $form[$name]['#type'] = 'textfield';
      $form[$name]['#description'] = t('If not provided, the IP address of the current user will be used.');
    }
    $form[$name]['#element_validate'][] = 'rules_ui_element_ip_address_validate';
    $form[$name]['#rows'] = 1;
    return $form;
  }

}

/**
 * UI for boolean data.
 */
class RulesDataUIBoolean extends RulesDataUI implements RulesDataDirectInputFormInterface {

  /**
   * Overrides RulesDataUI::getDefaultMode().
   */
  public static function getDefaultMode() {
    return 'input';
  }

  /**
   * Implements RulesDataDirectInputFormInterface::inputForm().
   */
  public static function inputForm($name, $info, $settings, RulesPlugin $element) {
    $settings += array($name => isset($info['default value']) ? $info['default value'] : NULL);
    // Note: Due to the checkbox even optional parameter always receive a value.
    $form[$name] = array(
      '#type' => 'radios',
      '#default_value' => $settings[$name],
      '#options' => array(
        TRUE => t('@label: True.', array('@label' => $info['label'])),
        FALSE => t('@label: False.', array('@label' => $info['label'])),
      ),
    );
    return $form;
  }

  /**
   * Implements RulesDataDirectInputFormInterface::render().
   */
  public static function render($value) {
    return array(
      'content' => array('#markup' => !empty($value) ? t('true') : t('false')),
      '#attributes' => array('class' => array('rules-parameter-boolean')),
    );
  }

}

/**
 * UI for dates.
 */
class RulesDataUIDate extends RulesDataUIText {

  /**
   * Implements RulesDataDirectInputFormInterface::inputForm().
   */
  public static function inputForm($name, $info, $settings, RulesPlugin $element) {
    $settings += array($name => isset($info['default value']) ? $info['default value'] : (empty($info['optional']) ? gmdate('Y-m-d H:i:s', time()) : NULL));

    // Convert any configured timestamp into a readable format.
    if (is_numeric($settings[$name])) {
      $settings[$name] = gmdate('Y-m-d H:i:s', $settings[$name]);
    }
    $form = parent::inputForm($name, $info, $settings, $element);
    $form[$name]['#type'] = 'textfield';
    $form[$name]['#element_validate'][] = 'rules_ui_element_date_validate';
    // Note that the date input evaluator takes care for parsing dates using
    // strtotime() into a timestamp, which is the internal date format.
    $form[$name]['#description'] = t('The date in GMT. You may enter a fixed time (like %format) or any other values in GMT known by the PHP !strtotime function (like "+1 day"). Relative dates like "+1 day" or "now" relate to the evaluation time.',
      array('%format' => gmdate('Y-m-d H:i:s', time() + 86400),
            '!strtotime' => l('strtotime()', 'http://php.net/strtotime')));

    // @todo Leverage the jquery datepicker+timepicker once a module providing
    // The timepicker is available.
    return $form;
  }

  /**
   * Implements RulesDataDirectInputFormInterface::render().
   */
  public static function render($value) {
    if (is_numeric($value)) {
      $value = format_date($value, 'short', '', 'GMT') . ' (' . t('GMT') . ')';
    }
    else {
      $value = check_plain($value);
    }
    return array(
      'content' => array('#markup' => $value),
      '#attributes' => array('class' => array('rules-parameter-date')),
    );
  }

}

/**
 * UI for duration type parameter.
 */
class RulesDataUIDuration extends RulesDataUIText {

  /**
   * Implements RulesDataDirectInputFormInterface::inputForm().
   */
  public static function inputForm($name, $info, $settings, RulesPlugin $element) {
    $form = parent::inputForm($name, $info, $settings, $element);
    $form[$name]['#type'] = 'rules_duration';
    $form[$name]['#after_build'][] = 'rules_ui_element_duration_after_build';
    return $form;
  }

  /**
   * Implements RulesDataDirectInputFormInterface::render().
   */
  public static function render($value) {
    $value = is_numeric($value) ? format_interval($value) : check_plain($value);
    return array(
      'content' => array('#markup' => $value),
      '#attributes' => array('class' => array('rules-parameter-duration')),
    );
  }

}

/**
 * UI for the URI type parameter.
 */
class RulesDataUIURI extends RulesDataUIText {

  /**
   * Implements RulesDataDirectInputFormInterface::inputForm().
   */
  public static function inputForm($name, $info, $settings, RulesPlugin $element) {
    $form = parent::inputForm($name, $info, $settings, $element);
    $form[$name]['#rows'] = 1;
    $form[$name]['#description'] = t('You may enter relative URLs like %url as well as absolute URLs like %absolute-url.', array('%url' => 'user/login?destination=node', '%absolute-url' => 'https://backdropcms.org'));
    return $form;
  }

}

/**
 * UI for lists of textual data.
 */
class RulesDataUIListText extends RulesDataUIText {

  /**
   * Overrides RulesDataUI::getDefaultMode().
   */
  public static function getDefaultMode() {
    return 'input';
  }

  /**
   * Implements RulesDataDirectInputFormInterface::inputForm().
   *
   * @todo This does not work for inputting textual values including "\n".
   */
  public static function inputForm($name, $info, $settings, RulesPlugin $element) {
    $settings += array($name => isset($info['default value']) ? $info['default value'] : NULL);
    $form = parent::inputForm($name, $info, $settings, $element);

    if ($form[$name]['#type'] == 'textarea') {
      // Fix up the value to be an array during after build.
      $form[$name]['#delimiter'] = "\n";
      $form[$name]['#after_build'][] = 'rules_ui_list_textarea_after_build';
      $form[$name]['#pre_render'][] = 'rules_ui_list_textarea_pre_render';
      $form[$name]['#default_value'] = !empty($settings[$name]) ? implode("\n", $settings[$name]) : NULL;
      $form[$name]['#description'] = t('A list of values, one on each line.');
    }
    else {
      $form[$name]['#multiple'] = TRUE;
    }
    return $form;
  }

  /**
   * Implements RulesDataDirectInputFormInterface::render().
   */
  public static function render($value) {
    return array(
      'content' => array('#markup' => check_plain(implode(', ', $value))),
      '#attributes' => array('class' => array('rules-parameter-list')),
    );
  }

}

/**
 * UI for lists of integers.
 */
class RulesDataUIListInteger extends RulesDataUIListText {

  /**
   * Implements RulesDataDirectInputFormInterface::inputForm().
   */
  public static function inputForm($name, $info, $settings, RulesPlugin $element) {
    $settings += array($name => isset($info['default value']) ? $info['default value'] : NULL);
    $form = parent::inputForm($name, $info, $settings, $element);

    if ($form[$name]['#type'] == 'textarea') {
      $form[$name]['#description'] = t('A list of integers, separated by commas. E.g. enter "1, 2, 3".');
      $form[$name]['#delimiter'] = ',';
      $form[$name]['#default_value'] = !empty($settings[$name]) ? implode(", ", $settings[$name]) : NULL;
      $form[$name]['#element_validate'][] = 'rules_ui_element_integer_list_validate';
      $form[$name]['#rows'] = 1;
    }
    return $form;
  }

}

/**
 * UI for lists of tokens.
 */
class RulesDataUIListToken extends RulesDataUIListInteger {

  /**
   * Implements RulesDataDirectInputFormInterface::inputForm().
   */
  public static function inputForm($name, $info, $settings, RulesPlugin $element) {
    $form = parent::inputForm($name, $info, $settings, $element);

    if ($form[$name]['#type'] == 'textarea') {
      $form[$name]['#description'] = t('A list of text tokens, separated by commas. E.g. enter "one, two, three".');
      $form[$name]['#element_validate'] = array('rules_ui_element_token_list_validate');
    }
    return $form;
  }

}

/**
 * UI for entity-based data types.
 */
class RulesDataUIEntity extends RulesDataUIText {

  /**
   * Overrides RulesDataUI::getDefaultMode().
   */
  public static function getDefaultMode() {
    return 'selector';
  }

  /**
   * Implements RulesDataDirectInputFormInterface::inputForm().
   */
  public static function inputForm($name, $info, $settings, RulesPlugin $element) {
    $form = parent::inputForm($name, $info, $settings, $element);
    if (empty($info['options list'])) {
      $entity_info = entity_get_info($info['type']);
      if (empty($entity_info['entity keys']['name'])) {
        $form[$name]['#type'] = 'number';
        $form[$name]['#step'] = 1;
      }
      else {
        $form[$name]['#type'] = 'textfield';
      }
      $form[$name]['#title'] = t('@entity identifier', array('@entity' => $entity_info['label']));
      $entity_label = strtolower($entity_info['label'][0]) . substr($entity_info['label'], 1);
      $form[$name]['#description'] = t('Specify an identifier of a @entity.', array('@entity' => $entity_label));
    }
    return $form;
  }

}

/**
 * UI for exportable entity-based data types.
 */
class RulesDataUIEntityExportable extends RulesDataUIEntity {

  /**
   * Overrides RulesDataUI::getDefaultMode().
   */
  public static function getDefaultMode() {
    return 'input';
  }

}

/**
 * Data UI variant displaying a select list of available bundle entities.
 *
 * This is used for "bundle entities" implemented via the 'bundle of' feature
 * of entity.module.
 */
class RulesDataUIBundleEntity extends RulesDataUIEntity implements RulesDataInputOptionsListInterface {

  /**
   * Overrides RulesDataUI::getDefaultMode().
   */
  public static function getDefaultMode() {
    return 'input';
  }

  /**
   * Implements RulesDataInputOptionsListInterface::optionsList().
   */
  public static function optionsList(RulesPlugin $element, $name) {
    list($data_type, $parameter_info) = RulesDataUI::getTypeInfo($element, $name);
    $bundles = array();
    $entity_info = entity_get_info();
    $bundle_of_type = $entity_info[$data_type]['bundle of'];
    if (isset($entity_info[$bundle_of_type]['bundles'])) {
      foreach ($entity_info[$bundle_of_type]['bundles'] as $bundle_name => $bundle_info) {
        $bundles[$bundle_name] = $bundle_info['label'];
      }
    }
    return $bundles;
  }

}

/**
 * UI for taxonomy vocabularies.
 *
 * @see RulesTaxonomyVocabularyWrapper
 */
class RulesDataUITaxonomyVocabulary extends RulesDataUIEntity implements RulesDataInputOptionsListInterface {

  /**
   * Overrides RulesDataUI::getDefaultMode().
   */
  public static function getDefaultMode() {
    return 'input';
  }

  /**
   * Implements RulesDataInputOptionsListInterface::optionsList().
   */
  public static function optionsList(RulesPlugin $element, $name) {
    $options = array();
    foreach (taxonomy_vocabulary_load_multiple(FALSE) as $machine_name => $vocab) {
      $options[$machine_name] = $vocab->name;
    }
    return $options;
  }

}

/**
 * UI for lists of entity-based data types.
 */
class RulesDataUIListEntity extends RulesDataUIListInteger {

  /**
   * Implements RulesDataDirectInputFormInterface::inputForm().
   */
  public static function inputForm($name, $info, $settings, RulesPlugin $element) {
    $form = parent::inputForm($name, $info, $settings, $element);
    if (empty($info['options list'])) {

      $entity_info = entity_get_info(entity_plus_property_list_extract_type($info['type']));
      if (!empty($entity_info['entity keys']['name'])) {
        $form[$name]['#element_validate'] = array('rules_ui_element_token_list_validate');
      }
      $form[$name]['#title'] = t('@entity identifiers', array('@entity' => $entity_info['label']));
      $entity_label = strtolower($entity_info['label'][0]) . substr($entity_info['label'], 1);
      $form[$name]['#description'] = t('Specify a comma-separated list of identifiers of @entity entities.', array('@entity' => $entity_label));
    }
    return $form;
  }

}
