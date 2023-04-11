# Localization server module suite

## About

The localization server project (formerly known as lt_server) provides a 
community localization editor, which allows people from around the world to 
collaborate on translating Backdrop projects to different languages. It is
inspired by Launchpad Rosetta (https://launchpad.net/rosetta) but is highly
tailored to Backdrop needs.

This module suite powers the base functionality of http://localize.backdropcms.org.

The module suite solves the Backdrop project translation problem with a web
based interface. The various Backdrop projects release source code on a daily
basis. The goal is to help translators to keep up with this pace by sharing
already translated strings and distributing translations effectively.

The localization server module suite consists of a few possible components:

 - l10n_community: Required. A translation community interface which provides
   the database backend to store projects and releases, but does not fill these
   with actual data itself. Uses a role based permission model.
   
 - l10n_groups: Optional. An "Organic Groups" module binder, which provides
   permission handling based on language groups (in addition to the default
   role based model used by l10n_community). 
   
 - A connector module: One required, only use one at a time. Connectors serve
   the purpose of filling in the actual list of projects, releases and strings
   used in the released packages. Different connectors allow this suite to be
   used in different use cases.
   
     - l10n_localpacks: Works based on a list of files uploaded to a local
       file system directory. The projects and releases are identified based
       on placement and naming of the package files. 
     
     - l10n_backdrop_rest: To be used on Backdrop.org only! Maintains a relation
       with the Backdrop.org project and release listings, syncronizes tarballs,
       collects translatables automatically.

## Installation

 - Install this module and its dependencies using the official 
  [Backdrop CMS instructions](https://backdropcms.org/guide/modules)

- Your PHP installation should have the PEAR Tar package installed (which
  requires zzlib support). This is required for Tar extraction (in the
  l10n_localpacks module) and Tar generation (in the l10n_community module).

- Locale (built into Backdrop) is required. Organic Groups
  (https://github.com/backdrop-contrib/og) is required by l10n_groups.

1. Enable l10n_community and l10n_localpacks at Administer >
   Site configuration > Modules. Optionally enable l10n_groups.

2. Configure the connector at Administer > Localization Server.


## How it works

The connector module's duty is to maintain a list of projects and releases, as
well as fill up the database with translatable strings based on release source
codes. This module consumes a huge amount of resources. Downloading packages,
unpacking their contents and running the string extraction process takes time,
CPU cycles and hard disk space. Although only temporary copies of the packages
are kept, some hard disk space and a decent amount of memory is required. This
is why connectors are preconfigured to scan only one project at a time. Big
projects like Ubercart or Backdrop itself take considerable time to parse.

The localization community module provides the actual interface. Users with
proper permissions can suggest new translations for strings, maintainers can
even decide on the official translation based on the different suggestions. To
translate a project, go to Translations, choose a language and optionally
choose a project. There you can translate all strings.

## Upgrading from Drupal 7

Before following the directions [outlined here](https://docs.backdropcms.org/documentation/upgrading-from-drupal-7-overview), you'll need to manually the `system` table of your database:
1. Back up your Drupal 7 database
2. Using phpMyAdmin or another database UI, edit the table `system`
3. Find the row with name = `l10n_community`
4. Manually change the `schema_version` from `6024` to `0`

This is necessary because of the inability of Backdrop to upgrade modules with schema versions smaller than 7000.

Proceed with the upgrade. 

## Credits

Bruno Massa  http://drupal.org/user/67164 (original D7 author)
Gábor Hojtsy http://drupal.org/user/4166 (current D7 maintainer)
[argiepiano](https://github.com/argiepiano) (Backdrop port)

This module was originally sponsored by Titan Atlas (http://www.titanatlas.com),
a Brazilian computer company, and then by Google Summer of Code 2007. The
localization server is currently a free time project.

## Current maintainers

- [argiepiano](https://github.com/argiepiano)
- Seeking co-maintainers

## License

This project is GPL v2 software. See the LICENSE.txt file in this directory for
complete text.
