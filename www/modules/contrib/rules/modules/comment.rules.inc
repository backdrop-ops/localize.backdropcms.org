<?php
/**
 * @file
 * Rules integration for the comment module.
 *
 * @addtogroup rules
 *
 * @{
 */

/**
 * Implements hook_rules_event_info().
 */
function rules_comment_event_info() {
  $defaults = array(
    'group' => t('comment'),
    'module' => 'comment',
    'access callback' => 'rules_comment_integration_access',
    'class' => 'RulesCommentEventHandler',
  );
  return array(
    'comment_insert' => $defaults + array(
      'label' => t('After saving a new comment'),
      'variables' => array(
        'comment' => array('type' => 'comment', 'label' => t('created comment')),
      ),
    ),
    'comment_update' => $defaults + array(
      'label' => t('After updating an existing comment'),
      'variables' => array(
        'comment' => array(
          'type' => 'comment',
          'label' => t('updated comment'),
        ),
        'comment_unchanged' => array(
          'type' => 'comment',
          'label' => t('unchanged comment'),
          'handler' => 'rules_events_entity_unchanged',
        ),
      ),
    ),
    'comment_presave' => $defaults + array(
      'label' => t('Before saving a comment'),
      'variables' => array(
        'comment' => array(
          'type' => 'comment',
          'label' => t('saved comment'),
          'skip save' => TRUE,
        ),
        'comment_unchanged' => array(
          'type' => 'comment',
          'label' => t('unchanged comment'),
          'handler' => 'rules_events_entity_unchanged',
        ),
      ),
    ),
    'comment_view' => $defaults + array(
      'label' => t('A comment is viewed'),
      'variables' => array(
        'comment' => array('type' => 'comment', 'label' => t('viewed comment')),
      ),
      'help' => t("Note that if backdrop's page cache is enabled, this event won't be generated for pages served from cache."),
    ),
    'comment_delete' => $defaults + array(
      'label' => t('After deleting a comment'),
      'variables' => array(
        'comment' => array('type' => 'comment', 'label' => t('deleted comment')),
      ),
    ),
  );
}

/**
 * Comment integration access callback.
 */
function rules_comment_integration_access($type, $name) {
  if ($type == 'event' || $type == 'condition') {
    return entity_plus_access('view', 'comment');
  }
}

/**
 * Event handler support comment bundle event settings.
 */
class RulesCommentEventHandler extends RulesEventHandlerEntityBundle {

  /**
   * Returns the label to use for the bundle property.
   *
   * @return string
   *   Returns the label to use for the bundle property.
   */
  protected function getBundlePropertyLabel() {
    return t('type');
  }

}

/**
 * @} End of "addtogroup rules"
 */
