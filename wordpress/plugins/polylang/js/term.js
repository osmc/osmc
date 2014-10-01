// quick edit
if ('undefined' != typeof(inlineEditTax)) {
	(function($) {
		var $wp_inline_edit = inlineEditTax.edit;

		inlineEditTax.edit = function( id ) {
			$wp_inline_edit.apply( this, arguments );
			var $term_id = 0;
			if ( typeof( id ) == 'object' )
				$term_id = parseInt( this.getId( id ) );

			if ( $term_id > 0 ) {
				var $edit_row = $('#edit-' + $term_id);
				var $select = $edit_row.find(':input[name="inline_lang_choice"]');
				$select.find('option:selected').removeProp('selected');
				var lang = $('#lang_' + $term_id).html();
				$("input[name='old_lang']").val(lang);
				$select.find('option[value="'+lang+'"]').prop('selected', true);
			}
		}
	})(jQuery);
}

jQuery(document).ready(function($) {
	// translations autocomplete input box
	function init_translations() {
		$('.tr_lang').each(function(){
			var tr_lang = $(this).attr('id').substring(8);
			var td = $(this).parent().siblings('.pll-edit-column');

			$(this).autocomplete({
				minLength: 0,

				source: ajaxurl + '?action=pll_terms_not_translated&term_language=' + $('#term_lang_choice').val() +
					'&term_id=' + $("input[name='tag_ID']").val() + '&taxonomy=' + $("input[name='taxonomy']").val() +
					'&translation_language=' + tr_lang + '&post_type=' + typenow + '&_pll_nonce=' + $('#_pll_nonce').val(),

				select: function(event, ui) {
					$('#htr_lang_'+tr_lang).val(ui.item.id);
					td.html(ui.item.link);
				},
			});

			// when the input box is emptied
			$(this).blur(function() {
				if (!$(this).val()) {
					$('#htr_lang_'+tr_lang).val(0);
					td.html(td.siblings('.hidden').children().clone());
				}
			});
		});
	}

	init_translations();

	// ajax for changing the term's language
	$('#term_lang_choice').change(function() {
		var data = {
			action: 'term_lang_choice',
			lang: $(this).val(),
			term_id: $("input[name='tag_ID']").val(),
			taxonomy: $("input[name='taxonomy']").val(),
			post_type: typenow,
			_pll_nonce: $('#_pll_nonce').val()
		}

		$.post(ajaxurl, data, function(response) {
			var res = wpAjax.parseAjaxResponse(response, 'ajax-response');
			$.each(res.responses, function() {
				switch (this.what) {
					case 'translations': // translations fields
						$("#term-translations").html(this.data);
						init_translations();
						break;
					case 'parent': // parent dropdown list for hierarchical taxonomies
						$('#parent').replaceWith(this.data);
						break;
					case 'tag_cloud': // popular items
						$('.tagcloud').replaceWith(this.data);
						break;
					default:
						break;
				}
			});
		});
	});
});
