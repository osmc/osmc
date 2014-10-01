<?php

/*
 * manages Polylang upgrades
 *
 * @since 1.2
 */
class PLL_Upgrade {
	public $options;

	/*
	 * constructor
	 *
	 * @since 1.2
	 */
	public function __construct(&$options) {
		$this->options = &$options;
	}

	/*
	 * upgrades if possible otherwise die to avoid activation
	 *
	 * @since 1.2
	 */
	public function upgrade_at_activation() {
		if (!$this->can_upgrade()) {
			ob_start();
			$this->admin_notices();
			die(ob_get_contents());
		}
	}

	/*
	 * upgrades if possible otherwise returns false to stop Polylang loading
	 *
	 * @since 1.2
	 *
	 * @return bool true if upgrade is possible, false otherwise
	 */
	public function upgrade() {
		if (!$this->can_upgrade()) {
			add_action('all_admin_notices', array(&$this, 'admin_notices'));
			return false;
		}
		return true;
	}


	/*
	 * check if we the previous version is not too old
	 * upgrades if OK
	 * /!\ never start any upgrade before admin_init as it is likely to conflict with some other plugins
	 *
	 * @since 1.2
	 *
	 * @return bool true if upgrade is possible, false otherwise
	 */
	public function can_upgrade() {
		// don't manage upgrade from version < 0.8
		if (version_compare($this->options['version'], '0.8', '<'))
			return false;

		add_action('admin_init', array(&$this, '_upgrade'));
		return true;
	}

	/*
	 * displays a notice when ugrading from a too old version
	 *
	 * @since 1.0
	 */
	public function admin_notices() {
		load_plugin_textdomain('polylang', false, basename(POLYLANG_DIR).'/languages');
		printf(
			'<div class="error"><p>%s</p><p>%s</p></div>',
			__('Polylang has been deactivated because you upgraded from a too old version.', 'polylang'),
			sprintf(
				__('Please upgrade first to %s before ugrading to %s.', 'polylang'),
				'<strong>0.9.8</strong>',
				POLYLANG_VERSION
			)
		);
	}

	/*
	 * upgrades the plugin depending on the previous version
	 *
	 * @since 1.2
	 */
	public function _upgrade() {
		foreach (array('0.9', '1.0', '1.1', '1.2', '1.2.1', '1.2.3', '1.3', '1.4', '1.4.1', '1.4.4', '1.5') as $version)
			if (version_compare($this->options['version'], $version, '<'))
				call_user_func(array(&$this, 'upgrade_' . str_replace('.', '_', $version)));

		if (absint(get_transient('pll_upgrade_1_4')) < time())
			$this->delete_pre_1_2_data();

		$this->options['version'] = POLYLANG_VERSION;
		update_option('polylang', $this->options);
	}

	/*
	 * upgrades if the previous version is < 0.9
	 *
	 * @since 1.2
	 */
	protected function upgrade_0_9() {
		$this->options['sync'] = defined('PLL_SYNC') && !PLL_SYNC ? 0 : 1; // the option replaces PLL_SYNC in 0.9
	}

	/*
	 * upgrades if the previous version is < 1.0
	 *
	 * @since 1.2
	 */
	protected function upgrade_1_0() {
		// the option replaces PLL_MEDIA_SUPPORT in 1.0
		$this->options['media_support'] = defined('PLL_MEDIA_SUPPORT') && !PLL_MEDIA_SUPPORT ? 0 : 1;

		// split the synchronization options in 1.0
		$this->options['sync'] = empty($this->options['sync']) ? array() : array_keys(PLL_Settings::list_metas_to_sync());

		// set default values for post types and taxonomies to translate
		$this->options['post_types'] = array_values(get_post_types(array('_builtin' => false, 'show_ui => true')));
		$this->options['taxonomies'] = array_values(get_taxonomies(array('_builtin' => false, 'show_ui => true')));
		update_option('polylang', $this->options);

		flush_rewrite_rules(); // rewrite rules have been modified in 1.0
	}

	/*
	 * upgrades if the previous version is < 1.1
	 *
	 * @since 1.2
	 */
	protected function upgrade_1_1() {
		// update strings register with icl_register_string
		$strings = get_option('polylang_wpml_strings');
		if ($strings) {
			foreach ($strings as $key => $string)
				$strings[$key]['icl'] = 1;
			update_option('polylang_wpml_strings', $strings);
		}

		// move polylang_widgets options
		if ($widgets = get_option('polylang_widgets')) {
			$this->options['widgets'] = $widgets;
			delete_option('polylang_widgets');
		}
	}

	/*
	 * upgrades if the previous version is < 1.2
	 *
	 * @since 1.2
	 */
	protected function upgrade_1_2() {
		$this->options['domains'] = array(); // option added in 1.2

		// need to register the taxonomies
		foreach (array('language', 'term_language', 'post_translations', 'term_translations') as $taxonomy)
			register_taxonomy($taxonomy, null , array('label' => false, 'public' => false, 'query_var' => false, 'rewrite' => false));

		// abort if the db upgrade has already been done previously
		if (get_terms('term_language', array('hide_empty' => 0)))
			return;

		set_time_limit(0); // in case we upgrade a huge site

		// upgrade old model based on metas to new model based on taxonomies
		global $wpdb;
		$wpdb->termmeta = $wpdb->prefix . 'termmeta'; // registers the termmeta table in wpdb
		$languages = get_terms('language', array('hide_empty' => 0)); // don't use get_languages_list which can't work with the old model

		foreach($languages as $lang) {
			// first update language with new storage for locale and text direction
			$text_direction = get_metadata('term', $lang->term_id, '_rtl', true);
			$desc = serialize(array('locale' => $lang->description, 'rtl' => $text_direction));
			wp_update_term((int) $lang->term_id, 'language', array('description' => $desc));

			// add language to new 'term_language' taxonomy
			$term_lang = wp_insert_term($lang->name, 'term_language', array('slug' => 'pll_' . $lang->slug));
			$lang_tt_ids[$lang->term_id] = $term_lang['term_taxonomy_id']; // keep the term taxonomy id for future
		}

		// get all terms with a language defined
		$terms = $wpdb->get_results("SELECT term_id, meta_value FROM $wpdb->termmeta WHERE meta_key = '_language'");
		foreach ($terms as $key => $term)
			$terms[$key] = $wpdb->prepare('(%d, %d)', $term->term_id, $lang_tt_ids[$term->meta_value]);

		$terms = array_unique($terms);

		// assign language to each term
		if (!empty($terms))
			$wpdb->query("INSERT INTO $wpdb->term_relationships (object_id, term_taxonomy_id) VALUES " . implode(',', $terms));

		// translations
		foreach (array('post', 'term') as $type) {
			$table = $type . 'meta';
			$terms = $slugs = $tts = $trs = array();

			// get all translated objects
			$objects = $wpdb->get_col("SELECT DISTINCT meta_value FROM {$wpdb->$table} WHERE meta_key = '_translations'");

			if (empty($objects))
				continue;

			foreach ($objects as $obj) {
				$term = uniqid('pll_'); // the term name
				$terms[] = $wpdb->prepare('("%1$s", "%1$s")', $term);
				$slugs[] = $wpdb->prepare('"%s"', $term);
				$translations = maybe_unserialize(maybe_unserialize($obj)); // 2 unserialize due to an old storage bug
				$description[$term] = serialize($translations);
			}

			$terms = array_unique($terms);

			// insert terms
			if (!empty($terms))
				$wpdb->query("INSERT INTO $wpdb->terms (slug, name) VALUES " . implode(',', $terms));

			// get all terms with their term_id
			$terms = $wpdb->get_results("SELECT term_id, slug FROM $wpdb->terms WHERE slug IN (" . implode(',', $slugs) . ")");

			// prepare terms taxonomy relationship
			foreach ($terms as $term)
				$tts[] = $wpdb->prepare('(%d, "%s", "%s")', $term->term_id, $type . '_translations', $description[$term->slug]);

			$tts = array_unique($tts);

			// insert term_taxonomy
			if (!empty($tts))
				$wpdb->query("INSERT INTO $wpdb->term_taxonomy (term_id, taxonomy, description) VALUES " . implode(',', $tts));

			// get all terms with term_taxonomy_id
			$terms = get_terms($type . '_translations', array('hide_empty' => false));

			// prepare objects relationships
			foreach ($terms as $term) {
				$translations = unserialize($term->description);
				foreach ($translations as $object_id)
					if (!empty($object_id))
						$trs[] = $wpdb->prepare('(%d, %d)', $object_id, $term->term_taxonomy_id);
			}

			$trs = array_unique($trs);

			// insert term_relationships
			if (!empty($trs))
				$wpdb->query("INSERT INTO $wpdb->term_relationships (object_id, term_taxonomy_id) VALUES " . implode(',', $trs));
		}

		// upgrade of string translations is now in upgrade_1_2_1
		// upgrade of nav menus is now in upgrade_1_2_3
	}

	/*
	 * upgrades if the previous version is < 1.2.1
	 *
	 * @since 1.2.1
	 */
	protected function upgrade_1_2_1() {
		// strings translations
		foreach(get_terms('language', array('hide_empty' => 0)) as $lang) {
			if ($strings = get_option('polylang_mo'.$lang->term_id)) {
				$mo = new PLL_MO();
				foreach ($strings as $msg)
					$mo->add_entry($mo->make_entry($msg[0], $msg[1]));
				$mo->export_to_db($lang);
			}
		}
	}

	/*
	 * upgrades if the previous version is < 1.2.3
	 * uprades multilingual menus depending on the old version due to multiple changes in menus management
	 *
	 * @since 1.2.3
	 */
	public function upgrade_1_2_3() {
		// old version < 1.1
		// multilingal locations and switcher item were stored in a dedicated option
		if (version_compare($this->options['version'], '1.1', '<')) {
			if ($menu_lang = get_option('polylang_nav_menus')) {
				foreach ($menu_lang as $location => $arr) {
					if (!in_array($location, array_keys(get_registered_nav_menus())))
						continue;

					$switch_options = array_slice($arr, -5, 5);
					$translations = array_diff_key($arr, $switch_options);
					$has_switcher = array_shift($switch_options);

					foreach (get_terms('language', array('hide_empty' => 0)) as $lang) {
						// move nav menus locations
						if (!empty($translations[$lang->slug]))
							$locations[$location][$lang->slug] = $translations[$lang->slug];

						// create the menu items for the language switcher
						if (!empty($has_switcher)) {
							$menu_item_db_id = wp_update_nav_menu_item($translations[$lang->slug], 0, array(
								'menu-item-title' => __('Language switcher', 'polylang'),
								'menu-item-url' => '#pll_switcher',
								'menu-item-status' => 'publish'
							));

							update_post_meta($menu_item_db_id, '_pll_menu_item', $switch_options);
						}
					}
				}

				if (!empty($locations))
					$this->options['nav_menus'][get_option('stylesheet')] = $locations;

				delete_option('polylang_nav_menus');
			}

		}

		elseif (empty($this->options['nav_menus'])) {
			$menus = get_theme_mod('nav_menu_locations');

			if (is_array($menus)) {
				// if old version < 1.2
				// clean the WP option as it was a bad idea to pollute it
				if (version_compare($this->options['version'], '1.2', '<')) {
					foreach ($menus as $loc => $menu) {
						if ($pos = strpos($loc, '#'))
							unset($menus[$loc]);
					}

					set_theme_mod('nav_menu_locations', $menus);
				}

				// get the multilingual locations
				foreach ($menus as $loc => $menu) {
					foreach (get_terms('language', array('hide_empty' => 0)) as $lang) {
						$arr[$loc][$lang->slug] = pll_get_term($menu, $lang);
					}
				}

				if (!empty($arr))
					$this->options['nav_menus'][get_option('stylesheet')] = $arr;
			}
		}
	}

	/*
	 * upgrades if the previous version is < 1.3
	 * moves the user biographies in default language to the 'description' user meta
	 *
	 * @since 1.3
	 */
	protected function upgrade_1_3() {
		$usermeta = 'description_' . $this->options['default_lang'];
		$query = new WP_User_Query(array('blog_id' => $GLOBALS['blog_id'], 'meta_key' => $usermeta));

		foreach ($query->get_results() as $user) {
			$desc = get_user_meta($user->ID, $usermeta, true);
			if (!empty($desc)) {
				update_user_meta($user->ID, 'description', $desc);
				delete_user_meta($user->ID, $usermeta);
			}
		}
	}

	/*
	 * upgrades if the previous version is < 1.4
	 * sets a transient to delete old model data
	 * deletes language cache (due to bug correction in home urls in 1.3.1 and added mo_id in 1.4)
	 *
	 * @since 1.4
	 */
	protected function upgrade_1_4() {
		set_transient('pll_upgrade_1_4', time() + 60 * 24 * 60 * 60); // 60 days
		delete_transient('pll_languages_list');
	}

	/*
	 * old data were not deleted in 1.2, just in case...
	 * delete them at first upgrade at least 60 days after upgrade to 1.4
	 *
	 * @since 1.4
	 */
	protected function delete_pre_1_2_data() {
		// suppress data of the old model < 1.2
		global $wpdb;
		$wpdb->termmeta = $wpdb->prefix . 'termmeta'; // registers the termmeta table in wpdb

		// do nothing if the termmeta table does not exists
		if (count($wpdb->get_results("SHOW TABLES LIKE '$wpdb->termmeta'"))) {
			$wpdb->query("DELETE FROM $wpdb->postmeta WHERE meta_key = '_translations'");
			$wpdb->query("DELETE FROM $wpdb->termmeta WHERE meta_key = '_language'");
			$wpdb->query("DELETE FROM $wpdb->termmeta WHERE meta_key = '_rtl'");
			$wpdb->query("DELETE FROM $wpdb->termmeta WHERE meta_key = '_translations'");

			// delete the termmeta table only if it is empty as other plugins may use it
			if (!$wpdb->get_var("SELECT COUNT(*) FROM $wpdb->termmeta;"))
				$wpdb->query("DROP TABLE $wpdb->termmeta;");
		}

		// delete the strings translations
		$languages = get_terms('language', array('hide_empty'=>false));
		foreach ($languages as $lang)
			delete_option('polylang_mo'.$lang->term_id);

		delete_transient('pll_upgrade_1_4');
	}

	/*
	 * upgrades if the previous version is < 1.4.1
	 * disables the browser detection when using multiple domains
	 *
	 * @since 1.4.1
	 */
	protected function upgrade_1_4_1() {
		if (3 == $this->options['force_lang'])
			$this->options['browser'] = $this->options['hide_default'] = 0;
	}

	/*
	 * upgrades if the previous version is < 1.4.4
	 * uprades widgets options for language filter
	 *
	 * @since 1.4.4
	 */
	protected function upgrade_1_4_4() {
		foreach ($GLOBALS['wp_registered_widgets'] as $widget) {
			if (!empty($this->options['widgets'][$widget['id']]) && !empty($widget['callback'][0]) && !empty($widget['params'][0]['number'])) {
				$obj = $widget['callback'][0];
				if (is_object($obj) && method_exists($obj, 'get_settings') && method_exists($obj, 'save_settings')) {
					$settings = $obj->get_settings();
					$settings[$widget['params'][0]['number']]['pll_lang'] = $this->options['widgets'][$widget['id']];
					$obj->save_settings($settings);
				}
			}
		}
		unset($this->options['widgets']);
	}

	/*
	 * upgrades if the previous version is < 1.5
	 * deletes language cache (due to host property added and bug on search url)
	 *
	 * @since 1.5
	 */
	protected function upgrade_1_5() {
		delete_transient('pll_languages_list');
	}

}
