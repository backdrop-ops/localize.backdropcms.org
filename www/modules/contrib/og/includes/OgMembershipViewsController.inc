<?php
/**
 * @file
 * Contains the OgMembershipViewsController class.
 */

/**
 * "OG membership" entity Views definitions.
 */
class OgMembershipViewsController extends EntityPlusDefaultViewsController {

  /**
   * Override views_data().
   */
  public function views_data() {
    $data = parent::views_data();

    $data['og_membership']['state']['filter']['handler'] = 'og_handler_filter_group_audience_state';
    $data['og_membership']['state']['field']['handler'] = 'og_handler_field_group_audience_state';

    $data['og_membership']['etid'] = array(
      'title' => t('Entity id'),
      'help' => t('Og membership "etid" property.'),
      'field' => array(
        'handler' => 'views_handler_field_numeric',
        'click sortable' => TRUE,
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
      'filter' => array(
        'handler' => 'views_handler_filter_numeric',
      ),
      'argument' => array(
        'handler' => 'views_handler_argument_numeric',
      ),
    );

    // Improve help text.
    $data['og_membership']['gid'] = $data['og_membership']['etid'];
    $data['og_membership']['gid']['title'] = t('Group ID');
    $data['og_membership']['gid']['help'] = t('Og membership "gid" property.');

    $data['og_membership']['og_roles'] = array(
      'title' => t('OG user roles in group'),
      'help' => t('Show all the roles a user belongs to in a group. Requires a relationship to users to be present.'),
      // This is a dummy field, so point it to a real field that we need - the
      // group ID
      'real field' => 'gid',
      'field' => array(
        'handler' => 'og_handler_field_user_roles',
      ),
    );

    $data['og_membership']['og_users_roles'] = array(
      'title' => t('OG Roles from membership'),
      'help' => t('The OG Roles associated with the OG membership'),
      // Provide a possible relationship to roles through uid.
      'relationship' => array(
        'label' => t('OG Roles from OG membership'),
        'handler' => 'og_handler_relationship_membership_roles',
        'base' => 'og_users_roles',
        'base field' => 'uid',
        'relationship field' => 'etid',
      ),
    );

    $data['og_role'] = array(
      'table' => array(
        'group' => t('OG user roles'),
        'join' => array(
          // Attach automatically to og_users_roles relations.
          'og_users_roles' => array(
            'left_field' => 'rid',
            'field' => 'rid',
          ),
        ),
      ),
    );
    $data['og_role']['name'] = array(
      'title' => t('Role Name'),
      'help' => t('The OG role name.'),
      'filter' => array(
        'handler' => 'og_handler_filter_user_roles',
      ),
    );

    // Link to edit membership.
    $data['og_membership']['edit_membership'] = array(
      'field' => array(
        'title' => t('Edit link'),
        'help' => t('Provide a simple link to edit the membership.'),
        'handler' => 'og_handler_field_og_membership_link_edit',
      ),
    );

    // Link to delete membership.
    $data['og_membership']['delete_membership'] = array(
      'field' => array(
        'title' => t('Delete link'),
        'help' => t('Provide a simple link to delete the membership.'),
        'handler' => 'og_handler_field_og_membership_link_delete',
      ),
    );

  $data['og_membership']['og_membership_bulk_form'] = array(
    'title' => t('Bulk operations'),
    'help' => t('Provides a checkbox on each row and a form for executing bulk actions on membership.'),
    'real field' => 'id',
    'field' => array(
      'handler' => 'og_handler_field_bulk_form',
      'click sortable' => FALSE,
      'real field' => 'id',
    ),
  );

    return $data;
  }
}
