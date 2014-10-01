<?php

/*
 * filters search forms when using permalinks
 *
 * @since 1.2
 */
class PLL_Frontend_Filters_Search {
	public $links;

	/*
	 * constructor
	 *
	 * @since 1.2
	 *
	 * @param object $polylang
	 */
	public function __construct(&$polylang) {
		$this->links = &$polylang->links;
		$this->curlang = &$polylang->curlang;
		$this->using_permalinks = (bool) get_option('permalink_structure'); // are we using permalinks?

		// adds the language information in the search form
		// low priority in case the search form is created using the same filter as described in http://codex.wordpress.org/Function_Reference/get_search_form
		add_filter('get_search_form', array(&$this, 'get_search_form'), 99);

		// adds the language information in admin bar search form
		add_action('add_admin_bar_menus', array(&$this, 'add_admin_bar_menus'));

		// backward compatibility WP < 3.6
		// adds javascript at the end of the document
		if (!$this->using_permalinks && (!defined('PLL_SEARCH_FORM_JS') || PLL_SEARCH_FORM_JS))
			add_action('wp_footer', array(&$this, 'wp_print_footer_scripts'));
	}

	/*
	 * adds the language information in the search form
	 * does not work if searchform.php (prior to WP 3.6) is used or if the search form is hardcoded in another template file
	 *
	 * @since 0.1
	 *
	 * @param string $form search form
	 * @return string modified search form
	 */
	public function get_search_form($form) {
		if ($form) {
			if ($this->using_permalinks) {
				// take care to modify only the url in the <form> tag
				preg_match('#<form.+>#', $form, $matches);
				$old = reset($matches);
				$new = preg_replace('#' . $this->links->home . '\/?#', $this->links->get_home_url('', true), $old);
				$form = str_replace($old, $new, $form);
			}
			else
				$form = str_replace('</form>', '<input type="hidden" name="lang" value="'.esc_attr($this->curlang->slug).'" /></form>', $form);
		}

		return $form;
	}

	/*
	 * adds the language information in admin bar search form
	 *
	 * @since 1.2
	 */
	function add_admin_bar_menus() {
		remove_action('admin_bar_menu', 'wp_admin_bar_search_menu', 4);
		add_action('admin_bar_menu', array(&$this, 'admin_bar_search_menu'), 4);
	}

	/*
	 * rewrites the admin bar search form to pass our get_search form filter. See #21342
	 *
	 * @since 0.9
	 *
	 * @param object $wp_admin_bar
	 */
	public function admin_bar_search_menu($wp_admin_bar) {
		$form  = '<form action="' . esc_url( home_url( '/' ) ) . '" method="get" id="adminbarsearch">';
		$form .= '<input class="adminbar-input" name="s" id="adminbar-search" type="text" value="" maxlength="150" />';
		$form .= '<input type="submit" class="adminbar-button" value="' . __('Search') . '"/>';
		$form .= '</form>';

		$wp_admin_bar->add_menu(array(
			'parent' => 'top-secondary',
			'id'     => 'search',
			'title'  => $this->get_search_form($form), // pass the get_search_form filter
			'meta'   => array('class' => 'admin-bar-search', 'tabindex' => -1)
		));
	}

	/*
	 * modifies the search form since filtering get_search_form won't work if the template uses searchform.php or the search form is hardcoded
	 * now only for backward compatibility WP < 3.6
	 *
	 * @since 0.1
	 */
	public function wp_print_footer_scripts() {
		// don't use directly e[0] just in case there is somewhere else an element named 's'
		// check before if the hidden input has not already been introduced by get_search_form (FIXME: is there a way to improve this) ?
		// thanks to AndyDeGroo for improving the code for compatility with old browsers
		// http://wordpress.org/support/topic/development-of-polylang-version-08?replies=6#post-2645559
		$lang = esc_js($this->curlang->slug);
		$js = "//<![CDATA[
		e = document.getElementsByName('s');
		for (i = 0; i < e.length; i++) {
			if (e[i].tagName.toUpperCase() == 'INPUT') {
				s = e[i].parentNode.parentNode.children;
				l = 0;
				for (j = 0; j < s.length; j++) {
					if (s[j].name == 'lang') {
						l = 1;
					}
				}
				if ( l == 0) {
					var ih = document.createElement('input');
					ih.type = 'hidden';
					ih.name = 'lang';
					ih.value = '$lang';
					e[i].parentNode.appendChild(ih);
				}
			}
		}
		//]]>";
		echo "<script type='text/javascript'>" .$js. "</script>";
	}
}
