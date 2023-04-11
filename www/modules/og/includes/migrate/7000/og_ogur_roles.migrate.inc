<?php

/**
 * @file
 * Add OG related fields to group and group-content node-types.
 */

class OgMigrateOgurRoles extends MigrationBase {

  /**
   * Override constructor from Migration class.
   */
  public function __construct($arguments) {
    parent::__construct($arguments);
    $this->description = t('Create OG Roles for OGUR roles from system role table.');
    $this->enabled = !$this->isComplete();
    $this->dependencies[] = 'OgMigrateAddFields';
  }

  /**
   * Check variable to see if we have already run this job.
   *
   * @return bool
   *   TRUE if we have already completed.
   */
  public function isComplete() {
    return !config_get('og.settings', 'og_7200_ogur_roles');
  }

  /**
   * Create group, group-audience and group description fields.
   */
  public function import() {
    // We are only interested in roles actually managed by OGUR, not default.
    $og_roles = array();
    foreach (node_type_get_types() as $type) {
      $og_roles[$type->type] = config_get('og.settings', 'og_user_roles_roles_' . $type->type);
    }
    $roles = array();
    $backdrop_roles = db_select('d6_og_users_roles', 'ogur')
                      ->fields('ogur', array('rid'))
                      ->groupBy('rid')
                      ->orderBy('rid', 'ASC')
                      ->execute()
                      ->fetchCol();
    // For each role, enumerate over all possible types and respective bundles.
    foreach ($backdrop_roles as $backdrop_role) {
      foreach (og_get_all_group_bundle() as $type => $bundle_array) {
        foreach ($bundle_array as $bundle => $name) {
          if ($og_roles[$bundle][$backdrop_role]) {
            $role = user_role_load($backdrop_role);
            $og_role = og_role_create($role->name, $type, 0, $bundle);
            og_role_save($og_role);
            $roles[$role->rid] = $og_role->rid;
          }
        }
      }
    }

    // Delete the field that indicates we still need to add fields.
    config_clear('og.settings', 'og_7200_ogur_roles');

    return MigrationBase::RESULT_COMPLETED;
  }

}
