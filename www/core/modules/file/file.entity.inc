<?php
/**
 * @file
 * Entity controller and class for files.
 */

/**
 * Defines the file entity class.
 */
class File extends Entity {

  /**
   * The file ID.
   *
   * @var integer
   */
  public $fid;

  /**
   * The file type (bundle).
   *
   * @var string
   */
  public $type;

  /**
   * The file language code.
   *
   * @var string
   */
  public $langcode = LANGUAGE_NONE;

  /**
   * The uid of the user who is associated with the file.
   *
   * @var integer
   */
  public $uid;

  /**
   * Name of the file with no path components.
   *
   * This may differ from the basename of the URI if the file is renamed to
   * avoid overwriting an existing file.
   *
   * @var string
   */
  public $filename;

  /**
   * The URI to access the file (either local or remote).
   *
   * @var string
   */
  public $uri;

  /**
   * The file's MIME type.
   *
   * @var string
   */
  public $filemime;

  /**
   * The size of the file in bytes.
   *
   * @var integer
   */
  public $filesize;

  /**
   * A field indicating the status of the file.
   *
   * Two status are defined in core: temporary (0) and permanent (1).
   * Temporary files older than BACKDROP_MAXIMUM_TEMP_FILE_AGE will be removed
   * during a cron run.
   *
   * @var integer
   */
  public $status;

  /**
   * UNIX timestamp for when the file was last saved.
   *
   * @var integer
   */
  public $timestamp;

  /**
   * Constructor for file entities.
   */
  public function __construct(array $values = array()) {
    // Automatically detect filename if not set.
    if (!isset($values['filename']) && isset($values['uri'])) {
      $values['filename'] = backdrop_basename($values['uri']);
    }

    // Automatically detect filemime if not set.
    if (!isset($values['filemime']) && isset($values['uri'])) {
      $values['filemime'] = file_get_mimetype($values['uri']);
    }

    parent::__construct($values);
  }

  /**
   * Implements EntityInterface::id().
   */
  public function id() {
    return isset($this->fid) ? $this->fid : NULL;
  }

  /**
   * Implements EntityInterface::entityType().
   */
  public function entityType() {
    return 'file';
  }

  /**
   * Implements EntityInterface::bundle().
   */
  public function bundle() {
    return $this->type;
  }

  /**
   * Implements EntityInterface::label().
   */
  public function label() {
    return $this->filename;
  }

  /**
   * Implements EntityInterface::uri().
   */
  public function uri() {
    return array(
      'path' => 'file/' . $this->fid,
      'options' => array(),
    );
  }

  /**
   * Overrides Entity::createAccess().
   */
  public static function createAccess($bundle = NULL, $account = NULL) {
    $rights = &backdrop_static('file_access', array());

    // If no user object is supplied, the access check is for the current user.
    if (empty($account)) {
      $account = $GLOBALS['user'];
    }

    // If we've already checked access for this file, user and op, return from
    // cache.
    if (isset($rights[$account->uid][$bundle])) {
      return $rights[$account->uid][$bundle];
    }

    if (user_access('bypass file access', $account)) {
      $rights[$account->uid][$bundle] = TRUE;
      return $rights[$account->uid][$bundle];
    }
    if (user_access('create files', $account)) {
      $rights[$account->uid][$bundle] = TRUE;
      return $rights[$account->uid][$bundle];
    }

    // We grant access to the file if both of the following conditions are met:
    // - No modules say to deny access.
    // - At least one module says to grant access.
    $access = module_invoke_all('file_access', $bundle, 'create', $account);
    if (in_array(FILE_ACCESS_DENY, $access, TRUE)) {
      $rights[$account->uid][$bundle] = FALSE;
      return $rights[$account->uid][$bundle];
    } elseif (in_array(FILE_ACCESS_ALLOW, $access, TRUE)) {
      $rights[$account->uid][$bundle] = TRUE;
      return $rights[$account->uid][$bundle];
    }

    $rights[$account->uid][$bundle] = FALSE;
    return $rights[$account->uid][$bundle];
  }

  /**
   * Overrides Entity::access().
   *
   * @param string $op
   *   The operation to be performed on the file. Possible values are:
   *   - create
   *   - view
   *   - download
   *   - update
   *   - delete
   * @param User|AnonymousUser|object $account
   *   (optional) The user to check for. Leave it to NULL to check for the
   *   global user.
   *
   * @return bool
   *   TRUE if access is granted, FALSE otherwise.
   */
  public function access($op, $account = NULL) {
    $rights = &backdrop_static('file_access', array());

    if ($op == 'create') {
      return self::createAccess($this->bundle(), $account);
    }
    elseif (!in_array($op, array('view', 'update', 'delete', 'download'), TRUE)) {
      // If the $op was not one of the supported ones, we return access denied.
      return FALSE;
    }

    // If no user object is supplied, the access check is for the current user.
    if (empty($account)) {
      $account = $GLOBALS['user'];
    }

    $cid = $this->id();

    // If we've already checked access for this file, user and op, return from
    // cache.
    if (isset($rights[$account->uid][$cid][$op])) {
      return $rights[$account->uid][$cid][$op];
    }

    if (user_access('bypass file access', $account)) {
      return $rights[$account->uid][$cid][$op] = TRUE;
    }

    // We grant access to the file if both of the following conditions are met:
    // - No modules say to deny access.
    // - At least one module says to grant access.
    $access = module_invoke_all('file_access', $op, $this, $account);
    if (in_array(FILE_ACCESS_DENY, $access, TRUE)) {
      return $rights[$account->uid][$cid][$op] = FALSE;
    }
    elseif (in_array(FILE_ACCESS_ALLOW, $access, TRUE)) {
      return $rights[$account->uid][$cid][$op] = TRUE;
    }

    // Fall back to default behaviors on view.
    if ($op == 'view') {
      $scheme = file_uri_scheme($this->uri);
      $wrapper = file_get_stream_wrapper($scheme);

      if (!empty($wrapper['private'])) {
        // For private files, users can view private files if the
        // user has the 'view private files' permission.
        if (user_access('view private files', $account)) {
          return $rights[$account->uid][$cid][$op] = TRUE;
        }

        // For private files, users can view their own private files if the
        // user is not anonymous, and has the 'view own private files'
        // permission.
        if (!empty($account->uid) && $this->uid == $account->uid && user_access('view own private files', $account)) {
          return $rights[$account->uid][$cid][$op] = TRUE;
        }
      }
      elseif ($this->status == FILE_STATUS_PERMANENT && $this->uid == $account->uid && user_access('view own files', $account)) {
        // For non-private files, allow to see if user owns the file.
        return $rights[$account->uid][$cid][$op] = TRUE;
      }
      elseif ($this->status == FILE_STATUS_PERMANENT && user_access('view files', $account)) {
        // For non-private files, users can view if they have the 'view files'
        // permission.
        return $rights[$account->uid][$cid][$op] = TRUE;
      }
    }

    return FALSE;
  }

}

/**
 * File storage controller for files.
 */
class FileStorageController extends EntityDatabaseStorageController {
  /**
   * Overrides EntityDatabaseStorageController::presave().
   *
   * @param File $entity
   *   The file entity about to be saved.
   */
  protected function preSave(EntityInterface $entity) {
    $entity->timestamp = REQUEST_TIME;
    $entity->filesize = filesize($entity->uri);
    if (!isset($entity->langcode)) {
      // Default the file's language code to none, because files are language
      // neutral more often than language dependent.
      $entity->langcode = LANGUAGE_NONE;
    }

    // The file type is used as a bundle key, and therefore, must not be NULL.
    // It defaults to FILE_TYPE_NONE when loaded via file_load(), but in case
    // file_save() is called on a new file object, default it here too.
    if (!isset($entity->type)) {
      $entity->type = FILE_TYPE_NONE;
    }

    // If the file isn't already assigned a real type, determine what type should
    // be assigned to it.
    if ($entity->type === FILE_TYPE_NONE) {
      $type = file_get_type($entity);
      if (isset($type)) {
        $entity->type = $type;
      }
    }
  }

  /**
   * Overrides EntityDatabaseStorageController::preDelete().
   *
   * @param File[] $entities
   *   The file entities about to be deleted.
   */
  public function preDelete($entities) {
    foreach ($entities as $entity) {
      // Delete the actual file. Failures due to invalid files and files that
      // were already deleted are logged to watchdog but ignored, the
      // corresponding file entity will be deleted.
      file_unmanaged_delete($entity->uri);
    }
    // Delete corresponding file usage entries.
    db_delete('file_usage')
      ->condition('fid', array_keys($entities), 'IN')
      ->execute();
  }

  /**
   * Implements EntityControllerInterface::buildContent().
   */
  public function buildContent(EntityInterface $file, $view_mode = 'full', $langcode = NULL) {
    global $language_content;
    $langcode = $langcode ? $langcode : $language_content->langcode;

    // Remove previously built content, if exists.
    $file->content = array();

    // In case of a multiple view, file_view_multiple() already ran the
    // 'prepare_view' step. An internal flag prevents the operation from running
    // twice.
    // Allow modules to change the view mode.
    $view_mode = key(entity_view_mode_prepare('file', array($file->fid => $file), $view_mode, $langcode));
    field_attach_prepare_view('file', array($file->fid => $file), $view_mode, $langcode);
    entity_prepare_view('file', array($file->fid => $file), $langcode);

    // Build the actual file display.
    $file->content['file'] = file_view_file($file, $view_mode, $langcode);

    // Build fields content.
    $file->content += field_attach_view('file', $file, $view_mode, $langcode);

    $links = array();
    $file->content['links'] = array(
      '#theme' => 'links__file',
      '#pre_render' => array('backdrop_pre_render_links'),
      '#attributes' => array('class' => array('links', 'inline')),
    );
    $file->content['links']['file'] = array(
      '#theme' => 'links__file__file',
      '#links' => $links,
      '#attributes' => array('class' => array('links', 'inline')),
    );

    // Allow modules to make their own additions to the file.
    module_invoke_all('file_view', $file, $view_mode, $langcode);
    module_invoke_all('entity_view', $file, 'file', $view_mode, $langcode);
  }

  /**
   * Overrides DefaultEntityController::view().
   */
  public function view($files, $view_mode = 'full', $langcode = NULL, $page = NULL) {
    global $language_content;
    $langcode = $langcode ? $langcode : $language_content->langcode;

    $view = array();
    foreach ($files as $file) {
      /** @var File $file */
      // Populate $file->content with a render() array.
      $this->buildContent($file, $view_mode, $langcode);

      $build = $file->content;
      // We don't need duplicate rendering info in $file->content.
      unset($file->content);

      $build += array(
        '#theme' => 'file_entity',
        '#file' => $file,
        '#view_mode' => $view_mode,
        '#language' => $langcode,
        '#page' => $page,
      );

      // Add contextual links for this file, except when the file is already
      // being displayed on its own page. Modules may alter this behavior (for
      // example, to restrict contextual links to certain view modes) by
      // implementing hook_file_view_alter().
      if (!empty($file->fid) && !($view_mode == 'full' && file_is_page($file))) {
        $build['#contextual_links']['file'] = array('file', array($file->fid));
      }

      // Allow modules to modify the structured file.
      $type = 'file';
      backdrop_alter(array('file_view', 'entity_view'), $build, $type);
      $view[$type][$file->id()] = $build;
    }

    return $view;
  }

}
