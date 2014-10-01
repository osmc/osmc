<?php

/*
 * manages custom menus translations as well as the language switcher menu item on frontend
 *
 * @since 1.2
 */
class PLL_Frontend_Nav_Menu {

	/*
	 * constructor
	 *
	 * @since 1.2
	 */
	public function __construct(&$polylang) {
		$this->options = &$polylang->options;
		$this->curlang = &$polylang->curlang;

		// split the language switcher menu item in several language menu items
		add_filter('wp_get_nav_menu_items', array(&$this, 'wp_get_nav_menu_items'));
		add_filter('wp_nav_menu_objects', array(&$this, 'wp_nav_menu_objects'));
		add_filter('nav_menu_link_attributes', array(&$this, 'nav_menu_link_attributes'), 10, 3);

		// filters menus by language
		add_filter('theme_mod_nav_menu_locations', array($this, 'nav_menu_locations'), 20);
	}

	/*
	 * splits the one item of backend in several items on frontend
	 * take care to menu_order as it is used later in wp_nav_menu
	 *
	 * @since 1.1.1
	 *
	 * @param array $items menu items
	 * @return array modified items
	 */
	public function wp_get_nav_menu_items($items) {
		$new_items = array();
		$offset = 0;

		foreach ($items as $key => $item) {
			if ($options = get_post_meta($item->ID, '_pll_menu_item', true)) {
				extract($options);
				$i = 0;

				foreach (pll_the_languages(array_merge(array('raw' => 1), $options)) as $language) {
					extract($language);
					$lang_item = clone $item;
					$lang_item->ID = $lang_item->ID . '-' . $slug; // a unique ID
					$lang_item->title = $show_flags && $show_names ? $flag.'&nbsp;'.esc_html($name) : ($show_flags ? $flag : esc_html($name));
					$lang_item->url = $url;
					$lang_item->lang = $slug; // save this for use in nav_menu_link_attributes
					$lang_item->classes = $classes;
					$lang_item->menu_order += $offset + $i++;
					$new_items[] = $lang_item;
				}
				$offset += $i - 1;
			}
			else {
				$item->menu_order += $offset;
				$new_items[] = $item;
			}
		}

		return $new_items;
	}

	/*
	 * returns the ancestors of a menu item
	 *
	 * @since 1.1.1
	 *
	 * @param object $item
	 * @return array ancestors ids
	 */
	public function get_ancestors($item) {
		$ids = array();
		$_anc_id = (int) $item->db_id;
		while(($_anc_id = get_post_meta($_anc_id, '_menu_item_menu_item_parent', true)) && !in_array($_anc_id, $ids))
			$ids[] = $_anc_id;
		return $ids;
	}

	/*
	 * removes current-menu and current-menu-ancestor classes to lang switcher when not on the home page
	 *
	 * @since 1.1.1
	 *
	 * @param array $items
	 * @return array modified menu items
	 */
	public function wp_nav_menu_objects($items) {
		$r_ids = $k_ids = array();

		foreach ($items as $item) {
			if (!empty($item->classes) && is_array($item->classes)) {
				if (in_array('current-lang', $item->classes)) {
					$item->classes = array_diff($item->classes, array('current-menu-item'));
					$r_ids = array_merge($r_ids, $this->get_ancestors($item)); // remove the classes for these ancestors
				}
				elseif (in_array('current-menu-item', $item->classes)) {
					$k_ids = array_merge($k_ids, $this->get_ancestors($item)); // keep the classes for these ancestors
				}
			}
		}

		$r_ids = array_diff($r_ids, $k_ids);

		foreach ($items as $item) {
			if (!empty($item->db_id) && in_array($item->db_id, $r_ids))
				$item->classes = array_diff($item->classes, array('current-menu-ancestor', 'current-menu-parent', 'current_page_parent', 'current_page_ancestor'));
		}

		return $items;
	}

	/*
	 * adds hreflang attribute for the language switcher menu items
	 * available since WP3.6
	 *
	 * @since 1.1
	 *
	 * @param array $atts
	 * @return array modified $atts
	 */
	public function nav_menu_link_attributes($atts, $item, $args) {
		if (isset($item->lang))
			$atts['hreflang'] = $item->lang;
		return $atts;
	}

	/*
	 * fills the theme nav menus locations with the right menu in the right language
	 *
	 * @since 1.2
	 *
	 * @param array|bool list of nav menus locations, false if menu locations have not been filled yet
	 * @return array|bool modified list of nav menus locations
	 */
	public function nav_menu_locations($menus) {
		if (is_array($menus)) {
			// support for theme customizer
			// let's look for multilingual menu locations directly in $_POST as there are not in customizer object
			if (isset($_POST['wp_customize'], $_POST['customized'])) {
				$customized = json_decode(wp_unslash($_POST['customized']));

				if (is_object($customized)) {
					foreach ($customized as $key => $c) {
						if (false !== strpos($key, 'nav_menu_locations[')) {
							$loc = substr(trim($key, ']'), 19);
							if (($pos = strpos($loc, '___')) && substr($loc, $pos+3) == $this->curlang->slug) {
								$loc = substr($loc, 0, $pos);
								$menus[$loc] = $c;
							}
						}
					}
				}
			}

			// otherwise get multilingual menu locations from DB
			else {
				$theme = get_option('stylesheet');

				foreach ($menus as $loc => $menu)
					$menus[$loc] = empty($this->options['nav_menus'][$theme][$loc][$this->curlang->slug]) ? 0 : $this->options['nav_menus'][$theme][$loc][$this->curlang->slug];
			}
		}
		return $menus;
	}
}
