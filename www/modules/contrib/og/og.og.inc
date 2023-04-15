<?php

/**
 * @file
 * Organic Groups plugins declarations.
 *
 * NOTE: This file is never called in Backdrop. It's here to provide a place to
 * store old ctools plugins that are not yet incorporated in Backdrop OG.
 * At the moment, the last one to be ported is og_membership_from_group_and_user
 */

/**
 * Implements hook_og_info().
 *
 */

function og_og_info() {
  $info = array(
    'relationships' => array(
      'og_membership_from_group_and_user' => array(
        'title' => t('OG membership from a group node and user'),
        'keyword' => 'og_membership',
        'description' => t('Get the OG membership context from an a node that is a group and user.'),
        'required context' => array(
          new og_context_required(t('Node'), 'node'),
          new og_context_required(t('User'), 'user'),
        ),
        'context' => 'og_membership_from_group_and_user_context',
        'file' => 'plugins/access/og_perm.inc',
      ),
    ),
  );

  return $info;
}
