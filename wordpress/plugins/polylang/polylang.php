<?php
/*
Plugin Name: Polylang
Plugin URI: http://polylang.wordpress.com/
Version: 1.5.6
Author: Frédéric Demarle
Description: Adds multilingual capability to WordPress
Text Domain: polylang
Domain Path: /languages
*/

/*
 * Copyright 2011-2014 Frédéric Demarle
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

// don't access directly
if (!function_exists('add_action'))
	exit();

define('POLYLANG_VERSION', '1.5.6');
define('PLL_MIN_WP_VERSION', '3.5');

define('POLYLANG_BASENAME', plugin_basename(__FILE__)); // plugin name as known by WP

define('POLYLANG_DIR', dirname(__FILE__)); // our directory
define('PLL_INC', POLYLANG_DIR . '/include');
define('PLL_FRONT_INC',  POLYLANG_DIR . '/frontend');
define('PLL_ADMIN_INC',  POLYLANG_DIR . '/admin');

// default directory to store user data such as custom flags
if (!defined('PLL_LOCAL_DIR'))
	define('PLL_LOCAL_DIR', WP_CONTENT_DIR . '/polylang');

// includes local config file if exists
if (file_exists(PLL_LOCAL_DIR . '/pll-config.php'))
	include_once(PLL_LOCAL_DIR . '/pll-config.php');

// our url. Don't use WP_PLUGIN_URL http://wordpress.org/support/topic/ssl-doesnt-work-properly
define('POLYLANG_URL', plugins_url('/' . basename(POLYLANG_DIR)));

// default url to access user data such as custom flags
if (!defined('PLL_LOCAL_URL'))
	define('PLL_LOCAL_URL', content_url('/polylang'));

// cookie name. no cookie will be used if set to false
if (!defined('PLL_COOKIE'))
	define('PLL_COOKIE', 'pll_language');

// backward compatibility WP < 3.6
// the search form js is no more needed in WP 3.6+ except if the search form is hardcoded elsewhere than in searchform.php
if (!defined('PLL_SEARCH_FORM_JS') && !version_compare($GLOBALS['wp_version'], '3.6', '<'))
	define('PLL_SEARCH_FORM_JS', false);

/*
 * controls the plugin, as well as activation, and deactivation
 *
 * @since 0.1
 */
class Polylang {

	/*
	 * constructor
	 *
	 * @since 0.1
	 */
	public function __construct() {
		// manages plugin activation and deactivation
		register_activation_hook( __FILE__, array(&$this, 'activate'));
		register_deactivation_hook( __FILE__, array(&$this, 'deactivate'));

		// stopping here if we are going to deactivate the plugin (avoids breaking rewrite rules)
		if (isset($_GET['action'], $_GET['plugin']) && 'deactivate' == $_GET['action'] && plugin_basename(__FILE__) == $_GET['plugin'])
			return;

		// avoid loading polylang admin for frontend ajax requests
		// special test for plupload which does not use jquery ajax and thus does not pass our ajax prefilter
		// special test for customize_save done in frontend but for which we want to load the admin
		if (!defined('PLL_AJAX_ON_FRONT')) {
			$in = isset($_REQUEST['action']) && in_array($_REQUEST['action'], array('upload-attachment', 'customize_save'));
			define('PLL_AJAX_ON_FRONT', defined('DOING_AJAX') && DOING_AJAX && empty($_REQUEST['pll_ajax_backend']) && !$in);
		}

		if (!defined('PLL_ADMIN'))
			define('PLL_ADMIN', defined('DOING_CRON') || (is_admin() && !PLL_AJAX_ON_FRONT));

		if (!defined('PLL_SETTINGS'))
			define('PLL_SETTINGS', is_admin() && isset($_GET['page']) && $_GET['page'] == 'mlang');

		// blog creation on multisite
		add_action('wpmu_new_blog', array(&$this, 'wpmu_new_blog'), 5); // before WP attempts to send mails which can break on some PHP versions

		// FIXME maybe not available on every installations but widely used by WP plugins
		spl_autoload_register(array(&$this, 'autoload')); // autoload classes

		// override load text domain waiting for the language to be defined
		// here for plugins which load text domain as soon as loaded :(
		if (!defined('PLL_OLT') || PLL_OLT)
			new PLL_OLT_Manager();

		// plugin initialization
		// take no action before all plugins are loaded
		add_action('plugins_loaded', array(&$this, 'init'), 1);

		// loads the API
		require_once(PLL_INC.'/api.php');

		// WPML API
		if (!defined('PLL_WPML_COMPAT') || PLL_WPML_COMPAT)
			require_once (PLL_INC.'/wpml-compat.php');

		// extra code for compatibility with some plugins
		if (!defined('PLL_PLUGINS_COMPAT') || PLL_PLUGINS_COMPAT)
			new PLL_Plugins_Compat();
	}

	/*
	 * activation or deactivation for all blogs
	 *
	 * @since 1.2
	 *
	 * @param string $what either 'activate' or 'deactivate'
	 */
	protected function do_for_all_blogs($what) {
		// network
		if (is_multisite() && isset($_GET['networkwide']) && ($_GET['networkwide'] == 1)) {
			global $wpdb;

			foreach ($wpdb->get_col("SELECT blog_id FROM $wpdb->blogs") as $blog_id) {
				switch_to_blog($blog_id);
				$what == 'activate' ? $this->_activate() : $this->_deactivate();
			}
			restore_current_blog();
		}

		// single blog
		else
			$what == 'activate' ? $this->_activate() : $this->_deactivate();
	}

	/*
	 * plugin activation for multisite
	 *
	 * @since 0.1
	 */
	public function activate() {
		global $wp_version;
		load_plugin_textdomain('polylang', false, basename(POLYLANG_DIR).'/languages'); // plugin i18n

		if (version_compare($wp_version, PLL_MIN_WP_VERSION , '<'))
			die (sprintf('<p style = "font-family: sans-serif; font-size: 12px; color: #333; margin: -5px">%s</p>',
				sprintf(__('You are using WordPress %s. Polylang requires at least WordPress %s.', 'polylang'),
					esc_html($wp_version),
					PLL_MIN_WP_VERSION
				)
			));

		$this->do_for_all_blogs('activate');
	}

	/*
	 * plugin activation
	 *
	 * @since 0.5
	 */
	protected function _activate() {
		global $polylang;

		if ($options = get_option('polylang')) {
			// plugin upgrade
			if (version_compare($options['version'], POLYLANG_VERSION, '<')) {
				$upgrade = new PLL_Upgrade($options);
				$upgrade->upgrade_at_activation();
			}
		}
		// defines default values for options in case this is the first installation
		else {
			$options = array(
				'browser'       => 1, // default language for the front page is set by browser preference
				'rewrite'       => 1, // remove /language/ in permalinks (was the opposite before 0.7.2)
				'hide_default'  => 0, // do not remove URL language information for default language
				'force_lang'    => 0, // do not add URL language information when useless
				'redirect_lang' => 0, // do not redirect the language page to the homepage
				'media_support' => 1, // support languages and translation for media by default
				'sync'          => array(), // synchronisation is disabled by default (was the opposite before 1.2)
				'post_types'    => array_values(get_post_types(array('_builtin' => false, 'show_ui => true'))),
				'taxonomies'    => array_values(get_taxonomies(array('_builtin' => false, 'show_ui => true'))),
				'domains'       => array(),
				'version'       => POLYLANG_VERSION,
			);

			update_option('polylang', $options);
		}

		// always provide a global $polylang object and add our rewrite rules if needed
		$polylang = new StdClass();
		$polylang->options = &$options;
		$polylang->model = new PLL_Admin_Model($options);
		$polylang->links_model = $polylang->model->get_links_model();
		flush_rewrite_rules();
	}

	/*
	 * plugin deactivation for multisite
	 *
	 * @since 0.1
	 */
	public function deactivate() {
		$this->do_for_all_blogs('deactivate');
	}

	/*
	 * plugin deactivation
	 *
	 * @since 0.5
	 */
	protected function _deactivate() {
		flush_rewrite_rules();
	}

	/*
	 * blog creation on multisite (to set default options)
	 *
	 * @since 0.9.4
	 *
	 * @param int $blog_id
	 */
	public function wpmu_new_blog($blog_id) {
		switch_to_blog($blog_id);
		$this->_activate();
		restore_current_blog();
	}

	/*
	 * autoload classes
	 *
	 * @since 1.2
	 *
	 * @param string $class
	 */
	public function autoload($class) {
		$class = str_replace('_', '-', strtolower(substr($class, 4)));
		foreach (array(PLL_INC, PLL_FRONT_INC, PLL_ADMIN_INC) as $path) {
			if (file_exists($file = "$path/$class.php")) {
				require_once($file);
				break;
			}
		}
	}

	/*
	 * Polylang initialization
	 * setups models and separate admin and frontend
	 *
	 * @since 1.2
	 */
	public function init() {
		global $polylang;

		$options = get_option('polylang');

		// plugin upgrade
		if ($options && version_compare($options['version'], POLYLANG_VERSION, '<')) {
			$upgrade = new PLL_Upgrade($options);
			if (!$upgrade->upgrade()) // if the version is too old
				return;
		}

		$class = apply_filters('pll_model', PLL_SETTINGS ? 'PLL_Admin_Model' : 'PLL_Model');
		$model = new $class($options);
		$links_model = $model->get_links_model();

		if (PLL_ADMIN) {
			$polylang = new PLL_Admin($links_model);
			$polylang->init();
		}
		// do nothing on frontend if no language is defined
		elseif ($model->get_languages_list()) {
			$polylang = new PLL_Frontend($links_model);
			$polylang->init();
		}

		if (!$model->get_languages_list())
			do_action('pll_no_language_defined'); // to load overriden textdomains

		// load wpml-config.xml
		if (!defined('PLL_WPML_COMPAT') || PLL_WPML_COMPAT)
			new PLL_WPML_Config;
	}
}

new Polylang();
