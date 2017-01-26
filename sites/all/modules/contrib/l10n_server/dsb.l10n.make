; ----------------
; Drush Make file to set up a l10n capable drupal environment.
; ----------------

api = 2
core = 7.x
projects[drupal][version] = 7.x

projects[l10n_server][type] = module
projects[l10n_server][download][type] = git
projects[l10n_server][download][url] = http://git.drupal.org/project/l10n_server.git
projects[l10n_server][download][branch] = 7.x-1.x

projects[l10n_client][type] = module
projects[l10n_client][download][type] = git
projects[l10n_client][download][url] = http://git.drupal.org/project/l10n_client.git
projects[l10n_client][download][branch] = 7.x-1.x

projects[simplenews][type] = module
projects[simplenews][download][type] = git
projects[simplenews][download][url] = http://git.drupal.org/project/simplenews.git
projects[simplenews][download][branch] = 7.x-1.x
