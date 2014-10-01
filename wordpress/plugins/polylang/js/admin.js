jQuery(document).ready(function($) {
	// languages form
	// fills the fields based on the language dropdown list choice
	$('#lang_list').change(function() {
		value = $(this).val().split('-');
		selected = $("select option:selected").text().split(' - ');
		$('#lang_slug').val(value[0]);
		$('#lang_locale').val(value[1]);
		$('input[name="rtl"]').val([value[2]]);
		$('#lang_name').val(selected[0]);
	});

	// settings page
	// manages visibility of fields
	$("input[name='force_lang']").change(function() {
		function pll_toggle(a, test) {
			test ? a.show() : a.hide();
		}

		var value = $(this).val();
		pll_toggle($('#pll-domains-table'), 3 == value);
		pll_toggle($("#pll-hide-default"), 3 > value);
		pll_toggle($("#pll-detect-browser"), 3 > value);
		pll_toggle($("#pll-rewrite"), 2 > value);

		pll_toggle($("#pll-url-complements"), 3 > value || $("input[name='redirect_lang']").size());
	});
});
