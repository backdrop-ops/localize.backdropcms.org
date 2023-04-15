<?php

/**
 * @file
 * A class used for group membership types.
 */
class OgMembershipType extends Entity {

  public $name;
  public $description = '';

  public function id() {
    return isset($this->id) ? $this->id : NULL;
  }

  /**
   * Implements EntityInterface::entityType().
   */
  public function entityType() {
    return 'og_membership_type';
  }

  /**
   * Implements EntityInterface::label().
   */
  public function label() {
    return isset($this->description) ? $this->description : NULL;
  }

  /**
   * Implements EntityInterface::uri().
   */
  public function uri() {
    return array(
      'path' => 'admin/config/group/group-membership/manage/' . $this->name,
      'options' => array(),
    );
  }

  public function __construct($values = array()) {
    parent::__construct($values, 'og_membership_type');
  }
}
