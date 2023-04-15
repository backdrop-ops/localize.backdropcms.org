(function ($) {

Backdrop.behaviors.ogUiContentTypes = {
  // Provide the vertical tab summaries for OG.
  attach: function (context) {console.log('loaded');
    var $context = $(context);
    $context.find('fieldset#edit-og').backdropSetSummary(function() {console.log('found it')
      var vals = [];
      if ($context.find('input[name="og_group_type"]:checked').length) {
        vals.push(Backdrop.t('Is a "Group" content type'));
      }

      if ($context.find('input[name="og_group_content_type"]:checked').length) {
        vals.push(Backdrop.t('Is a "Group content" content type'));
        var target_type = $context.find('select[name="target_type"]').find('option:selected').text();
        console.log(target_type);
        if (target_type) {
          vals.push(Backdrop.t('Target type: ' + target_type));
        }
        var target_bundles = $context.find('select[name="target_bundles[]"]').find('option:selected').toArray().map(item => item.text).join();
        if (target_bundles) {
          vals.push(Backdrop.t('Target bundles: ' + target_bundles));
        }
      }

      return vals.length ? vals.join(', ') : Backdrop.t('Not set');
    });
  }
};

})(jQuery);
