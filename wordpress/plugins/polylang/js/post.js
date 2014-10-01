
// overrides tagBox but mainly copy paste of WP code
(function($){
	// overrides function to add the language
	tagBox.get = function(id, a) {
		var tax = id.substr(id.indexOf('-')+1);

		// add the language in the $_POST variable
		var data = {
			action: 'get-tagcloud',
			lang: $('#post_lang_choice').val(),
			tax: tax
		}

		$.post(ajaxurl, data, function(r, stat) {
			if ( 0 == r || 'success' != stat )
				r = wpAjax.broken;

			r = $('<p id="tagcloud-'+tax+'" class="the-tagcloud">'+r+'</p>');
			$('a', r).click(function(){
				tagBox.flushTags( $(this).closest('.inside').children('.tagsdiv'), this);
				return false;
			});

			// add an if else condition to allow modifying the tags outputed when switching the language
			if (a == 1)
				$('#'+id).after(r);
			else {
				v = $('.the-tagcloud').css('display');
				$('.the-tagcloud').replaceWith(r);
				$('.the-tagcloud').css('display', v);
			}
		});
	},

	// creates the function to be reused
	tagBox.suggest = function() {
		ajaxtag = $('div.ajaxtag');
		// add the unbind function to allow calling the function when the language is modified
		$('input.newtag', ajaxtag).unbind().blur(function() {
			if ( this.value == '' )
	            $(this).parent().siblings('.taghint').css('visibility', '');
	    }).focus(function(){
			$(this).parent().siblings('.taghint').css('visibility', 'hidden');
		}).keyup(function(e){
			if ( 13 == e.which ) {
				tagBox.flushTags( $(this).closest('.tagsdiv') );
				return false;
			}
		}).keypress(function(e){
			if ( 13 == e.which ) {
				e.preventDefault();
				return false;
			}
		}).each(function(){
			// add the language in the $_GET variable
			var lang = $('#post_lang_choice').val();
			var tax = $(this).closest('div.tagsdiv').attr('id');
			// change action name for backward compatibility WP < 3.7
			$(this).suggest( ajaxurl + '?action=polylang-ajax-tag-search&lang=' + lang + '&tax=' + tax, { delay: 500, minchars: 2, multiple: true, multipleSep: "," } );
		});
	}

	// overrides function to add language (in tagBox.suggest)
	tagBox.init = function() {
		var t = this, ajaxtag = $('div.ajaxtag');

	    $('.tagsdiv').each( function() {
	        tagBox.quickClicks(this);
	    });

		$('input.tagadd', ajaxtag).click(function(){
			t.flushTags( $(this).closest('.tagsdiv') );
		});

		$('div.taghint', ajaxtag).click(function(){
			$(this).css('visibility', 'hidden').parent().siblings('.newtag').focus();
		});

		tagBox.suggest();

	    // save tags on post save/publish
	    $('#post').submit(function(){
			$('div.tagsdiv').each( function() {
	        	tagBox.flushTags(this, false, 1);
			});
		});

		// tag cloud
		$('a.tagcloud-link').click(function(){
			tagBox.get( $(this).attr('id'), 1 );
			$(this).unbind().click(function(){
				$(this).siblings('.the-tagcloud').toggle();
				return false;
			});
			return false;
		});
	}
})(jQuery);

// quick edit
// thanks to WP Dreamer http://wpdreamer.com/2012/03/manage-wordpress-posts-using-bulk-edit-and-quick-edit/
(function($) {
	var $wp_inline_edit = inlineEditPost.edit;

	inlineEditPost.edit = function( id ) {
		$wp_inline_edit.apply( this, arguments );
		var $post_id = 0;
		if ( typeof( id ) == 'object' )
			$post_id = parseInt( this.getId( id ) );

		if ( $post_id > 0 ) {
			var $edit_row = $( '#edit-' + $post_id );
			var $select = $edit_row.find(':input[name="inline_lang_choice"]');
			$select.find('option:selected').removeProp('selected');
			var lang = $('#lang_' + $post_id).html();
			$("input[name='old_lang']").val(lang);
			$select.find('option[value="'+lang+'"]').prop('selected', true);
		}
	}
})(jQuery);

jQuery(document).ready(function($) {
	// collect taxonomies - code partly copied from WordPress
	var taxonomies = new Array();
	$('.categorydiv').each( function(){
		var this_id = $(this).attr('id'), taxonomyParts, taxonomy;

		taxonomyParts = this_id.split('-');
		taxonomyParts.shift();
		taxonomy = taxonomyParts.join('-');
		taxonomies.push(taxonomy); // store the taxonomy for future use

		// add our hidden field in the new category form - for each hierarchical taxonomy
		// to set the language when creating a new category
		$('#' + taxonomy + '-add-submit').before($('<input />')
			.attr('type', 'hidden')
			.attr('id', taxonomy + '-lang')
			.attr('name', 'term_lang_choice')
			.attr('value', $('#post_lang_choice').val())
		);
	});

	// ajax for changing the post's language in the languages metabox
	$('#post_lang_choice').change( function() {
		var data = {
			action: 'post_lang_choice',
			lang: $(this).val(),
			taxonomies: taxonomies,
			post_id: $('#post_ID').val(),
			_pll_nonce: $('#_pll_nonce').val()
		}

		$.post(ajaxurl, data , function(response) {
			var res = wpAjax.parseAjaxResponse(response, 'ajax-response');
			$.each(res.responses, function() {
				switch (this.what) {
					case 'translations': // translations fields
						$('#post-translations').html(this.data);
						init_translations();
						break;
					case 'taxonomy': // categories metabox for posts
						var tax = this.data;
						$('#' + tax + 'checklist').html(this.supplemental.all);
						$('#' + tax + 'checklist-pop').html(this.supplemental.populars);
						$('#new' + tax + '_parent').replaceWith(this.supplemental.dropdown);
						$('#' + tax + '-lang').val($('#post_lang_choice').val()); // hidden field
						break;
					case 'pages': // parent dropdown list for pages
						$('#pageparentdiv > .inside').html(this.data);
						break;
					default:
						break;
				}
			});

			// modifies the language in the tag cloud
			$('.tagcloud-link').each(function() {
				var id = $(this).attr('id');
				tagBox.get(id, 0);
			});

			// modifies the language in the tags suggestion input
			tagBox.suggest();
		});
	});

	// ajax for changing the media's language
	$('.media_lang_choice').change( function() {
		var data = {
			action: 'media_lang_choice',
			lang: $(this).val(),
			post_id: $(this).attr('name'),
			_pll_nonce: $('#_pll_nonce').val()
		}

		$.post(ajaxurl, data , function(response) {
			var res = wpAjax.parseAjaxResponse(response, 'ajax-response');
			$.each(res.responses, function() {
				switch (this.what) {
					case 'translations': // translations fields
						$('.translations').html(this.data);
						$('.compat-field-translations').html(this.data); // WP 3.5+
						break;
					default:
						break;
				}
			});
		});
	});

	// translations autocomplete input box
	function init_translations() {
		$('.tr_lang').each(function(){
			var tr_lang = $(this).attr('id').substring(8);
			var td = $(this).parent().siblings('.pll-edit-column');

			$(this).autocomplete({
				minLength: 0,

				source: ajaxurl + '?action=pll_posts_not_translated&post_language=' + $('#post_lang_choice').val() +
					'&translation_language=' + tr_lang + '&post_type=' + $('#post_type').val() +
					'&_pll_nonce=' + $('#_pll_nonce').val(),

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

});
