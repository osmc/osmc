<?php

/*
 * a class for the Polylang settings pages
 *
 * @since 1.2
 */
class PLL_Settings {
	public $links_model, $model, $options;
	protected $active_tab, $strings = array(); // strings to translate

	/*
	 * constructor
	 *
	 * @since 1.2
	 *
	 * @param object $polylang
	 */
	public function __construct(&$polylang) {
		$this->links_model = &$polylang->links_model;
		$this->model = &$polylang->model;
		$this->options = &$polylang->options;

		$this->active_tab = !empty($_GET['tab']) ? $_GET['tab'] : 'lang';

		// adds screen options and the about box in the languages admin panel
		add_action('load-settings_page_mlang',  array(&$this, 'load_page'));

		// saves per-page value in screen option
		add_filter('set-screen-option', create_function('$s, $o, $v', 'return $v;'), 10, 3);
	}

	/*
	 * adds screen options and the about box in the languages admin panel
	 *
	 * @since 0.9.5
	 */
	public function load_page() {
		// test of $this->active_tab avoids displaying the automatically generated screen options on other tabs
		switch ($this->active_tab) {
			case 'lang':
				if (!defined('PLL_DISPLAY_ABOUT') || PLL_DISPLAY_ABOUT) {
					add_meta_box(
						'pll_about_box',
						__('About Polylang', 'polylang'),
						create_function('', "include(PLL_ADMIN_INC.'/view-about.php');"),
						'settings_page_mlang',
						'normal'
					);
				}

				add_screen_option('per_page', array(
					'label'   => __('Languages', 'polylang'),
					'default' => 10,
					'option'  => 'pll_lang_per_page'
				));
				break;

			case 'strings':
				add_screen_option('per_page', array(
					'label'   => __('Strings translations', 'polylang'),
					'default' => 10,
					'option'  => 'pll_strings_per_page'
				));
				break;

			default:
				break;
		}
	}

	/*
	 * diplays the 3 tabs pages: languages, strings translations, settings
	 * also manages user input for these pages
	 *
	 * @since 0.1
	 */
	public function languages_page() {
		// prepare the list of tabs
		$tabs = array('lang' => __('Languages','polylang'));

		// only if at least one language has been created
		if ($listlanguages = $this->model->get_languages_list()) {
			$tabs['strings'] = __('Strings translation','polylang');
			$tabs['settings'] = __('Settings', 'polylang');
		}

		$tabs = apply_filters('pll_settings_tabs', $tabs);

		switch($this->active_tab) {
			case 'lang':
				// prepare the list table of languages
				$list_table = new PLL_Table_Languages();
				$list_table->prepare_items($listlanguages);
				break;

			case 'strings':
				// get the strings to translate
				$data = $this->get_strings();

				$selected = empty($_REQUEST['group']) ? -1 : $_REQUEST['group'];
				foreach ($data as $key=>$row) {
					$groups[] = $row['context']; // get the groups

					// filter for search string
					if (($selected !=-1 && $row['context'] != $selected) || (!empty($_REQUEST['s']) && stripos($row['name'], $_REQUEST['s']) === false && stripos($row['string'], $_REQUEST['s']) === false))
						unset ($data[$key]);
				}

				$groups = array_unique($groups);

				// load translations
				foreach ($listlanguages as $language) {
					// filters by language if requested
					if (($lg = get_user_meta(get_current_user_id(), 'pll_filter_content', true)) && $language->slug != $lg)
						continue;

					$mo = new PLL_MO();
					$mo->import_from_db($language);
					foreach ($data as $key=>$row) {
						$data[$key]['translations'][$language->name] = $mo->translate($row['string']);
						$data[$key]['row'] = $key; // store the row number for convenience
					}
				}

				$string_table = new PLL_Table_String($groups, $selected);
				$string_table->prepare_items($data);
				break;

			case 'settings':
				$post_types = get_post_types(array('public' => true, '_builtin' => false));
				$post_types = array_diff($post_types, get_post_types(array('_pll' => true)));
				$post_types = array_unique(apply_filters('pll_get_post_types', $post_types, true));

				$taxonomies = get_taxonomies(array('public' => true, '_builtin' => false));
				$taxonomies = array_diff($taxonomies, get_taxonomies(array('_pll' => true)));
				$taxonomies = array_unique(apply_filters('pll_get_taxonomies', $taxonomies , true));
				break;

			default:
				break;
		}

		$using_permalinks = $GLOBALS['wp_rewrite']->using_permalinks();

		$action = isset($_REQUEST['pll_action']) ? $_REQUEST['pll_action'] : '';

		switch ($action) {
			case 'add':
				check_admin_referer( 'add-lang', '_wpnonce_add-lang' );

				if ($this->model->add_language($_POST))
					PLL_Admin::download_mo($_POST['locale']);

				$this->redirect(); // to refresh the page (possible thanks to the $_GET['noheader']=true)
				break;

			case 'delete':
				check_admin_referer('delete-lang');

				if (!empty($_GET['lang']))
					$this->model->delete_language((int) $_GET['lang']);

				$this->redirect(); // to refresh the page (possible thanks to the $_GET['noheader']=true)
				break;

			case 'edit':
				if (!empty($_GET['lang']))
					$edit_lang = $this->model->get_language((int) $_GET['lang']);
				break;

			case 'update':
				check_admin_referer( 'add-lang', '_wpnonce_add-lang' );

				$error = $this->model->update_language($_POST);

				$this->redirect(); // to refresh the page (possible thanks to the $_GET['noheader']=true)
				break;

			case 'string-translation':
				if (!empty($_REQUEST['submit'])) {
					check_admin_referer( 'string-translation', '_wpnonce_string-translation' );
					$strings = $this->get_strings();

					foreach ($this->model->get_languages_list() as $language) {
						if(empty($_POST['translation'][$language->name])) // in case the language filter is active (thanks to John P. Bloch)
							continue;

						$mo = new PLL_MO();
						$mo->import_from_db($language);

						foreach ($_POST['translation'][$language->name] as $key=>$translation)
							$mo->add_entry($mo->make_entry($strings[$key]['string'], stripslashes($translation)));

						// clean database (removes all strings which were registered some day but are no more)
						if (!empty($_POST['clean'])) {
							$new_mo = new PLL_MO();

							foreach ($strings as $string)
								$new_mo->add_entry($mo->make_entry($string['string'], $mo->translate($string['string'])));
						}

						isset($new_mo) ? $new_mo->export_to_db($language) : $mo->export_to_db($language);
					}
					add_settings_error('general', 'pll_strings_translations_updated', __('Translations updated.', 'polylang'), 'updated');
				}

				do_action('pll_save_strings_translations');

				// unregisters strings registered through WPML API
				if ($string_table->current_action() == 'delete' && !empty($_REQUEST['strings']) && function_exists('icl_unregister_string')) {
					check_admin_referer( 'string-translation', '_wpnonce_string-translation' );
					$strings = $this->get_strings();

					foreach ($_REQUEST['strings'] as $key)
						icl_unregister_string($strings[$key]['context'], $strings[$key]['name']);
				}

				// to refresh the page (possible thanks to the $_GET['noheader']=true)
				$this->redirect(array_intersect_key($_REQUEST, array_flip(array('s', 'paged', 'group'))));
				break;

			case 'options':
				check_admin_referer( 'options-lang', '_wpnonce_options-lang' );

				$this->options['default_lang'] = sanitize_title($_POST['default_lang']); // we have slug as value

				foreach(array('force_lang', 'rewrite') as $key)
					$this->options[$key] = isset($_POST[$key]) ? (int) $_POST[$key] : 0;

				// FIXME : TODO error message if not a valid url
				if (3 == $this->options['force_lang'] && isset($_POST['domains']) && is_array($_POST['domains'])) {
					foreach ($_POST['domains'] as $key => $domain) {
						$this->options['domains'][$key] = esc_url_raw(trim($domain));
					}
					$this->options['domains'][$this->options['default_lang']] = get_option('home');
				}

				foreach (array('browser', 'hide_default', 'redirect_lang', 'media_support') as $key)
					$this->options[$key] = isset($_POST[$key]) ? 1 : 0;

				if (3 == $this->options['force_lang'])
					$this->options['browser'] = $this->options['hide_default'] = 0;

				foreach (array('sync', 'post_types', 'taxonomies') as $key)
					$this->options[$key] = empty($_POST[$key]) ? array() : array_keys($_POST[$key], 1);

				update_option('polylang', $this->options);

				// refresh rewrite rules in case rewrite,  hide_default, post types or taxonomies options have been modified
				// it seems useless to refresh permastruct here
				flush_rewrite_rules();

				// refresh language cache in case home urls have been modified
				$this->model->clean_languages_cache();

				// fills existing posts & terms with default language
				if (isset($_POST['fill_languages']) && $nolang = $this->model->get_objects_with_no_lang()) {
					if (!empty($nolang['posts']))
						$this->model->set_language_in_mass('post', $nolang['posts'], $this->options['default_lang']);
					if (!empty($nolang['terms']))
						$this->model->set_language_in_mass('term', $nolang['terms'], $this->options['default_lang']);
				}

				add_settings_error('general', 'settings_updated', __('Settings saved.'), 'updated');
				$this->redirect();
				break;

			default:
				break;
		}

		// displays the page
		include(PLL_ADMIN_INC.'/view-languages.php');
	}

	/*
	 * register strings for translation making sure it is not duplicate or empty
	 *
	 * @since 0.6
	 *
	 * @param string $name a unique name for the string
	 * @param string $string the string to register
	 * @param string $context optional the group in which the string is registered, defaults to 'polylang'
	 * @param bool $multiline optional wether the string table should display a multiline textarea or a single line input, defaults to single line
	 */
	public function register_string($name, $string, $context = 'polylang', $multiline = false) {
		// backward compatibility with Polylang older than 1.1
		if (is_bool($context)) {
			$multiline = $context;
			$context = 'polylang';
		}

		$to_register = compact('name', 'string', 'context', 'multiline');
		if (!in_array($to_register, $this->strings) && $to_register['string'])
			$this->strings[] = $to_register;
	}

	/*
	 * get registered strings
	 *
	 * @since 0.6.1
	 *
	 * @return array list of all registered strings
	 */
	public function &get_strings() {
		// WP strings
		$this->register_string(__('Site Title'), get_option('blogname'), 'WordPress');
		$this->register_string(__('Tagline'), get_option('blogdescription'), 'WordPress');
		$this->register_string(__('Date Format'), get_option('date_format'), 'WordPress');
		$this->register_string(__('Time Format'), get_option('time_format'), 'WordPress');

		// widgets titles
		global $wp_registered_widgets;
		$sidebars = wp_get_sidebars_widgets();
		foreach ($sidebars as $sidebar => $widgets) {
			if ($sidebar == 'wp_inactive_widgets' || empty($widgets))
				continue;

			foreach ($widgets as $widget) {
				// nothing can be done if the widget is created using pre WP2.8 API :(
				// there is no object, so we can't access it to get the widget options
				if (!isset($wp_registered_widgets[$widget]['callback'][0]) || !is_object($wp_registered_widgets[$widget]['callback'][0]) || !method_exists($wp_registered_widgets[$widget]['callback'][0], 'get_settings'))
					continue;

				$widget_settings = $wp_registered_widgets[$widget]['callback'][0]->get_settings();
				$number = $wp_registered_widgets[$widget]['params'][0]['number'];
				// don't enable widget title translation if the widget is visible in only one language or if there is no title
				if (empty($widget_settings[$number]['pll_lang']) && isset($widget_settings[$number]['title']) && $title = $widget_settings[$number]['title'])
					$this->register_string(__('Widget title', 'polylang'), $title, 'Widget');
			}
		}

		// allow plugins to modify our list of strings, mainly for use by our PLL_WPML_Compat class
		$this->strings = apply_filters('pll_get_strings', $this->strings);
		return $this->strings;
	}

	/*
	 * list the post metas to synchronize
	 *
	 * @since 1.0
	 *
	 * @return array
	 */
	static public function list_metas_to_sync() {
		return array(
			'taxonomies'        => __('Taxonomies', 'polylang'),
			'post_meta'         => __('Custom fields', 'polylang'),
			'comment_status'    => __('Comment status', 'polylang'),
			'ping_status'       => __('Ping status', 'polylang'),
			'sticky_posts'      => __('Sticky posts', 'polylang'),
			'post_date'         => __('Published date', 'polylang'),
			'post_format'       => __('Post format', 'polylang'),
			'post_parent'       => __('Page parent', 'polylang'),
			'_wp_page_template' => __('Page template', 'polylang'),
			'menu_order'        => __('Page order', 'polylang'),
			'_thumbnail_id'     => __('Featured image', 'polylang'),
		);
	}

	/*
	 * redirects to language page (current active tab)
	 * saves error messages in a transient for reuse in redirected page
	 *
	 * @since 1.5
	 *
	 * @param array $args query arguments to add to the url
	 */
	protected function redirect($args = array()) {
		if ($errors = get_settings_errors()) {
			set_transient('settings_errors', $errors, 30);
			$args['settings-updated'] = 1;
		}
		wp_redirect(add_query_arg($args,  wp_get_referer() ));
		exit;
	}
}
