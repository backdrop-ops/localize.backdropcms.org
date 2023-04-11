<?php
/**
 * @file
 * Plugin to provide access control based upon role membership.
 */
class OgNodeGroupLayoutAccess extends LayoutAccessNegatable {
   /**
   * {@inheritdoc}
   */
  function form(&$form, &$form_state) {
    parent::form($form, $form_state);
    $form['negate'] = array(
      '#type' => 'radios',
      '#title' => t('Current node is'),
      '#options' => array(
        0 => t('OG group node'),
        1 => t('Not OG group node'),
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
      return !og_is_group('node', $node);
    } 
    else {
      return og_is_group('node', $node);
    }
  }

  /**
   * {@inheritdoc}
   */
  function summary() {
    return $this->settings['negate'] ? t('Node is not an OG group') : t('Node is an OG group');
  }
}
