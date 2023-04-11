<?php

/**
 * @file
 * Implements a message display action.
 */

/**
 * Implements hook_vboaction_info().
 */
function views_bulk_operations_comment_unpublish_by_keyword_vboaction_info() {
  return array(
    'views_bulk_operations_comment_unpublish_by_keyword_vboaction' => array(
      'label' => t('Unpublish comment containing keyword(s)'),
      'type' => 'comment',
      'configurable' => TRUE,
      'behavior' => array('changes_property'),
      //'triggers' => array('comment_presave', 'comment_insert', 'comment_update'),
      'vbo' => TRUE,
    ),
  );
}

/**
 * Unpublishes a comment if it contains certain keywords.
 *
 * @param object $comment
 *   Comment object to modify.
 * @param array $context
 *   Array with components:
 *   - 'keywords': Keywords to look for. If the comment contains at least one
 *     of the keywords, it is unpublished.
 *
 * @see views_bulk_operations_comment_unpublish_by_keyword_vboaction_form()
 * @see views_bulk_operations_comment_unpublish_by_keyword_vboaction_submit()
 */
function views_bulk_operations_comment_unpublish_by_keyword_vboaction($comment, $context) {
  $node = node_load($comment->nid);
  $build = comment_view($comment, $node);
  $text = backdrop_render($build);
  foreach ($context['keywords'] as $keyword) {
    if (strpos($text, $keyword) !== FALSE) {
      $comment->status = COMMENT_NOT_PUBLISHED;
      comment_save($comment);
      watchdog('action', 'Unpublished comment %subject.', array('%subject' => $comment->subject));
      break;
    }
  }
}

/**
 * Form builder; Prepare a form for blacklisted keywords.
 *
 * @ingroup forms
 * @see views_bulk_operations_comment_unpublish_by_keyword_vboaction()
 * @see views_bulk_operations_comment_unpublish_by_keyword_vboaction_submit()
 */
function views_bulk_operations_comment_unpublish_by_keyword_vboaction_form($context) {
  $form['keywords'] = array(
    '#title' => t('Keywords'),
    '#type' => 'textarea',
    '#description' => t('The comment will be unpublished if it contains any of the phrases above. Use a case-sensitive, comma-separated list of phrases. Example: funny, bungee jumping, "Company, Inc."'),
    '#default_value' => isset($context['keywords']) ? backdrop_implode_tags($context['keywords']) : '',
  );

  return $form;
}

/**
 * Process views_bulk_operations_comment_unpublish_by_keyword_vboaction_form form submissions.
 *
 * @see views_bulk_operations_comment_unpublish_by_keyword_vboaction()
 */
function views_bulk_operations_comment_unpublish_by_keyword_vboaction_submit($form, $form_state) {
  return array('keywords' => backdrop_explode_tags($form_state['values']['keywords']));
}
