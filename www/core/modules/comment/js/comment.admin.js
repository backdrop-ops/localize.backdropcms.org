/**
 * @file
 * Attaches comment behaviors to the node form.
 */

(function ($) {

Backdrop.behaviors.commentFieldsetSummaries = {
  attach: function (context) {
    var $context = $(context);
    $context.find('fieldset.comment-node-settings-form').backdropSetSummary(function () {
      var vals = [];
      var status = $context.find('.form-item-comment input:checked').next('label').text().replace(/^\s+|\s+$/g, '');
      vals.push(Backdrop.checkPlain(status));
      if (status.trim() != 'Open') {
        if ($context.find(".form-item-comment-hidden input:checked").length) {
          vals.push(Backdrop.t('Hidden'));
        }
      }
      return Backdrop.checkPlain(vals.join(', '));
    });

    // Provide the summary for the node type form.
    $context.find('fieldset.comment-node-type-settings-form').backdropSetSummary(function() {
      var vals = [];

      // Default comment setting.
      vals.push($context.find(".form-item-comment-default input:checked").parent().find('label').text().replace(/^\s+|\s+$/g, ''));

      // Comments per page.
      var number = parseInt($context.find(".form-item-comment-per-page input").val());
      vals.push(Backdrop.t('@number comments per page', {'@number': number}));

      // Threading.
      if ($context.find(".form-item-comment-mode input:checked").length) {
        vals.push(Backdrop.t('Threaded'));
      }
      else {
        vals.push(Backdrop.t('Flat list'));
      }

      // Automatic comment closer setting.
      if ($context.find(".form-item-comment-close-enabled input:checked").length) {
        var number = parseInt($context.find(".form-item-comment-close-days input").val());
        vals.push(Backdrop.t('Automatically close comments after @number days', {'@number': number}));
      }

      return Backdrop.checkPlain(vals.join(', '));
    });
  }
};

})(jQuery);
