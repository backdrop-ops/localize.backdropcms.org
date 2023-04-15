<?php
/**
 * @file
 * Plugin to provide access control based upon OG group status.
 */

class OgNodeGroupContentLayoutAccess extends LayoutAccessNegatable {
   /**
   * {@inheritdoc}
   */
  function form(&$form, &$form_state) {
    parent::form($form, $form_state);
    $form['negate'] = array(
      '#type' => 'radios',
      '#title' => t('Current node is'),
      '#options' => array(
        0 => t('OG group content node'),
        1 => t('Not OG group content node'),
      ),
      '#weight' => 100,
      '#default_value' => (int) $this->settings['negate'],
    );
  }

  /**
   * {@inheritdoc}
   */
  function checkAccess() {
    $node = $this->contexts['node']->data;
    if ($this->settings['negate']) {
      return !og_is_group_content_type('node', $node->type);
    } 
    else {
      return og_is_group_content_type('node', $node->type);
    }
  }

  /**
   * {@inheritdoc}
   */
  function summary() {
    return $this->settings['negate'] ? t('Node is not OG group content') : t('Node is OG group content');
  }
}
