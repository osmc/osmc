<?php

/*
 * manages custom menus translations as well as the language switcher menu item on admin side
 *
 * @since 1.2
 */
class PLL_Admin_Nav_Menu {

	/*
	 * constructor: setups filters and actions
	 *
	 * @since 1.2
	 *
	 * @param object $polylang
	 */
	public function __construct(&$polylang) {
		$this->model = &$polylang->model;
		$this->options = &$polylang->options;
		$this->theme = get_option( 'stylesheet' );

		// integration in the WP menu interface
		add_action('admin_init', array(&$this, 'admin_init')); // after Polylang upgrade

		// integration with WP customizer
		add_action('customize_register', array(&$this, 'create_nav_menu_locations'), 5);

		// protection against #24802
		add_filter('pre_insert_term', array(&$this, 'pre_insert_term'), 10, 2);
	}

	/*
	 * setups filters and terms
	 * adds the language switcher metabox and create new nav menu locations
	 *
	 * @since 1.1
	 */
	public function admin_init(){
		add_action('admin_enqueue_scripts', array(&$this, 'admin_enqueue_scripts'));
		add_action('wp_update_nav_menu_item', array(&$this, 'wp_update_nav_menu_item'), 10, 2);
		add_filter('wp_get_nav_menu_items', array(&$this, 'translate_switcher_title'));

		// translation of menus based on chosen locations
		add_filter('pre_update_option_theme_mods_' . $this->theme, array($this, 'update_nav_menu_locations'));
		add_filter('theme_mod_nav_menu_locations', array($this, 'nav_menu_locations'), 20);

		// filter _wp_auto_add_pages_to_menu by language
		add_action('transition_post_status', array(&$this, 'auto_add_pages_to_menu'), 5, 3); // before _wp_auto_add_pages_to_menu

		// FIXME is it possible to choose the order (after theme locations in WP3.5 and older) ?
		// FIXME not displayed if Polylang is activated before the first time the user goes to nav menus http://core.trac.wordpress.org/ticket/16828
		add_meta_box('pll_lang_switch_box', __('Language switcher', 'polylang'), array( &$this, 'lang_switch' ), 'nav-menus', 'side', 'high');

		$this->create_nav_menu_locations();
	}

	/*
	 * create temporary nav menu locations (one per location and per language) for all non-default language
	 *
	 * @since 1.2
	 */
	public function create_nav_menu_locations() {
		global $_wp_registered_nav_menus;

		if (isset($_wp_registered_nav_menus)) {
			foreach ($_wp_registered_nav_menus as $loc => $name)
				foreach ($this->model->get_languages_list() as $lang)
					$arr[$loc . (pll_default_language() == $lang->slug ? '' : '___' . $lang->slug)] = $name . ' ' . $lang->name;

			$_wp_registered_nav_menus = $arr;
		}
	}

	/*
	 * language switcher metabox
	 * The checkbox and all hidden fields are important
	 * thanks to John Morris for his very interesting post http://www.johnmorrisonline.com/how-to-add-a-fully-functional-custom-meta-box-to-wordpress-navigation-menus/
	 *
	 * @since 1.1
	 */
	public function lang_switch() {
		global $_nav_menu_placeholder, $nav_menu_selected_id;
		$_nav_menu_placeholder = 0 > $_nav_menu_placeholder ? $_nav_menu_placeholder - 1 : -1;?>

		<div id="posttype-lang-switch" class="posttypediv">
			<div id="tabs-panel-lang-switch" class="tabs-panel tabs-panel-active">
				<ul id ="lang-switch-checklist" class="categorychecklist form-no-clear">
					<li>
						<label class="menu-item-title">
							<input type="checkbox" class="menu-item-checkbox" name="menu-item[<?php echo $_nav_menu_placeholder; ?>][menu-item-object-id]" value="-1"> <?php _e('Language switcher', 'polylang'); ?>
						</label>
						<input type="hidden" class="menu-item-type" name="menu-item[<?php echo $_nav_menu_placeholder; ?>][menu-item-type]" value="custom">
						<input type="hidden" class="menu-item-title" name="menu-item[<?php echo $_nav_menu_placeholder; ?>][menu-item-title]" value="<?php _e('Language switcher', 'polylang'); ?>">
						<input type="hidden" class="menu-item-url" name="menu-item[<?php echo $_nav_menu_placeholder; ?>][menu-item-url]" value="#pll_switcher">
	   				</li>
	   			</ul>
	   		</div>
	   		<p class="button-controls">
	   			<span class="add-to-menu">
	   				<input type="submit" <?php disabled( $nav_menu_selected_id, 0 ); ?> class="button-secondary submit-add-to-menu right" value="Add to Menu" name="add-post-type-menu-item" id="submit-posttype-lang-switch">
	   				<span class="spinner"></span>
	   			</span>
	   		</p>
	   	</div><?php
	}

	/*
	 * prepares javascript to modify the language switcher menu item
	 *
	 * @since 1.1
	 */
	public function admin_enqueue_scripts() {
		$screen = get_current_screen();
		if ('nav-menus' != $screen->base)
			return;

		$suffix = defined('SCRIPT_DEBUG') && SCRIPT_DEBUG ? '' : '.min';
		wp_enqueue_script('pll_nav_menu', POLYLANG_URL .'/js/nav-menu'.$suffix.'.js', array('jquery'), POLYLANG_VERSION);

		// the strings for the options
		foreach (array_reverse(PLL_Switcher::get_switcher_options('menu', 'string')) as $str)
			$data['strings'][] = $str;

		$data['strings'][] = __('Language switcher', 'polylang'); // the title

		// get all language switcher menu items
		$items = get_posts(array(
			'numberposts' => -1,
			'nopaging'    => true,
			'post_type'   => 'nav_menu_item',
			'fields'      => 'ids',
			'meta_key'    => '_pll_menu_item'
		));

		// the options values for the language switcher
		$data['val'] = array();
		foreach ($items as $item)
			$data['val'][$item] = get_post_meta($item, '_pll_menu_item', true);

		// send all these data to javascript
		wp_localize_script('pll_nav_menu', 'pll_data', $data);
	}

	/*
	 * save our menu item options
	 *
	 * @since 1.1
	 *
	 * @param int $menu_id not used
	 * @param int $menu_item_db_id
	 */
	public function wp_update_nav_menu_item( $menu_id = 0, $menu_item_db_id = 0 ) {
		if (empty($_POST['menu-item-url'][$menu_item_db_id]) || $_POST['menu-item-url'][$menu_item_db_id] != '#pll_switcher')
			return;

		// security check
		// as 'wp_update_nav_menu_item' can be called from outside WP admin
		if (current_user_can('edit_theme_options')) {
			check_admin_referer( 'update-nav_menu', 'update-nav-menu-nonce' );

			$options = array('hide_current' => 0,'force_home' => 0 ,'show_flags' => 0 ,'show_names' => 1); // default values
			// our jQuery form has not been displayed
			if (empty($_POST['menu-item-pll-detect'][$menu_item_db_id])) {
				if (!get_post_meta($menu_item_db_id, '_pll_menu_item', true)) // our options were never saved
					update_post_meta($menu_item_db_id, '_pll_menu_item', $options);
			}
			else {
				foreach ($options as $opt => $v)
					$options[$opt] = empty($_POST['menu-item-'.$opt][$menu_item_db_id]) ? 0 : 1;
				update_post_meta($menu_item_db_id, '_pll_menu_item', $options); // allow us to easily identify our nav menu item
			}
		}
	}

	/*
	 * translates the language switcher menu items title in case the user switches the admin language
	 *
	 * @since 1.1.1
	 *
	 * @param array $items
	 * @return array modified $items
	 */
	public function translate_switcher_title($items) {
		foreach ($items as $item)
			if ('#pll_switcher' == $item->url)
				$item->post_title = __('Language switcher', 'polylang');
		return $items;
	}

	/*
	 * assign menu languages and translations based on (temporary) locations
	 *
	 * @since 1.1
	 *
	 * @param array $mods theme mods
	 * @return unmodified $mods
	 */
	public function update_nav_menu_locations($mods) {
		if (current_user_can('edit_theme_options') && isset($mods['nav_menu_locations'])) {

			// Manage Locations tab in Appearance -> Menus
			if (isset($_REQUEST['action']) && 'locations' == $_REQUEST['action']) {
				check_admin_referer('save-menu-locations');
				$this->options['nav_menus'][$this->theme] = array();
			}

			// Edit Menus tab in Appearance -> Menus
			// add the test of $_REQUEST['update-nav-menu-nonce'] to avoid conflict with Vantage theme
			elseif (isset($_REQUEST['action'], $_REQUEST['update-nav-menu-nonce']) && 'update' == $_REQUEST['action']) {
				check_admin_referer('update-nav_menu', 'update-nav-menu-nonce');
				$this->options['nav_menus'][$this->theme] = array();
			}

			// customizer
			// don't reset locations in this case.
			// see http://wordpress.org/support/topic/menus-doesnt-show-and-not-saved-in-theme-settings-multilingual-site
			elseif (isset($_REQUEST['action']) && 'customize_save' == $_REQUEST['action']) {
				check_ajax_referer( 'save-customize_' . $GLOBALS['wp_customize']->get_stylesheet(), 'nonce' );
			}

			else
				return $mods; // no modification for nav menu locations

			$default = pll_default_language();

			// extract language and menu from locations
			foreach ($mods['nav_menu_locations'] as $loc => $menu) {
				if ($pos = strpos($loc, '___')) {
					$this->options['nav_menus'][$this->theme][substr($loc, 0, $pos)][substr($loc, $pos+3)] = $menu;
					unset($mods['nav_menu_locations'][$loc]); // remove temporary locations before database update
				}
				else
					$this->options['nav_menus'][$this->theme][$loc][$default] = $menu;
			}

			update_option('polylang', $this->options);
		}
		return $mods;
	}

	/*
	 * fills temporary menu locations based on menus translations
	 *
	 * @since 1.2
	 *
	 * @param bool|array $menus
	 * @return bool|array modified list of menu locations
	 */
	public function nav_menu_locations($menus) {
		if (is_array($menus)) {
			foreach ($menus as $loc => $menu) {
				foreach ($this->model->get_languages_list() as $lang) {
					if (pll_default_language() != $lang->slug && !empty($this->options['nav_menus'][$this->theme][$loc][$lang->slug]))
						$menus[$loc . '___' . $lang->slug] = $this->options['nav_menus'][$this->theme][$loc][$lang->slug];
				}
			}
		}

		return $menus;
	}

	/*
	 * filters _wp_auto_add_pages_to_menu by language
	 *
	 * @since 0.9.4
	 *
	 * @param string $new_status Transition to this post status.
	 * @param string $old_status Previous post status.
	 * @param object $post Post data.
	 */
	public function auto_add_pages_to_menu( $new_status, $old_status, $post ) {
		if ('publish' != $new_status || 'publish' == $old_status || 'page' != $post->post_type || ! empty($post->post_parent) || !($lang = $this->model->get_post_language($post->ID)))
			return;

		// get all the menus in the post language
		$menus = get_terms('nav_menu', array('lang' => $lang->slug, 'fields' => 'ids', 'hide_empty' => false));
		$menus = implode(',', $menus);

		add_filter('option_nav_menu_options', create_function('$a', "\$a['auto_add'] = array_intersect(\$a['auto_add'], array($menus)); return \$a;"));
	}

	/*
	 * FIXME prevents sharing a menu term with a language term by renaming the nav menu before its creation
	 * to avoid http://core.trac.wordpress.org/ticket/24802
	 * and http://wordpress.org/support/topic/all-connection-between-elements-lost
	 *
	 * @since 1.1.3
	 *
	 * @param string $name term name
	 * @param string $taxonomy
	 * @return string modified (nav menu) term name if necessary
	 */
	function pre_insert_term($name, $taxonomy) {
		return ('nav_menu' == $taxonomy && in_array($name, $this->model->get_languages_list(array('fields' => 'name')))) ? $name .= '-menu' : $name;
	}
}
