# Organic Groups (OG)

The Organic Groups module (also referred to as the `og` module), provides users
the ability to create, manage, and delete their own 'groups' on a site.
Each group can have members, and maintains a group home page which individual
group members may post into. Posts can be sent to multiple groups (i.e. cross-
posted), and individual posts (referred as 'group content') may be shared with
members, or non-members where necessary. Group membership can be open, closed
or moderated.

## Dependencies

- Entity Plus
- Entity UI
- Entity Reference
- _Core: Entity_
- _Core Field: Text_
- _Core Field: List_
- _Core Field: Options_

## Installation

 - Install this module and its dependencies using the official 
  [Backdrop CMS instructions](https://backdropcms.org/guide/modules)

## Configuration and Usage

Please visit the [Organic Groups Wiki](https://github.com/backdrop-contrib/og/wiki) for information.

## Example OG module

An example module called "Organic groups example" is included in this project inside the folder og_demo. For information on that module, read the README file in that folder.

## Upgrading Organic Groups from Drupal 7

If you are upgrading an existing Organic Groups installation from Drupal 7 (or upgrading a Drupal 7 module that requires Organic Groups), there are some steps you can (should) take to ensure that your upgraded installation is fully enabled at the end of the process.

#### Stub Modules

NOTE: The stub modules listed below are **still needed** in the Drupal 7 version when upgrading OG to Backdrop, even in Backdrop 1.22.0 and higher.

Backdrop Organic Groups relies on two modules that don't exist in Drupal:

- Entity UI (`entity_ui`), needed for the Backdrop version of Organic Groups;
- Entity Plus (`entity_plus`), also needed for the Backdrop version of Organic Groups.

When the Backdrop version of `update.php` runs, if those modules are not present and enabled, then the update will disable all of the Backdrop modules that depend on them, which includes Organic Groups, plus any further modules that depend on Organic Groups.

So the solution is to install "stub" modules in the Drupal installation prior to generating the database from which the Backdrop upgrade will take place.

#### Upgrade Process

Assuming you are following [the official upgrade instructions](https://backdropcms.org/upgrade-from-drupal) or something similar [like this](https://packweb.eu/blog/migrating-drupal-7-backdrop-cms), do the following:

1. Download and unzip these two modules, which are Drupal 7 "stub" module needed during the upgrade process:
    - [entity_plus.zip](https://github.com/backdrop-contrib/rules/wiki/files/entity_plus.zip)
    - [entity_ui.zip](https://github.com/backdrop-contrib/rules/wiki/files/entity_ui.zip)

2. In Step 2, "Prepare your Drupal site for upgrade", prior to substep 9 ("Make a second backup...calling it backdrop-ready.sql"):

    - Install those two stub modules into `sites/all/modules` of your Drupal installation;
    - Enable those two modules. (They don't do anything. This just makes your database know that they exist, or rather, that they will be forthcoming.)

3. Continue with substep 9: make your second backup of the database to be used for Backdrop, e.g., backdrop-ready.sql.

4. Once you have made that second backup, you can uninstall and remove the two stub modules from your Drupal installation.

5. Continue with the rest of the migration/upgrade process.

Of course, make sure that you've already installed the Backdrop versions of Organic Groups, Entity Plus, and Entity UI in your Backdrop contrib folder prior to running `update.php`, or all of the above will be for naught.

## Issues

- Bugs and Feature requests should be reported in the 
  [Issue Queue](https://github.com/backdrop-contrib/og/issues)

## Current Maintainers

 - [Laryn Kragt Bakker](https://github.com/laryn) - [CEDC.org](https://cedc.org)
 - [Alejandro Cremaschi](https://github.com/argiepiano)
 - Co-maintainers and collaborators are welcome.

## Credits

- Ported to Backdrop CMS by [Laryn Kragt Bakker](https://github.com/laryn) - [CEDC.org](https://cedc.org).
- Organic groups for Drupal 5 and 6 authored by Moshe Weitzman
- Current Drupal project maintainer and Drupal 7 author is Amitai Burstein (Amitaibu), gizra.com

## License

This project is GPL v2 software. See the LICENSE.txt file in this directory for
complete text.
