<?php

/**
 * @file
 * Implements a message display action.
 */

/**
 * Implements hook_vboaction_info().
 */
function views_bulk_operations_node_unpublish_by_keyword_vboaction_info() {
  return array(
    'views_bulk_operations_node_unpublish_by_keyword_vboaction' => array(
      'type' => 'node',
      'label' => t('Unpublish content containing keyword(s)'),
      'configurable' => TRUE,
      //'triggers' => array('node_presave', 'node_insert', 'node_update'),
      'vbo' => TRUE,
    ),
  );
}

/**
 * Generates settings form for views_bulk_operations_node_unpublish_by_keyword_vboaction().
 *
 * @param array $context
 *   Array of additional information about what triggered this action.
 *
 * @return array
 *   A form array.
 *
 * @see views_bulk_operations_node_unpublish_by_keyword_vboaction_submit()
 */
function views_bulk_operations_node_unpublish_by_keyword_vboaction_form($context) {
  $form['keywords'] = array(
    '#title' => t('Keywords'),
    '#type' => 'textarea',
    '#description' => t('The content will be unpublished if it contains any of the phrases above. Use a case-sensitive, comma-separated list of phrases. Example: funny, bungee jumping, "Company, Inc."'),
    '#default_value' => isset($context['keywords']) ? backdrop_implode_tags($context['keywords']) : '',
  );
  return $form;
}

/**
 * Saves settings form for views_bulk_operations_node_unpublish_by_keyword_vboaction().
 */
function views_bulk_operations_node_unpublish_by_keyword_vboaction_submit($form, $form_state) {
  return array('keywords' => backdrop_explode_tags($form_state['values']['keywords']));
}

/**
 * Unpublishes a node containing certain keywords.
 *
 * @param $node
 *   A node object to modify.
 * @param $context
 *   Array with the following elements:
 *   - 'keywords': Array of keywords. If any keyword is present in the rendered
 *     node, the node's status flag is set to unpublished.
 *
 * @ingroup actions
 */
function views_bulk_operations_node_unpublish_by_keyword_vboaction($node, $context) {
  foreach ($context['keywords'] as $keyword) {
    $elements = node_view(clone $node);
    if (strpos(backdrop_render($elements), $keyword) !== FALSE || strpos($node->title, $keyword) !== FALSE) {
      $node->status = NODE_NOT_PUBLISHED;
      watchdog('action', 'Set @type %title to unpublished.', array('@type' => node_type_get_name($node), '%title' => $node->title));
      break;
    }
  }
}
