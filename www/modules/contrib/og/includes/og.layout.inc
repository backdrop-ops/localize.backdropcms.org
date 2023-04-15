<?php
/**
 * @file
 * Contains layout hook implementations provided by OG.
 */

/** 
 * Implements hook_layout_access_info().
 */
function og_layout_access_info() {
  $info['og_node_group'] = array(
    'title' => t("OG: Node is an OG group"),
    'description' => t('Control access by whether or not a node is of type "Group".'),
    'class' => 'OgNodeGroupLayoutAccess',
    'required contexts' => array(
      'node' => 'node',
    ),
  );
  $info['og_node_group_content'] = array(
    'title' => t("OG: Node is an OG group content"),
    'description' => t('Control access by whether or not a node is of type "Group content".'),
    'class' => 'OgNodeGroupContentLayoutAccess',
    'required contexts' => array(
      'node' => 'node',)
  );

  $info['og_member'] = array(
    'title' => t("OG: User membership in group"),
    'description' => t('Control access by group membership.'),
    'class' => 'OgMemberLayoutAccess',
    'default' => array('state' => OG_STATE_ACTIVE),
    'required contexts' => array(
      'node' => 'node',
      'user' => 'user',
    ),
  );

  $info['og_perm'] = array(
    'title' => t("OG: User permission in group"),
    'description' => t('Control access by group permission string.'),
    'class' => 'OgPermLayoutAccess',
    'required contexts' => array(
      'node' => 'node',
      'user' => 'user',
    ),
  );

  return $info;
}
