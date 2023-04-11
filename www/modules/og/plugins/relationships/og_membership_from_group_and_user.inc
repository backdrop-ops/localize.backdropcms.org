<?php


/**
 * @file
 * Plugin to provide a relationship handler for OG membership from group and
 * user.
 */

/**
 * Return a new context based on an existing context.
 */
function og_membership_from_group_and_user_context($context, $conf) {
  list($node_context, $user_context) = $context;

  // If unset it wants a generic, unfilled context, which is just NULL.
  if (empty($node_context->data)) {
    return new og_context('entity:og_membership', NULL);
  }

  if (empty($user_context) || empty($user_context->data)) {
    return;
  }

  $node = $node_context->data;
  $account = clone $user_context->data;

  if ($og_membership = og_get_membership('node', $node->nid, 'user', $account->uid)) {
    return new og_context('entity:og_membership', $og_membership);
  }
}
