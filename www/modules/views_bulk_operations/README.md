# Views Bulk Operations

Views Bulk Operations augments Views by allowing bulk operations
(provided by Backdrop's core, this module, Rules, or custom actions) to be 
executed on the displayed rows.
It does so by showing a checkbox in front of each displayed row, and adding a
select box on top of the View containing operations that can be applied.

This module is a port of Drupal 7's View Bulk Operation. It has been modified to
integrate seamlessly with Backdrop's existing Bulk Operation actions. This port 
also provides most configurable actions present in Drupal 7 that did not make it
into Backdrop's core Bulk Operations. 

IMPORTANT: This module is still in **alpha version**. Test thoroughly before using
it in production. Be sure to **make backup copies** of both, the database AND 
the files/config folders before installing. 

## Features

- Batching of operations with configurable batch size ensuring no timeouts.
- Adds most missing configurable actions available in D7 core and in D7 VBO. 
- Simple API for creating custom actions.
- Preliminary action configuration on the view admin page.
- Confirmation step.
- Integrates with Backdrop's core actions, and respects their workflow.

## Getting started

1. Enable Views Bulk Operations
2. Create a View.
3. Add a field. Look for a field named like so: "Views bulk operations: Content". IMPORTANT: Backdrop will also show you the core's Bulk Operation views field. Unlike VBO's fields, core fields always start with the name of the entity, as in "Content: Bulk Operations". 
3. **DO NOT** use both VBO's and core's operations in the same view! They will not get along very well.
4. Add a "Views bulk operations: XXX" field, available to all entity types.
5. Configure the field by selecting at least one operation. Note: Backdrop core's actions have fewer configuration options than the ones provided by VBO. For example, you can't enqueue Backdrop core operations like you can with VBO's.
6. Go to the View page. VBO functionality should be present.

## Creating custom actions

Creating custom actions is similar to Backdrop's core. The most important differences are:
- VBO actions must be defined in hook_vboaction_info()
- VBO action definitions **require** the key 'vbo' =>  TRUE in order to work properly. Otherwise the action will be considered a Backdrop core action, and its workflow will need to follow that of the core actions.

Documentation on how to created VBO actions will be added to the project's wiki soon. For examples on VBO actions, please check the files in the actions folder of this module. 

## Actions Permissions

A module called actions_permissions is included in the package. This module generates a permission for each action, and VBO honors those permissions before showing or executing the corresponding actions. This is useful if you want to provide your VBO to several groups of users with different privileges: the same view will accommodate those different groups, showing to each the actions that they are permitted to see.

## Issues and contributions

Issues, questions and contributions are welcome in the Github issue queue for the module.

## Current maintainers

- [argiepiano](https://github.com/argiepiano)
- Seeking co-maintainers

## Credits

- Ported to Backdrop CMS by [argiepiano](https://github.com/argiepiano).
- Creators and maintainers of the Drupal 7 version:
  - [Graber](https://www.drupal.org/u/graber)
  - [bojanz](https://www.drupal.org/u/bojanz)
  - [infojunkie](https://www.drupal.org/u/infojunkie)
  - [joelpittet](https://www.drupal.org/u/joelpittet)
  - [Jon Pugh](https://www.drupal.org/u/jon-pugh) 


## License

This project is GPL v2 software. See the LICENSE.txt file in this directory
for complete text.

