Potx
================================================================================

The goal of the Translation Template Extractor project is to provide
command line and web based Gettext translation template extractor
functionality for Backdrop. These translation templates are used by
teams to translate Backdrop to their language of choice. There are
basically two ways to use the contents of this project:

 * Use Drush commands provided by the module to extract the translation strings.
     Command - drush potx
     Alias - NA
     Argument - mode : potx output mode e.g. single, multiple or core
     Options - potx accepts 4 options
       --modules : Comma delimited list of modules to extract translatable strings from.
       --files   : Comma delimited list of files to extract translatable strings from.
       --folder  : Folder to begin translation extraction in. When no other option is set this defaults to current directory.
       --api     : Backdrop core version to use for extraction settings.

     Example -
     1.Extract translatable strings from applicable files in current directory and write to single output file.
       drush potx single
       drush potx (By default, mode = single)

     2.Extract translatable strings from applicable files of example module and write to module-specific output file.
       drush potx multiple --modules=example

     3.Extract translatable strings from example.module and write to single output file.
       drush potx --files=sites/all/modules/example/example.module

     4.Extract strings from folder 'projects/backdrop/1' using API version 1.
       drush potx single --api=1 --folder=projects/backdrop/1

 * Install the module on a Backdrop site as you would with any other
   module. Once potx module is turned on, you can go to the
   "Extract" tab on the "Translate interface" administration interface, select
   the module or modules or theme or themes you want to have a translation
   template for, and submit the form. You will get one single template file
   generated.

   Note: If you only get a white browser screen as response to the
   extraction request, the memory limit for PHP on the server is probably
   too low, try to set that higher.

The module also includes optional Coder Review (https://backdropcms.org/project/coder_review)
integration, allowing you to spot translatability errors in modules while
doing your regular code review.

Credits
================================================================================

Ported to Backdrop
---

* Geoff St. Pierre [@serundeputy](https://github.com/serundeputy)

Maintainers
---

* Geoff St. Pierre [@serundeputy](https://github.com/serundeputy)
* Seeking additional maintainers

Originally Authored for Drupal
---

Command line extractor functionality orignally by
  Jacobo Tarrio <jtarrio [at] alfa21.com> (2003, 2004 Alfa21 Outsourcing)

Greatly optimized by
  Brandon Bergren (2007)

Drupal Currently maintained by
  Gabor Hojtsy <gabor [at] hojtsy.hu>
