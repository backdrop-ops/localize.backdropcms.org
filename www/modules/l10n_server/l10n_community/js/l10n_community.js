(function ($) {

  Backdrop.behaviors.replaceL10nServerURLs = {
    attach: function (context) {
      // Act on links if we have URL replacements to do.
      if (Backdrop.settings.l10nServerURLs) {
        for (var url in Backdrop.settings.l10nServerURLs) {
          // Look for links with this exact URL and replace with extended version.
          // This ensures we keep filter values while switching tabs.
          $('a[href="' + url + '"]', context).not('.filter-exclude').attr('href', Backdrop.settings.l10nServerURLs[url]);
        }
      }
    }
  }

})(jQuery);
