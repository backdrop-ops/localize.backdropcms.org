<?php
/**
 *  FieldBlock extends Block
 *
 *  Provides entity field values in configurable blocks.
 */
class OgNodeCreateLinks extends Block {
  function __construct($plugin_name, array $data = array()) {
    parent::__construct($plugin_name, $data);

    $this->settings += array(
      'field_name' => OG_AUDIENCE_FIELD,
      'types' => array(),
    );
  }

  /**
   *  Sets block subject on block view.
   */
  function getTitle() {
    $title = parent::getTitle();

    if (empty($title) && $this->settings['title_display'] === LAYOUT_TITLE_DEFAULT) {
      return t('Content create links');
    }

    return $title;
  }

  /**
   *  Sets block content on block view.
   */
  function getContent() {

    if (empty($this->contexts['node']->data)) {
      return FALSE;
    }
  
    $node = $this->contexts['node']->data;
    $links = og_node_create_links('node', $node->nid, $this->settings['field_name'], NULL, !empty($this->settings['types']) ? $this->settings['types'] : NULL);
    if (!$links) {
      return FALSE;
    }
  
    return $links;
  }

  /**
   *  Builds the block's configuration form.
   */
  function form(&$form, &$form_state) {
    parent::form($form, $form_state);

    $conf = $this->settings;

    $options = array();
    foreach (field_info_fields() as $field_name => $field) {
      if (!og_is_group_audience_field($field_name)) {
        continue;
      }

      if ($field['settings']['target_type'] != 'node') {
        continue;
      }
      // Get the best matching field name.
      $options[$field_name] = $this->_og_field_label($field_name) . " ($field_name)";
    }

    $form['field_name'] = array(
      '#title' => t('Field name'),
      '#type' => 'select',
      '#options' => $options,
      '#default_value' => $conf['field_name'],
      '#description' => t('The group audience field to prepopulate.'),
      '#required' => TRUE,
    );

    $options = array();
    foreach (node_type_get_types() as $type) {
      if (og_is_group_content_type('node', $type->type)) {
        $options[$type->type] = check_plain($type->name);
      }
    }
    $form['types'] = array(
      '#title' => t('Restrict to content types'),
      '#type' => 'checkboxes',
      '#options' => $options,
      '#default_value' => $conf['types'],
      '#description' => t('If left empty, all possible content types are shown.'),
    );
    return $form;
    }

  /**
   * Submit handler to save the form settings.
   */
  function formSubmit($form, &$form_state) {
    parent::formSubmit($form, $form_state);
    $this->settings['field_name'] = $form_state['values']['field_name'];
    $this->settings['types'] = array_filter($form_state['values']['types']);
  }

  /**
   * Returns the label of a certain field.
   *
   * Cribbed from Views.
   */
  function _og_field_label($field_name) {
    $label_counter = array();
    // Count the amount of instances per label per field.
    $instances = field_info_instances();
    foreach ($instances as $entity_type) {
      foreach ($entity_type as $bundle) {
        if (isset($bundle[$field_name])) {
          $label_counter[$bundle[$field_name]['label']] = isset($label_counter[$bundle[$field_name]['label']]) ? ++$label_counter[$bundle[$field_name]['label']] : 1;
        }
      }
    }
    if (empty($label_counter)) {
      return $field_name;
    }
    // Sort the field lables by it most used label and return the most used one.
    arsort($label_counter);
    $label_counter = array_keys($label_counter);
    return $label_counter[0];
  }
}
