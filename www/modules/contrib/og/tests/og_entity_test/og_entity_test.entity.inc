<?php

/**
 * @file
 * Entity classes for the OG Entity Test Module.
 */


/**
 * Main class for test entities.
 */
class EntityClass extends Entity {

  public function __construct(array $values = array(), $entityType = NULL) {
    parent::__construct($values, 'og_entity_test');
  }

  /**
   * Override buildContent() to add the username to the output.
   */
  public function buildContent($view_mode = 'full', $langcode = NULL) {
    $content['user'] = array(
      '#markup' => "User: " . format_username(user_load($this->uid)),
    );
    return entity_get_controller($this->entityType)->buildContent($this, $view_mode, $langcode, $content);
  }

  /**
   * Specifies the default label, which is picked up by label() by default.
   */
  protected function defaultLabel() {
    $type = og_entity_test_get_types($this->name);
    return $type->label;
  }

  /**
   * Specifies the default uri, which is picked up by uri() by default.
   */
  protected function defaultURI() {
    return array('path' => 'custom/' . $this->identifier());
  }

  /**
   * Implements EntityInterface::id().
   */
  public function id() {
    return $this->pid;
  }

  /**
   * Implements EntityInterface::entityType().
   */
  public function entityType() {
    return 'og_entity_test';
  }

  /**
   * Implements EntityInterface::label().
   */
  public function label() {
    return t('Test Entity');
  }

  /**
   * Implements EntityInterface::uri().
   */
  public function uri() {
    return array('path' => 'custom/' . $this->id());
  }

}

/**
 * Main class for test entities.
 */
class EntityClassType extends Entity {

  /**
   * Implements EntityInterface::id().
   */
  public function id() {
    return $this->id;
  }

  /**
   * Implements EntityInterface::entityType().
   */
  public function entityType() {
    return 'og_entity_test_type';
  }

  /**
   * Implements EntityInterface::label().
   */
  public function label() {
    return t('Test entity type');
  }

  /**
   * Implements EntityInterface::uri().
   */
  public function uri() {
    return array('path' => 'custom/' . $this->id());
  }

}

/**
 * Main class for test entities (with revision support).
 */
class EntityClassRevision extends EntityClass {

  public function __construct(array $values = array(), $entityType = NULL) {
    Entity::__construct($values, 'og_entity_test2');
  }

}
