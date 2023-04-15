<?php
/**
 *  FieldBlock extends Block
 *
 *  Provides entity field values in configurable blocks.
 */
class OgUiMembershipLinks extends Block {

  /**
   *  Sets block subject on block view.
   */
  function getTitle() {
    $title = parent::getTitle();

    if (empty($title) && $this->settings['title_display'] === LAYOUT_TITLE_DEFAULT) {
      return t('OG membership links');
    }

    return $title;
  }

  /**
   *  Sets block content on block view.
   */
  function getContent() {
    $conf = $this->settings;

    global $user;
    if (empty($this->contexts['node']->data) || !og_is_group('node', $this->contexts['node']->data)) {
      return FALSE;
    }

    $group = clone $this->contexts['node']->data;
    // $entity = $group->getEntity();
    if (!og_get_membership('node', $group->nid, 'user', $user->uid)) {
      if (!og_user_access('node', $group->nid, 'subscribe') && !og_user_access('node', $group->nid, 'subscribe without approval')) {
        // User doesn't have access.
        return;
      }

      $subscribe_access = og_user_access('node', $group->nid, 'subscribe');

      $items = array();
      $options = array();

      $subscribe_path = 'group/node/' . $group->nid . '/subscribe';
      if ($user->uid) {
        $path = $subscribe_path;
      }
      else {
        // User is anonymous, so redirect to subscribe after login.
        $path = 'user/login';
        $options['query']['destination'] = $subscribe_path;
      }

      if (!$subscribe_access) {
        $items[] = array('data' => l(t('Request group membership'), $path, $options));
      }
      else {
        $items[] = array('data' => l(t('Subscribe to group'), $path, $options));
      }
    }
    else {
      // User already has membership.
      if (!og_user_access('node', $group->nid, 'unsubscribe')) {
        // User can't unsubscribe.
        return;
      }

      if (!empty($group->uid) && $group->uid == $user->uid) {
        // User is the group manager.
        return;
      }

      $items[] = array('data' => l(t('Unsubscribe from group'), 'group/node/' . $group->nid . '/unsubscribe'));
      $title = t('Unsubscribe link');
    }

    $content = array(
      '#theme' => 'item_list',
      '#items' => $items,
    );
    return $content;
  }

}
