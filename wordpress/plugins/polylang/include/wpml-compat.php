<?php
/*
 * compatibility with WPML API. See http://wpml.org/documentation/support/wpml-coding-api/
 */

/*
 * defines two WPML constants once the language has been defined
 * the compatibility with WPML is not perfect on admin side as the constants are defined
 * in 'setup_theme' by Polylang (based on user info) and 'plugins_loaded' by WPML (based on cookie)
 *
 * @since 0.9.5
 */
function pll_define_wpml_constants() {
	global $polylang;

	if (!empty($polylang->curlang)) {
		if(!defined('ICL_LANGUAGE_CODE'))
			define('ICL_LANGUAGE_CODE', $polylang->curlang->slug);

		if(!defined('ICL_LANGUAGE_NAME'))
			define('ICL_LANGUAGE_NAME', $polylang->curlang->name);
	}

	elseif (PLL_ADMIN) {
		if(!defined('ICL_LANGUAGE_CODE'))
			define('ICL_LANGUAGE_CODE', 'all');

		if(!defined('ICL_LANGUAGE_NAME'))
			define('ICL_LANGUAGE_NAME', '');
	}
}

add_action('pll_language_defined', 'pll_define_wpml_constants');

/*
 * link to the home page in the active language
 *
 * @since 0.9.4
 *
 * @return string
 */
if (!function_exists('icl_get_home_url')) {
	function icl_get_home_url() {
		return pll_home_url();
	}
}

/*
 * used for building custom language selectors
 * available only on frontend
 *
 * list of paramaters accepted in $args
 *
 * skip_missing  => wether to skip missing translation or not, 0 or 1, defaults to 0
 * orderby       => 'id', 'cod', 'name', defaults to 'id'
 * order         => 'ASC' or 'DESC', defaults to 'ASC'
 * link_empty_to => link to use when the translation is missing {$lang} is replaced by the language code
 *
 * list of parameters returned per language:
 *
 * id               => the language id
 * active           => wether this is the active language or no, 0 or 1
 * native_name      => the language name
 * missing          => wether the translation is missing or not, 0 or 1
 * translated_name  => empty, does not exist in Polylang
 * language_code    => the language code (slug)
 * country_flag_url => the url of the flag
 * url              => the url of the translation
 *
 * @since 1.0
 *
 * @param string|array $args optional
 * @return array array of arrays per language
 */
if (!function_exists('icl_get_languages')) {
	function icl_get_languages($args = '') {
		global $polylang;
		if (empty($polylang) || !($polylang instanceof PLL_Frontend) || empty($polylang->curlang))
			return array();

		$args = extract(wp_parse_args($args));
		$orderby = (isset($orderby) && $orderby == 'code') ? 'slug' : (isset($orderby) && $orderby == 'name' ? 'name' : 'id');
		$order = (!empty($order) && $order == 'desc') ? 'DESC' : 'ASC';

		$arr = array();

		foreach ($polylang->model->get_languages_list(array('hide_empty' => true, 'orderby' => $orderby, 'order' => $order)) as $lang) {
			$url = $polylang->links->get_translation_url($lang);

			if (empty($url) && !empty($skip_missing))
				continue;

			$arr[] = array(
				'id'               => $lang->term_id,
				'active'           => isset($polylang->curlang->slug) && $polylang->curlang->slug == $lang->slug ? 1 : 0,
				'native_name'      => $lang->name,
				'missing'          => empty($url) ? 1 : 0,
				'translated_name'  => '', // does not exist in Polylang
				'language_code'    => $lang->slug,
				'country_flag_url' => $lang->flag_url,
				'url'              => $url ? $url :
					(empty($link_empty_to) ? $polylang->links->get_home_url($lang) :
					str_replace('{$lang}', $lang->slug, $link_empty_to))
			);
		}
		return $arr;
	}
}

/*
 * used for creating language dependent links in themes
 *
 * @since 1.0
 *
 * @param int $id object id
 * @param string $type optional, post type or taxonomy name of the object, defaults to 'post'
 * @param string $text optional the link text. If not specified will produce the name of the element in the current language
 * @param array $args optional an array of arguments to add to the link, defaults to empty
 * @param string $anchor optional the anchor to add to teh link, defaults to empty
 * @return string a language dependent link
 */
if (!function_exists('icl_link_to_element')) {
	function icl_link_to_element($id, $type = 'post', $text = '', $args = array(), $anchor = '') {
		global $polylang;

		if ($type == 'tag')
			$type = 'post_tag';

		if (isset($polylang) && ($lang = pll_current_language()) && ($tr_id = $polylang->model->get_translation($type, $id, $lang)) && $polylang->links->current_user_can_read($tr_id))
			$id = $tr_id;

		if (post_type_exists($type)) {
			$link = get_permalink($id);
			if (empty($text))
				$text = get_the_title($id);
		}
		elseif (taxonomy_exists($type)) {
			$link = get_term_link($id, $type);
			if (empty($text) && ($term = get_term($id, $type)) && !empty($term) && !is_wp_error($term))
				$text = $term->name;
		}

		if (empty($link) || is_wp_error($link))
			return '';

		if (!empty($args))
			$link .= (false === strpos($link, '?') ? '?' : '&' ) . http_build_query($args);

		if (!empty($anchor))
			$link .= '#' . $anchor;

		return sprintf('<a href="%s">%s</a>', esc_url($link), esc_html($text));
	}
}

/*
 * used for calculating the IDs of objects (usually categories) in the current language
 *
 * @since 0.9.5
 *
 * @param int $id object id
 * @param string $type, post type or taxonomy name of the object, defaults to 'post'
 * @param bool $return_original_if_missing optional, true if Polylang should return the original id if the translation is missing, defaults to false
 * @param string $lang optional language code, defaults to current language
 * @return int|null the object id of the translation, null if the translation is missing and $return_original_if_missing set to false
 */
if (!function_exists('icl_object_id')) {
	function icl_object_id($id, $type, $return_original_if_missing = false, $lang = false) {
		global $polylang;
		return isset($polylang) && ($lang = $lang ? $lang : pll_current_language()) && ($tr_id = $polylang->model->get_translation($type, $id, $lang)) ? $tr_id :
			($return_original_if_missing ? $id : null);
	}
}

/*
 * registers a string for translation in the "strings translation" panel
 *
 * @since 0.9.3
 *
 * @param string $context the group in which the string is registered, defaults to 'polylang'
 * @param string $name a unique name for the string
 * @param string $string the string to register
 */
if (!function_exists('icl_register_string')) {
	function icl_register_string($context, $name, $string) {
		$GLOBALS['pll_wpml_compat']->register_string($context, $name, $string);
	}
}

/*
 * removes a string from the "strings translation" panel
 *
 * @since 1.0.2
 *
 * @param string $context the group in which the string is registered, defaults to 'polylang'
 * @param string $name a unique name for the string
 */
if (!function_exists('icl_unregister_string')) {
	function icl_unregister_string($context, $name) {
		$GLOBALS['pll_wpml_compat']->unregister_string($context, $name);
	}
}

/*
 * gets the translated value of a string (previously registered with icl_register_string or pll_register_string)
 *
 * @since 0.9.3
 *
 * @param string $context not used by Polylang
 * @param string $name not used by Polylang
 * @param string $string the string to translated
 * @return string the translated string in the current language
 */
if (!function_exists('icl_t')) {
	function icl_t($context, $name, $string) {
		return pll__($string);
	}
}

/*
 * undocumented function used by NextGen Gallery
 * seems to be used to both register and translate a string
 * used in PLL_Plugins_Compat for Jetpack with only 3 arguments
 *
 * @since 1.0.2
 *
 * @param string $context the group in which the string is registered, defaults to 'polylang'
 * @param string $name a unique name for the string
 * @param string $string the string to register
 * @param bool $bool optional, not used by Polylang
 * @return string the translated string in the current language
 */
if (!function_exists('icl_translate')) {
	function icl_translate($context, $name, $string, $bool = false) {
		$GLOBALS['pll_wpml_compat']->register_string($context, $name, $string);
		return pll__($string);
	}
}

/*
 * undocumented function used by Types
 * FIXME: tested only with Types
 * probably incomplete as Types locks the custom fields for a new post, but not when edited
 * this is probably linked to the fact that WPML has always an original post in the default language and not Polylang :)
 *
 * @since 1.1.2
 *
 * @return array
 */
if (!function_exists('wpml_get_copied_fields_for_post_edit')) {
	function wpml_get_copied_fields_for_post_edit() {
		if (empty($_GET['from_post']))
			return array();

		// don't know what WPML does but Polylang does copy all public meta keys by default
		foreach ($keys = array_unique(array_keys(get_post_custom($_GET['from_post']))) as $k => $meta_key)
			if (is_protected_meta($meta_key))
				unset ($keys[$k]);

		// apply our filter and fill the expected output (see /types/embedded/includes/fields-post.php)
		$arr['fields'] = array_unique(apply_filters('pll_copy_post_metas', empty($keys) ? array() : $keys, false));
		$arr['original_post_id'] = (int) $_GET['from_post'];
		return $arr;
	}
}

/*
 * undocumented function used by Warp 6 by Yootheme
 *
 * @since 1.0.5
 *
 * @return string default language code
 */
if (!function_exists('icl_get_default_language')) {
	function icl_get_default_language() {
		return pll_default_language();
	}
}

/*
 * registers strings in a persistant way as done by WPML
 *
 * @since 1.0.2
 */
class PLL_WPML_Compat {
	protected $strings; // used for cache

	/*
	 * constructor
	 *
	 * @since 1.0.2
	 */
	public function __construct() {
		add_action('pll_get_strings', array(&$this, 'get_strings'));
	}

	/*
	 * unlike pll_register_string, icl_register_string stores the string in database
	 * so we need to do the same as some plugins or themes may expect this
	 * we use a serialized option to do this
	 *
	 * @since 1.0.2
	 *
	 * @param string $context the group in which the string is registered, defaults to 'polylang'
	 * @param string $name a unique name for the string
	 * @param string $string the string to register
 	 */
	public function register_string($context, $name, $string) {
		if (empty($this->strings))
			$this->strings = get_option('polylang_wpml_strings', array());

		// registers the string if it does not exist yet
		$to_register = array('context' => $context, 'name'=> $name, 'string' => $string, 'multiline' => false, 'icl' => true);
		if (!in_array($to_register, $this->strings) && $to_register['string']) {
			$this->strings[] = $to_register;
			update_option('polylang_wpml_strings', $this->strings);
		}
	}

	/*
	 * removes a string from the registered strings list
	 *
	 * @since 1.0.2
	 *
	 * @param string $context the group in which the string is registered, defaults to 'polylang'
	 * @param string $name a unique name for the string
	 */
	public function unregister_string($context, $name) {
		if (empty($this->strings))
			$this->strings = get_option('polylang_wpml_strings', array());

		foreach ($this->strings as $key=>$string) {
			if ($string['context'] == $context && $string['name'] == $name) {
				unset($this->strings[$key]);
				update_option('polylang_wpml_strings', $this->strings);
			}
		}
	}

	/*
	 * adds strings registered by icl_register_string to those registered by pll_register_string
	 *
	 * @since 1.0.2
	 *
	 * @param array $strings existing registered strings
	 * @return array registered strings with added strings through WPML API
	 */
	public function get_strings($strings) {
		if (empty($this->strings))
			$this->strings = get_option('polylang_wpml_strings');

		return empty($this->strings) ? $strings : array_merge($strings, $this->strings);
	}
} // class PLL_WPML_Compat

$GLOBALS['pll_wpml_compat'] = new PLL_WPML_Compat;

/*
 * reads and interprets the file wpml-config.xml
 * see http://wpml.org/documentation/support/language-configuration-files/
 * the language switcher configuration is not interpreted
 * the xml parser has been adapted from http://php.net/manual/en/function.xml-parse-into-struct.php#84261
 * many thanks to wickedfather at hotmail dot com
 *
 * @since 1.0
 */
class PLL_WPML_Config {
	protected $values, $index, $wpml_config, $strings;

	/*
	 * constructor
	 *
	 * @since 1.0
	 */
	public function __construct() {
		$this->init();
	}

	/*
	 * parses the wpml-config.xml file
	 *
	 * @since 1.0
	 *
	 * @param string wpml-config.xml file content
	 * @param string $context identifies where the file was found
	 */
	protected function xml_parse($xml, $context) {
		$parser = xml_parser_create();
		xml_parser_set_option($parser, XML_OPTION_CASE_FOLDING, 0);
		xml_parser_set_option($parser, XML_OPTION_SKIP_WHITE, 1);
		xml_parse_into_struct($parser, $xml, $this->values);
		xml_parser_free($parser);

		$this->index = 0;
		$arr = $this->xml_parse_recursive();
		$arr = $arr['wpml-config'];

		$keys = array(
			array('custom-fields', 'custom-field'),
			array('custom-types','custom-type'),
			array('taxonomies','taxonomy'),
			array('admin-texts','key')
		);

		foreach ($keys as $k) {
			if (isset($arr[$k[0]])) {
				if (!isset($arr[$k[0]][$k[1]][0])) {
					$elem = $arr[$k[0]][$k[1]];
					unset($arr[$k[0]][$k[1]]);
					$arr[$k[0]][$k[1]][0] = $elem;
				}

				$this->wpml_config[$k[0]][$context] = $arr[$k[0]];
			}
		}
	}

	/*
	 * recursively parses the wpml-config.xml file
	 *
	 * @since 1.0
	 *
	 * @return array
	 */
	protected function xml_parse_recursive() {
		$found = array();
		$tagCount = array();

		while (isset($this->values[$this->index])) {
			extract($this->values[$this->index]);
			$this->index++;

			if ($type == 'close')
				return $found;

			if (isset($tagCount[$tag])) {
				if ($tagCount[$tag] == 1)
					$found[$tag] = array($found[$tag]);

				$tagRef = &$found[$tag][$tagCount[$tag]];
				$tagCount[$tag]++;
			}
			else {
				$tagCount[$tag] = 1;
				$tagRef = &$found[$tag];
			}

			if ($type == 'open') {
				$tagRef = $this->xml_parse_recursive();
				if (isset($attributes))
					$tagRef['attributes'] = $attributes;
			}

			if ($type == 'complete') {
				if (isset($attributes)) {
					$tagRef['attributes'] = $attributes;
					$tagRef = &$tagRef['value'];
				}
				if (isset($value))
					$tagRef = $value;
			}
		}

		return $found;
	}

	/*
	 * finds the wpml-config.xml files to parse and setup filters
	 *
	 * @since 1.0
	 */
	public function init() {
		$this->wpml_config = array();

		// child theme
		if (($template = get_template_directory()) != ($stylesheet = get_stylesheet_directory()) && file_exists($file = $stylesheet.'/wpml-config.xml'))
			$this->xml_parse(file_get_contents($file), get_stylesheet()); // FIXME fopen + fread + fclose quicker ?

		// theme
		if (file_exists($file = $template.'/wpml-config.xml'))
 			$this->xml_parse(file_get_contents($file), get_template());

		// plugins
		// don't forget sitewide active plugins thanks to Reactorshop http://wordpress.org/support/topic/polylang-and-yoast-seo-plugin/page/2?replies=38#post-4801829
		$plugins = (is_multisite() && $sitewide_plugins = get_site_option('active_sitewide_plugins')) && is_array($sitewide_plugins) ? array_keys($sitewide_plugins) : array();
		$plugins = array_merge($plugins, get_option('active_plugins'));

		foreach ($plugins as $plugin) {
			if (file_exists($file = dirname(POLYLANG_DIR).'/'.dirname($plugin).'/wpml-config.xml'))
				$this->xml_parse(file_get_contents($file), dirname($plugin));
		}

		// custom
		if (file_exists($file = PLL_LOCAL_DIR.'/wpml-config.xml'))
 			$this->xml_parse(file_get_contents($file), 'polylang');

		if (isset($this->wpml_config['custom-fields']))
			add_filter('pll_copy_post_metas', array(&$this, 'copy_post_metas'), 10, 2);

		if (isset($this->wpml_config['custom-types']))
			add_filter('pll_get_post_types', array(&$this, 'translate_types'), 10, 2);

		if (isset($this->wpml_config['taxonomies']))
			add_filter('pll_get_taxonomies', array(&$this, 'translate_taxonomies'), 10, 2);

		if (!isset($this->wpml_config['admin-texts']))
			return;

		// get a cleaner array for easy manipulation
		foreach ($this->wpml_config['admin-texts'] as $context => $arr)
			foreach ($arr as $keys)
				$this->strings[$context] = $this->admin_texts_recursive($keys);

		foreach ($this->strings as $context => $options) {
			foreach ($options as $option_name => $value) {
				if (PLL_ADMIN) { // backend
					$option = get_option($option_name);
					if (is_string($option) && $value == 1)
						pll_register_string($option_name, $option, $context);
					elseif (is_array($option) && is_array($value))
						$this->register_string_recursive($context, $value, $option); // for a serialized option
				}
				else
					add_filter('option_'.$option_name, array(&$this, 'translate_strings'));
			}
		}
	}

	/*
	 * arranges strings in a cleaner way
	 *
	 * @since 1.0
	 *
	 * @param array $keys
	 * @return array
	 */
	protected function admin_texts_recursive($keys) {
		if (!isset($keys[0])) {
			$elem = $keys;
			unset($keys);
			$keys[0] = $elem;
		}
		foreach ($keys as $key)
			$strings[$key['attributes']['name']] = isset($key['key']) ? $this->admin_texts_recursive($key['key']) : 1;

		return $strings;
	}

	/*
	 * recursively registers strings for a serialized option
	 *
	 * @since 1.0
	 *
	 * @param string $context the group in which the strings will be registered
	 * @param array $strings
	 * @param array $options
	 */
	protected function register_string_recursive($context, $strings, $options) {
		foreach ($options as $name => $value) {
			if (isset($strings[$name])) {
				// allow numeric values to be translated
				// https://wordpress.org/support/topic/wpml-configxml-strings-skipped-when-numbers-ids
				if ((is_numeric($value) || is_string($value)) && $strings[$name] == 1)
					pll_register_string($name, $value, $context);
				elseif (is_array($value) && is_array($strings[$name]))
					$this->register_string_recursive($context, $strings[$name], $value);
			}
		}
	}

	/*
	 * adds custom fields to the list of metas to copy when creating a new translation
	 *
	 * @since 1.0
	 *
	 * @param array $metas the list of custom fields to copy or synchronize
	 * @param bool $sync true for sync, false for copy
	 * @return array the list of custom fields to copy or synchronize
	 */
	public function copy_post_metas($metas, $sync) {
		foreach ($this->wpml_config['custom-fields'] as $context) {
			foreach ($context['custom-field'] as $cf) {
				// copy => copy and synchronize
				// translate => copy but don't synchronize
				// ignore => don't copy
				// see http://wordpress.org/support/topic/custom-field-values-override-other-translation-values?replies=8#post-4655563
				if ('copy' == $cf['attributes']['action'] || (!$sync && 'translate' == $cf['attributes']['action']))
					$metas[] = $cf['value'];
				else
					$metas = array_diff($metas,  array($cf['value']));
			}
		}
		return $metas;
	}

	/*
	 * language and translation management for custom post types
	 *
	 * @since 1.0
	 *
	 * @param array $types list of post type names for which Polylang manages language and translations
	 * @param bool $hide true when displaying the list in Polylang settings
	 * @return array list of post type names for which Polylang manages language and translations
	 */
	public function translate_types($types, $hide) {
		foreach ($this->wpml_config['custom-types'] as $context) {
			foreach ($context['custom-type'] as $pt) {
				if ($pt['attributes']['translate'] == 1 && !$hide)
					$types[$pt['value']] = $pt['value'];
				elseif ($hide)
					unset ($types[$pt['value']]); // the author decided what to do with the post type so don't allow the user to change this
			}
		}
		return $types;
	}

	/*
	 * language and translation management for custom taxonomies
	 *
	 * @since 1.0
	 *
	 * @param array $taxonomies list of taxonomy names for which Polylang manages language and translations
	 * @param bool $hide true when displaying the list in Polylang settings
	 * @return array list of taxonomy names for which Polylang manages language and translations
	 */
	public function translate_taxonomies($taxonomies, $hide) {
		foreach ($this->wpml_config['taxonomies'] as $context) {
			foreach ($context['taxonomy'] as $tax) {
				if ($tax['attributes']['translate'] == 1 && !$hide)
					$taxonomies[$tax['value']] = $tax['value'];
				elseif ($hide)
					unset ($taxonomies[$tax['value']]); // the author decided what to do with the taxonomy so don't allow the user to change this
			}
		}

		return $taxonomies;
	}

	/*
	 * translates the strings for an option
	 *
	 * @since 1.0
	 *
	 * @param array|string either a string to translate or a list of strings to translate
	 * @return array|string translated string(s)
	 */
	public function translate_strings($value) {
		if (is_array($value)) {
			$option = substr(current_filter(), 7);
			foreach ($this->strings as $context => $options) {
				if (array_key_exists($option, $options))
					return $this->translate_strings_recursive($options[$option], $value); // for a serialized option
			}
		}
		return pll__($value);
	}

	/*
	 * recursively translates strings for a serialized option
	 *
	 * @since 1.0
	 *
	 * @param array $strings
	 * @param array|string $values either a string to translate or a list of strings to translate
	 * @return array|string translated string(s)
	 */
	protected function translate_strings_recursive($strings, $values) {
		foreach ($values as $name => $value) {
			if (isset($strings[$name])) {
				// allow numeric values to be translated
				// https://wordpress.org/support/topic/wpml-configxml-strings-skipped-when-numbers-ids
				if ((is_numeric($value) || is_string($value)) && $strings[$name] == 1)
					$values[$name] = pll__($value);
				elseif (is_array($value) && is_array($strings[$name]))
					$values[$name] = $this->translate_strings_recursive($strings[$name], $value);
			}
		}
		return $values;
	}
} // class PLL_WPML_Config
