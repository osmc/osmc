<?php

/*
 * template tag: displays the language switcher
 *
 * list of parameters accepted in $args:
 *
 * dropdown               => displays a dropdown if set to 1, defaults to 0
 * echo                   => echoes the the switcher if set to 1 (default)
 * hide_if_empty          => hides languages with no posts (or pages) if set to 1 (default)
 * show_flags             => shows flags if set to 1, defaults to 0
 * show_names             => shows languages names if set to 1 (default)
 * display_names_as       => wether to display the language name or code. valid options are 'slug' and 'name'
 * force_home             => forces linking to the home page is set to 1, defaults to 0
 * hide_if_no_translation => hides the link if there is no translation if set to 1, defaults to 0
 * hide_current           => hides the current language if set to 1, defaults to 0
 * post_id                => if not null, link to translations of post defined by post_id, defaults to null
 * raw                    => set this to true to build your own custom language switcher, defaults to 0
 *
 * @since 0.5
 *
 * @param array $args optional
 * @return null|string|array null if displaying, array if raw is requested, string otherwise
 */
function pll_the_languages($args = '') {
	global $polylang;
	if ($polylang instanceof PLL_Frontend && !empty($polylang->links)) {
		$switcher = new PLL_Switcher;
		return $switcher->the_languages($polylang->links, $args);
	}
	return '';
}

/*
 * returns the current language
 *
 * @since 0.8.1
 *
 * @param string $field optional the language field to return 'name', 'locale', defaults to 'slug'
 * @return string the requested field for the current language
 */
function pll_current_language($field = 'slug') {
	global $polylang;
	return isset($polylang->curlang->$field) ? $polylang->curlang->$field : false;
}

/*
 * returns the default language
 *
 * @since 1.0
 *
 * @param string $field optional the language field to return 'name', 'locale', defaults to 'slug'
 * @return string the requested field for the default language
 */
function pll_default_language($field = 'slug') {
	global $polylang;
	return isset($polylang->options['default_lang']) && ($lang = $polylang->model->get_language($polylang->options['default_lang'])) && isset($lang->$field) ? $lang->$field : false;
}

/*
 * among the post and its translations, returns the id of the post which is in the language represented by $slug
 *
 * @since 0.5
 *
 * @param int $post_id post id
 * @param string $slug optional language code, defaults to current language
 * @return int post id of the translation if exists
 */
function pll_get_post($post_id, $slug = '') {
	global $polylang;
	return isset($polylang) && ($slug = $slug ? $slug : pll_current_language()) ? $polylang->model->get_post($post_id, $slug) : null;
}

/*
 * among the term and its translations, returns the id of the term which is in the language represented by $slug
 *
 * @since 0.5
 *
 * @param int $term_id term id
 * @param string $slug optional language code, defaults to current language
 * @return int term id of the translation if exists
 */
function pll_get_term($term_id, $slug = '') {
	global $polylang;
	return isset($polylang) && ($slug = $slug ? $slug : pll_current_language()) ? $polylang->model->get_term($term_id, $slug) : null;
}

/*
 * returns the home url in the current language
 *
 * @since 0.8
 *
 * @param string $lang language code (optional on frontend)
 * @return string
 */
function pll_home_url($lang = '') {
	global $polylang;

	if (empty($lang))
		$lang = pll_current_language();

	return isset($polylang) && !empty($polylang->links) && !empty($lang) ? $polylang->links->get_home_url($lang) : home_url('/');
}

/*
 * registers a string for translation in the "strings translation" panel
 *
 * @since 0.6
 *
 * @param string $name a unique name for the string
 * @param string $string the string to register
 * @param string $context optional the group in which the string is registered, defaults to 'polylang'
 * @param bool $multiline optional wether the string table should display a multiline textarea or a single line input, defaults to single line
 */
function pll_register_string($name, $string, $context = 'polylang', $multiline = false) {
	global $polylang;
	if ($polylang instanceof PLL_Admin && !empty($polylang->settings_page))
		$polylang->settings_page->register_string($name, $string, $context, $multiline);
}

/*
 * translates a string (previously registered with pll_register_string)
 *
 * @since 0.6
 *
 * @param string $string the string to translate
 * @return string the string translation in the current language
 */
function pll__($string) {
	return __($string, 'pll_string');
}

/*
 * echoes a translated string (previously registered with pll_register_string)
 *
 * @since 0.6
 *
 * @param string $string the string to translate
 */
function pll_e($string) {
	_e($string, 'pll_string');
}

/*
 * translates a string (previously registered with pll_register_string)
 *
 * @since 1.5.4
 *
 * @param string $string the string to translate
 * @param string $lang language code
 * @return string the string translation in the requested language
 */
function pll_translate_string($string, $lang) {
	if (pll_current_language() == $lang)
		return pll__($string);
		
	static $mo = array();
	
	if (empty($mo[$lang])) {
		$mo[$lang] = new PLL_MO();
		$mo[$lang]->import_from_db($GLOBALS['polylang']->model->get_language($lang));	
	}
	
	return $mo[$lang]->translate($string);
}

/*
 * returns true if Polylang manages languages and translations for this post type
 *
 * @since 1.0.1
 *
 * @param string post type name
 * @return bool
 */
function pll_is_translated_post_type($post_type) {
	global $polylang;
	return isset($polylang) && $polylang->model->is_translated_post_type($post_type);
}

/*
 * returns true if Polylang manages languages and translations for this taxonomy
 *
 * @since 1.0.1
 *
 * @param string taxonomy name
 * @return bool
 */
function pll_is_translated_taxonomy($tax) {
	global $polylang;
	return isset($polylang) && $polylang->model->is_translated_taxonomy($tax);
}

/*
 * returns the list of available languages
 *
 * list of parameters accepted in $args:
 *
 * hide_empty => hides languages with no posts if set to true (defaults to false)
 * fields     => return only that field if set (see PLL_Language for a list of fields)
 *
 * @since 1.5
 *
 * @param array $args list of parameters
 * @return array
 */
function pll_languages_list($args = array()) {
	global $polylang;
	$args = wp_parse_args($args, array('fields' => 'slug'));
	return isset($polylang) ? $polylang->model->get_languages_list($args) : false;
}

/*
 * set the post language
 *
 * @since 1.5
 *
 * @param int $post_id post id
 * @param string $lang language code
 */
function pll_set_post_language($id, $lang) {
	global $polylang;
	if (isset($polylang))
		$polylang->model->set_post_language($id, $lang);
}

/*
 * set the term language
 *
 * @since 1.5
 *
 * @param int $id term id
 * @param string $lang language code
 */
function pll_set_term_language($id, $lang) {
	global $polylang;
	if (isset($polylang))
		$polylang->model->set_term_language($id, $lang);
}

/*
 * save posts translations
 *
 * @since 1.5
 *
 * @param array $arr an associative array of translations with language code as key and post id as value
 */
function pll_save_post_translations($arr) {
	global $polylang;
	if (isset($polylang))
		$polylang->model->save_translations('post', reset($arr), $arr);
}

/*
 * save terms translations
 *
 * @since 1.5
 *
 * @param array $arr an associative array of translations with language code as key and term id as value
 */
function pll_save_term_translations($arr) {
	global $polylang;
	if (isset($polylang))
		$polylang->model->save_translations('term', reset($arr), $arr);
}

/*
 * returns the post language
 *
 * @since 1.5.4
 *
 * @param int $post_id
 * @param string $field optional the language field to return 'name', 'locale', defaults to 'slug'
 * @return bool|string the requested field for the post language, false if no language is associated to that post
 */
function pll_get_post_language($post_id, $field = 'slug') {
	global $polylang;
	return isset($polylang) && ($lang = $polylang->model->get_post_language($post_id)) ? $lang->$field : false;
}

/*
 * returns the term language
 *
 * @since 1.5.4
 *
 * @param int $term_id
 * @param string $field optional the language field to return 'name', 'locale', defaults to 'slug'
 * @return bool|string the requested field for the term language, false if no language is associated to that term
 */
function pll_get_term_language($term_id, $field = 'slug') {
	global $polylang;
	return isset($polylang) && ($lang = $polylang->model->get_term_language($term_id)) ? $lang->$field : false;
}

/*
 * count posts in a language
 *
 * @since 1.5
 *
 * @param string $lang language code
 * @param array $args (accepted keys: post_type, m, year, monthnum, day, author, author_name, post_format)
 * @return int posts count
 */
function pll_count_posts($lang, $args = array()) {
	global $polylang;
	return isset($polylang) ? $polylang->model->count_posts($polylang->model->get_language($lang), $args) : false;
}
