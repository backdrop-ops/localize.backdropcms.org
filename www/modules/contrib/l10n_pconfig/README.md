
Plural formula configurator
====

About
-------

The plural formula configurator sets sensible defaults for plural forms when
adding languages and lets you edit the plural formula for all languages on the
web interface.

Drupal does not expose these fields for editing due to the complexity of plural
forms. You should make sure to only give permissions to edit language details
to those, who will likely not screw up your plural formulas.

Installation
------------


- Install this module using the official Backdrop CMS instructions at
  https://backdropcms.org/guide/modules

- The "administer languages" permission applies to functionality of this
   module. You probably already have this granted to all parties involved.
   
- Configure plural formulas on Configuration > Regional and language > Languages.
   Plural formulas are set when known for predefined languages. You can set
   custom plural formulas when adding a custom language or editing an existing
   one. 

Credits
--------

- [Gábor Hojtsy](http://drupal.org/user/4166): Drupal 7 maintainer
- [SebCorbin](https://www.drupal.org/u/sebcorbin): Drupal 7 maintainer
- [fmitchell](https://www.drupal.org/u/fmitchell): Drupal 7 maintainer
- [argiepiano](https://github.com/argiepiano): Porting and Backdrop version maintainer 

License
-------

This project is GPL v2 software. See the LICENSE.txt file in this directory for
complete text.
