# Mail System

Provides an Administrative UI and Developers API for managing the mail backend/plugin.

Allows to use different backends for formatting and sending e-mails by default, per module and per mail key. Additionally, a theme can be configured that is used for sent mails. The theme must be enabled for each template.

Other email modules that make use of Backdrop's mailing system include:

+ simplenews
+ simplenews_scheduler
+ mimemail
+ mandrill
+ smtp
+ views send

## Installation

- Install this module using the official Backdrop CMS instructions at
  https://backdropcms.org/guide/modules.

## Configuration

Use the configuration page at `/admin/config/system/mailsystem` to set one available mail system class as the system-wide default. You can choose different mail system classes for formatting and for sending. For instance, you may want to choose Mimemail for it's excellent ability to format emails, and choose SMTP for sending emails so you can use a third-party server for performance and stability. Additionally mail system classes can be chosen for each module that sends out email. Examples include: Contact, Webform, Views Send. Some modules further specifies emailing with optional keys. You may have to examine the source code of the hook_mail implementation of the module in question in order to find an appropriate value. For many setups this can be left blank.

## License

This project is GPL v2 software. See the LICENSE.txt
file in this directory for complete text.

## Current Maintainers

* Graham Oliver (https://github.com/Graham-72/)
* Herb v/d Dool (https://github.com/herbdool)

## Credits

### Port to Backdrop

+ Graham Oliver (github.com/Graham-72)
+ Andy Martha (github.com/biolithic)

### Maintainers for Drupal:

+ Sascha Grossenbacher (Berdir)
+ Nik Alexandrov (Nafes)
+ Les Lim (Les_Lim)
+ Robert Vincent (pillarsdotnet)

### Acknowledgement

This port to Backdrop would not, of course, be possible without all
the work done by the developers and maintainers of the Drupal module.
