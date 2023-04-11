
Entity Tokens
=============

Provides token replacements for all properties that have no tokens and are
known to the entity API. Enable to use.

IMPORTANT: Version 2 is the recommended version. Version 1 more closely
resemebles the Drupal version, but presents several unresolved issues.

Version 2.x of Entity Tokens has been rewritten to:
- Automatically provide tokens for custom entities' properties
- Allow the use of chained field tokens that mimic the chained properties of entity metadata wrappers
  for fields that contain structures (e.g. formatted text fields) and chained multi-value fields
- Avoid providing replacements for unchained field tokens (all of which are provided in Backdrop's core)
- Avoid providing replacements for image and date fields, as those are handled very well in Backdrop's core
- Provide token information that "plays nice" with information provided by Backdrop core

IMPORTANT: we have moved metadata property definitions for date fields from Entity Token
to Entity Plus, which is their correct location. This means that all date metadata information
is now located in Entity Plus version >= 1.0.14.

**Note on Drupal compatibility:** Unlike the Drupal 7 version, this Backdrop
version uses underscores in tokens rather than dashes.

Examples of chained field tokens
--------------------------------

All unchained field tokens, and all chained date and image field tokes are handled by Backdrop's core. 
Entity Tokens handles all other chained field tokens for known field types provided in core.

- [node:field_text_multiple:0] = Provides the first value of a multi-value text field attached to a node.
- [node:field_my_formatted_text:format] = Provides the format name for a formatted text field attached to a node.
- [node:field_link_multiple:1:url] = Provides the URL of the second value of a multi-value link field.
- [node:field_text_with_summary:summary] = Provides the summary of a long text with summary field.

Requirements
------------
This module requires that the following modules are also enabled:

* [Entity Plus](https://github.com/backdrop-contrib/entity_plus)

LICENSE
-------

This project is GPL v2 software. See the LICENSE.txt file in this directory 
for complete text.

CURRENT MAINTAINERS
-------------------

- [argiepiano](https://github.com/argiepiano)
- Seeking co-maintainers

CREDITS
-------

Version 2.x was rewritten by argiepiano (https://github.com/argiepiano/)

Version 1.x was ported to Backdrop from Drupal 7 Entity API module 
by docwilmot (https://github.com/docwilmot/) 

Original Drupal Entity API by Wolfgang Ziegler, nuppla@zites.net
