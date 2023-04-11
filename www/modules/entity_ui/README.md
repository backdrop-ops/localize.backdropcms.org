# Entity UI

Port to Backdrop of the UI functionality of Drupal's Entity API.
Provides an administrative interface for managing entities defined in 
`hook_entity_info()`.

## Installation

Install this module using the official Backdrop CMS instructions at
https://backdropcms.org/guide/modules

Add the appropriate "admin ui" key to `hook_entity_info()` or modify an existing 
implementation via `hook_entity_info_alter()`. Admin UI key structure:
```
- admin ui: An array of optional information used for providing an
  administrative user interface. To enable the UI at least the path must be
  given. Apart from that, the 'access callback' (see below) is required for
  the entity, as well as the 'ENTITY_TYPE_form' for editing, adding and
  cloning. The form gets the entity and the operation ('edit', 'add' or
  'clone') passed. See entity_ui_get_form() for more details.
  Known keys are:
  - path: A path where the UI should show up as expected by hook_menu().
  - controller class: (optional) A controller class name for providing the
    UI. Defaults to EntityDefaultUIController, which implements an admin UI
    suiting for managing configuration entities. Other provided controllers
    suiting for content entities are EntityContentUIController or
    EntityBundleableUIController (which work fine despite the poorly named
    'admin ui' key).
    For customizing the UI inherit from the default class and override
    methods as suiting and specify your class as controller class.
  - file: (optional) The name of the file in which the entity form resides
    as it is required by hook_menu().
  - file path: (optional) The path to the file as required by hook_menu. If
    not set, it defaults to entity type's module's path, thus the entity
    types 'module' key is required.
  - menu wildcard: The wildcard to use in paths of the hook_menu() items.
    Defaults to %entity_object which is the loader provided by Entity API.
```
 
Example:
```
    'admin ui' => array(
      'path' => 'admin/structure/profiles',
      'file' => 'profile2.admin.inc',
      'controller class' => 'Profile2TypeUIController',
    ),
```

## License

This project is GPL v2 software.
See the LICENSE.txt file in this directory for complete text.

## Current Maintainers

  - Andy Shillingford github.com/docwilmot
  - Alejandro Cremaschi github.com/argiepiano

## Credits

The Drupal Entity API project https://www.drupal.org/project/entity

