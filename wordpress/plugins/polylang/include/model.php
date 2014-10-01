<?php

/*
 * setups the language and translations model based on WordPress taxonomies
 *
 * @since 1.2
 */
class PLL_Model {
	public $options;
	public $languages; // used to cache the list of languages
	public $blog_id; // used to get one cache array per site

	/*
	 * constructor: registers custom taxonomies and setups filters and actions
	 *
	 * @since 1.2
	 *
	 * @param array $options Polylang options
	 */
	public function __construct(&$options) {
		$this->options = &$options;
		$this->blog_id = get_current_blog_id();

		// register our taxonomies as soon as possible
		// this is early registration, not ready for rewrite rules as wp_rewrite will be setup later
		$args = array('label' => false, 'public' => false, 'query_var' => false, 'rewrite' => false, '_pll' => true);
		register_taxonomy('language', null, $args);
		register_taxonomy('term_language', 'term', $args);
		register_taxonomy('term_translations', 'term', $args);
		$args['update_count_callback'] = '_update_generic_term_count'; // count *all* posts to avoid deleting in clean_translations_terms
		register_taxonomy('post_translations', null, $args);

		add_filter('get_terms', array(&$this, '_prime_terms_cache'), 10, 2);
		add_filter('wp_get_object_terms', array(&$this, 'wp_get_object_terms'), 10, 3);

		// we need to clean languages cache when editing a language,
		// when editing page of front or when modifying the permalink structure
		add_action('edited_term_taxonomy', array(&$this, 'clean_languages_cache'), 10, 2);
		add_action('update_option_page_on_front', array(&$this, 'clean_languages_cache'));
		add_action('update_option_permalink_structure', array(&$this, 'clean_languages_cache'));

		// registers completely the language taxonomy
		add_action('setup_theme', array(&$this, 'register_taxonomy'), 1);

		// setups post types to translate
		add_action('registered_post_type', array(&$this, 'registered_post_type'));

		// just in case someone would like to display the language description ;-)
		add_filter('language_description', create_function('$v', "return '';"));
	}

	/*
	 * cache language and translations when terms are queried by get_terms
	 *
	 * @since 1.2
	 *
	 * @param array $terms queried terms
	 * @param array $taxonomies queried taxonomies
	 * @return array unmodified $terms
	 */
	public function _prime_terms_cache($terms, $taxonomies) {
		if ($this->is_translated_taxonomy($taxonomies)) {
			foreach ($terms as $term) {
				$term_ids[] = is_object($term) ? $term->term_id : $term;
			}
		}

		if (!empty($term_ids))
			update_object_term_cache(array_unique($term_ids), 'term'); // adds language and translation of terms to cache
		return $terms;
	}

	/*
	 * when terms are found for posts, add their language and translations to cache
	 *
	 * @since 1.2
	 *
	 * @param array $terms terms found
	 * @param array $object_ids not used
	 * @param array $taxonomies terms taxonomies
	 * @return array unmodified $terms
	 */
	// FIXME is that useful? or even desirable?
	public function wp_get_object_terms($terms, $object_ids, $taxonomies) {
		$taxonomies = explode("', '", trim($taxonomies, "'"));
		if (!in_array('term_translations', $taxonomies))
			$this->_prime_terms_cache($terms, $taxonomies);
		return $terms;
	}

	/*
	 * wrap wp_get_object_terms to cache it and return only one object
	 * inspired by the function get_the_terms
	 *
	 * @since 1.2
	 *
	 * @param int $object_id post_id or term_id
	 * @param string $taxonomy Polylang taxonomy depending if we are looking for a post (or term) language (or translation)
	 * @return bool|object the term associated to the object in the requested taxonomy if exists, false otherwise
	 */
	public function get_object_term($object_id, $taxonomy) {
		$term = get_object_term_cache($object_id, $taxonomy);

		if ( false === $term ) {
			// query language and translations at the same time
			$taxonomies = (false !== strpos($taxonomy, 'term_')) ?
				array('term_language', 'term_translations') :
				array('language', 'post_translations');

			foreach (wp_get_object_terms($object_id, $taxonomies) as $t) {
				wp_cache_add($object_id, array($t), $t->taxonomy . '_relationships'); // store it the way WP wants it
				if ($t->taxonomy == $taxonomy)
					$term = $t;
			}
		}
		else {
			$term = reset($term);
		}

		return empty($term) ? false : $term;
	}

	/*
	 * returns the list of available languages
	 * caches the list in a db transient (except flags), unless PLL_CACHE_LANGUAGES is set to false
	 * caches the list (with flags) in the private property $languages
	 *
	 * list of parameters accepted in $args:
	 *
	 * hide_empty => hides languages with no posts if set to true (defaults to false)
	 * fields     => return only that field if set (see PLL_Language for a list of fields)
	 *
	 * @since 0.1
	 *
	 * @param array $args
	 * @return array|string|int list of PLL_Language objects or PLL_Language object properties
	 */
	public function get_languages_list($args = array()) {
		if (empty($this->languages[$this->blog_id])) {
			if ((defined('PLL_CACHE_LANGUAGES') && !PLL_CACHE_LANGUAGES) || false === ($languages = get_transient('pll_languages_list'))) {
				$languages = get_terms('language', array('hide_empty' => false, 'orderby'=> 'term_group'));
				$languages = empty($languages) || is_wp_error($languages) ? array() : $languages;

				$term_languages = get_terms('term_language', array('hide_empty' => false));
				$term_languages = empty($term_languages) || is_wp_error($term_languages) ?
					array() : array_combine(wp_list_pluck($term_languages, 'name'), $term_languages);

				if (!empty($languages) && !empty($term_languages)) {
					// don't use array_map + create_function to instantiate an autoloaded class as it breaks badly in old versions of PHP
					foreach ($languages as $k => $v) {
						$languages[$k] = new PLL_Language($v, $term_languages[$v->name]);
					}
				}
				else {
					$languages = array(); // in case something went wrong
				}
			}

			$this->languages[$this->blog_id] = $languages;

			// need to wait for $wp_rewrite availibility to set homepage urls
			did_action('setup_theme') ? $this->_languages_list() : add_action('setup_theme', array(&$this, '_languages_list'));
		}

		$args = wp_parse_args($args, array('hide_empty' => false));

		// remove empty languages if requested
		$languages = array_filter($this->languages[$this->blog_id], create_function('$v', sprintf('return $v->count || !%d;', $args['hide_empty'])));

		return empty($args['fields']) ? $languages : wp_list_pluck($languages, $args['fields']);
	}

	/*
	 * fills home urls and flags in language list and set transient in db
	 * delayed to be sure we have access to $wp_rewrite for home urls
	 * languages objects are not cached in db if PLL_CACHE_LANGUAGES is set to false
	 * home urls are not cached in db if PLL_CACHE_HOME_URL is set to false
	 *
	 * @since 1.4
	 */
	public function _languages_list() {
		if ((defined('PLL_CACHE_LANGUAGES') && !PLL_CACHE_LANGUAGES) || false === get_transient('pll_languages_list')) {
			if (!defined('PLL_CACHE_HOME_URL') || PLL_CACHE_HOME_URL) {
				foreach ($this->languages[$this->blog_id] as $language)
					$language->set_home_url();
			}

			if (!defined('PLL_CACHE_LANGUAGES') || PLL_CACHE_LANGUAGES)
				set_transient('pll_languages_list', $this->languages[$this->blog_id]);
		}

		foreach ($this->languages[$this->blog_id] as $language) {
			if (defined('PLL_CACHE_HOME_URL') && !PLL_CACHE_HOME_URL)
				$language->set_home_url();

			// add flags (not in db cache as they may be different on frontend and admin)
			$language->set_flag();
		}
	}

	/*
	 * cleans language cache
	 * can be called directly with no parameter
	 * called by the 'edited_term_taxonomy' filter with 2 parameters when count needs to be updated
	 *
	 * @since 1.2
	 *
	 * @param int $term not used
	 * @param string $taxonomy taxonomy name
	 */
	public function clean_languages_cache($term = 0, $taxonomy = null) {
		if (empty($taxonomy->name) || 'language' == $taxonomy->name) {
			delete_transient('pll_languages_list');
			$this->languages[$this->blog_id] = array();
		}
	}

	/*
	 * returns the language by its term_id, tl_term_id, slug or locale
	 *
	 * @since 0.1
	 *
	 * @param int|string term_id, tl_term_id, slug or locale of the queried language
	 * @return object|bool PLL_Language object, false if no language found
	 */
	public function get_language($value) {
		static $language;

		if (is_object($value))
			return $this->get_language($value->term_id); // will force cast to PLL_Language

		if (empty($language[$this->blog_id][$value])) {
			foreach ($this->get_languages_list() as $lang)
				$language[$this->blog_id][$lang->term_id]
					= $language[$this->blog_id][$lang->tl_term_id]
					= $language[$this->blog_id][$lang->slug]
					= $language[$this->blog_id][$lang->locale] = $lang;
		}

		return empty($language[$this->blog_id][$value]) ? false : $language[$this->blog_id][$value];
	}

	/*
	 * removes unused translations terms in database
	 *
	 * @since 1.5
	 *
	 * @param string $type either 'post' or 'term'
	 */
	protected function clean_translations_terms($type) {
		// FIXME does nothing since 1.5.2 as count seems not to be reliable enough
		/*
		global $wpdb;
		$ids = $wpdb->get_col("SELECT term_id FROM $wpdb->term_taxonomy WHERE taxonomy IN ('{$type}_translations') AND count = 0");
		foreach ($ids as $id)
			wp_delete_term((int) $id, $type . '_translations');
		*/
	}

	/*
	 * saves translations for posts or terms
	 *
	 * @since 0.5
	 *
	 * @param string $type either 'post' or 'term'
	 * @param int $id post id or term id
	 * @param array $translations: an associative array of translations with language code as key and translation id as value
	 */
	public function save_translations($type, $id, $translations) {
		if (($lang = call_user_func(array(&$this, 'get_'.$type.'_language'), $id)) && isset($translations) && is_array($translations)) {

			$translations = array_merge(array($lang->slug => $id), $translations); // make sure this object is in translations
			$translations = array_diff($translations, array(0)); // don't keep non translated languages

			// unlink removed translations
			$old_translations = $this->get_translations($type, $id);
			foreach (array_diff_assoc($old_translations, $translations) as $object_id)
				$this->delete_translation($type, $object_id);

			// don't create a translation group for untranslated posts as it is useless
			// but we need one for terms to allow relationships remap when importing from a WXR file
			if ('term' == $type || count($translations) > 1) {
				$terms = wp_get_object_terms($translations, $type . '_translations');
				$term = reset($terms);

				// create a new term if necessary
				if (empty($term)) {
					wp_insert_term($group = uniqid('pll_'), $type . '_translations', array('description' => serialize($translations)));
				}
				else {
					// take care not to overwrite extra data stored in description field, if any
					$d = unserialize($term->description);
					$d = is_array($d) ? array_diff_key($d, $old_translations) : array(); // remove old translations
					$d = array_merge($d, $translations); // add new one
					wp_update_term($group = (int) $term->term_id, $type . '_translations', array('description' => serialize($d)));
				}

				// link all translations to the new term
				foreach($translations as $p)
					wp_set_object_terms($p, $group, $type . '_translations');

				// clean unused terms to avoid orphans in db
				$this->clean_translations_terms($type);
			}
		}
	}

	/*
	 * deletes a translation of a post or term
	 *
	 * @since 0.5
	 *
	 * @param string $type either 'post' or 'term'
	 * @param int $id post id or term id
	 */
	public function delete_translation($type, $id) {
		$term = $this->get_object_term($id, $type . '_translations');

		if (!empty($term)) {
			$d = unserialize($term->description);
			$slug = array_search($id, $this->get_translations($type, $id)); // in case some plugin stores the same value with different key
			unset($d[$slug]);

			wp_update_term((int) $term->term_id, $type . '_translations', array('description' => serialize($d)));

			if ('post' == $type)
				wp_set_object_terms($id, null, $type . '_translations');
			else {
				// always keep a group for terms to allow relationships remap when importing from a WXR file
				$translations[$slug] = $id;
				wp_insert_term($group = uniqid('pll_'), $type . '_translations', array('description' => serialize($translations)));
				wp_set_object_terms($id, $group, $type . '_translations');
			}

			// clean unused terms to avoid orphans in db
			$this->clean_translations_terms($type);
		}
	}

	/*
	 * returns the id of the translation of a post or term
	 *
	 * @since 0.5
	 *
	 * @param string $type either 'post' or 'term'
	 * @param int $id post id or term id
	 * @param object|string $lang object or slug
	 * @return bool|int post id or term id of the translation, flase if there is none
	 */
	public function get_translation($type, $id, $lang) {
		if (!$lang = $this->get_language($lang))
			return false;

		$translations = $this->get_translations($type, $id);

		return isset($translations[$lang->slug]) ? (int) $translations[$lang->slug] : false;
	}

	/*
	 * returns an array of translations of a post or term
	 *
	 * @since 0.5
	 *
	 * @param string $type either 'post' or 'term'
	 * @param int $id post id or term id
	 * @return array an associative array of translations with language code as key and translation id as value
	 */
	public function get_translations($type, $id) {
		$type = ($type == 'post' || $this->is_translated_post_type($type)) ? 'post' : (($type == 'term' || $this->is_translated_taxonomy($type)) ? 'term' : false);
		$translations = $type && ($term = $this->get_object_term($id, $type . '_translations')) && !empty($term) ? unserialize($term->description) : array();

		// make sure we return only translations (thus we allow plugins to store other informations in the array)
		return array_intersect_key($translations, array_flip($this->get_languages_list(array('fields' => 'slug'))));
	}

	/*
	 * store the post language in the database
	 *
	 * @since 0.6
	 *
	 * @param int $post_id post id
	 * @param int|string|object language (term_id or slug or object)
	 */
	public function set_post_language($post_id, $lang) {
		wp_set_post_terms($post_id, $lang ? $this->get_language($lang)->slug : '', 'language' );
	}

	/*
	 * returns the language of a post
	 *
	 * @since 0.1
	 *
	 * @param int $post_id post id
	 * @return bool|object PLL_Language object, false if no language is associated to that post
	 */
	public function get_post_language($post_id) {
		$lang = $this->get_object_term($post_id, 'language' );
		return ($lang) ? $this->get_language($lang) : false;
	}

	/*
	 * among the post and its translations, returns the id of the post which is in $lang
	 *
	 * @since 0.1
	 *
	 * @param int $post_id post id
	 * @param int|string|object language (term_id or slug or object)
	 * @return bool|int the translation post id if exists, otherwise the post id, false if the post has no language
	 */
	public function get_post($post_id, $lang) {
		$post_lang = $this->get_post_language($post_id); // FIXME is this necessary?
		if (!$lang || !$post_lang)
			return false;

		$lang = $this->get_language($lang);
		return $post_lang->term_id == $lang->term_id ? $post_id : $this->get_translation('post', $post_id, $lang);
	}

	/*
	 * stores the term language in the database
	 *
	 * @since 0.6
	 *
	 * @param int $term_id term id
	 * @param int|string|object language (term_id or slug or object)
	 */
	public function set_term_language($term_id, $lang) {
		wp_set_object_terms($term_id, $lang ? $this->get_language($lang)->tl_term_id : '', 'term_language');
	}

	/*
	 * removes the term language in database
	 *
	 * @since 0.5
	 *
	 * @param int $term_id term id
	 */
	public function delete_term_language($term_id) {
		wp_delete_object_term_relationships($term_id, 'term_language');
	}

	/*
	 * returns the language of a term
	 *
	 * @since 0.1
	 *
	 * @param int|string $value term id or term slug
	 * @param string $taxonomy optional taxonomy needed when the term slug is passed as first parameter
	 * @return bool|object PLL_Language object, false if no language is associated to that term
	 */
	public function get_term_language($value, $taxonomy = '') {
		if (is_numeric($value))
			$term_id = $value;

		// get_term_by still not cached in WP 3.5.1 but internally, the function is always called by term_id
		elseif (is_string($value) && $taxonomy)
			$term_id = get_term_by('slug', $value , $taxonomy)->term_id;

		$lang = $this->get_object_term($term_id, 'term_language');

		// switch to PLL_Language
		return ($lang) ? $this->get_language($lang->term_id) : false;
	}

	/*
	 * among the term and its translations, returns the id of the term which is in $lang
	 *
	 * @since 0.1
	 *
	 * @param int $term_id term id
	 * @param int|string|object language (term_id or slug or object)
	 * @return bool|int the translation term id if exists, otherwise the term id, false if the term has no language
	 */
	public function get_term($term_id, $lang) {
		$lg = $this->get_term_language($term_id); // FIXME is this necessary?
		if (!$lang || !$lg)
			return false;

		$lang = $this->get_language($lang);
		return $lg->term_id == $lang->term_id ? $term_id : $this->get_translation('term', $term_id, $lang);
	}

	/*
	 * a join clause to add to sql queries when filtering by language is needed directly in query
	 *
	 * @since 1.2
	 *
	 * @param string $type either 'post' or 'term'
	 * @return string join clause
	 */
	public function join_clause($type) {
		global $wpdb;
		return " INNER JOIN $wpdb->term_relationships AS pll_tr ON pll_tr.object_id = " . ('term' == $type ? "t.term_id" : "ID");
	}

	/*
	 * a where clause to add to sql queries when filtering by language is needed directly in query
	 *
	 * @since 1.2
	 *
	 * @param object|array|string $lang a PLL_Language object or a comma separated list of languag slug or an array of language slugs
	 * @param string $type either 'post' or 'term'
	 * @return string where clause
	 */
	public function where_clause($lang, $type) {
		global $wpdb;
		$tt_id = 'term' == $type ? 'tl_term_taxonomy_id' : 'term_taxonomy_id';

		// $lang is an object
		// generally the case if the query is coming from Polylang
		if (is_object($lang))
			return $wpdb->prepare(" AND pll_tr.term_taxonomy_id = %d", $lang->$tt_id);

		// $lang is a comma separated list of slugs (or an array of slugs)
		// generally the case is the query is coming from outside with 'lang' parameter
		$slugs = is_array($lang) ? $lang : explode(',', $lang);
		foreach ($slugs as $slug)
			$languages[] = (int) $this->get_language($slug)->$tt_id;

		return " AND pll_tr.term_taxonomy_id IN (" . implode(',', $languages) . ")";
	}

	/*
	 * adds terms clauses to get_terms to filter them by languages - used in both frontend and admin
	 *
	 * @since 1.2
	 *
	 * @param array $clauses the list of sql clauses in terms query
	 * @param object $lang PLL_Language object
	 * @return array modifed list of clauses
	 */
	public function terms_clauses($clauses, $lang) {
		if (!empty($lang)) {
			$clauses['join'] .= $this->join_clause('term');
			$clauses['where'] .= $this->where_clause($lang, 'term');
		}
		return $clauses;
	}

	/*
	 * register the language taxonomy
	 *
	 * @since 1.2
	 */
	public function register_taxonomy() {
		// registers the language taxonomy
		register_taxonomy('language', $this->get_translated_post_types(), array(
			'labels' => array(
				'name' => __('Languages', 'polylang'),
				'singular_name' => __('Language', 'polylang'),
				'all_items' => __('All languages', 'polylang'),
			),
			'public' => false, // avoid displaying the 'like post tags text box' in the quick edit
			'query_var' => 'lang',
			'_pll' => true // polylang taxonomy
		));
	}

	/*
	 * returns post types that need to be translated
	 *
	 * @since 1.2
	 *
	 * @param bool $filter true if we should return only valid registered post types
	 * @return array post type names for which Polylang manages languages and translations
	 */
	public function get_translated_post_types($filter = true) {
		static $post_types = array();

		// the post types list is cached for better better performance
		// wait for 'after_setup_theme' to apply the cache to allow themes adding the filter in functions.php
		if (!$post_types || !did_action('after_setup_theme')) {
			$post_types = array('post' => 'post', 'page' => 'page');

			if (!empty($this->options['media_support']))
				$post_types['attachement'] = 'attachment';

			if (is_array($this->options['post_types']))
				$post_types = array_merge($post_types,  $this->options['post_types']);

			$post_types = apply_filters('pll_get_post_types', $post_types , false);
		}

		return $filter ? array_intersect($post_types, get_post_types()) : $post_types;
	}

	/*
	 * check if registered post type must be translated
	 *
	 * @since 1.2
	 *
	 * @param string $post_type post type name
	 */
	public function registered_post_type($post_type) {
		if ($this->is_translated_post_type($post_type)) {
			register_taxonomy_for_object_type('language', $post_type);
			register_taxonomy_for_object_type('post_translations', $post_type);
		}
	}

	/*
	 * returns true if Polylang manages languages and translations for this post type
	 *
	 * @since 1.2
	 *
	 * @param string|array $post_type post type name or array of post type names
	 */
	public function is_translated_post_type($post_type) {
		$post_types = $this->get_translated_post_types(false);
		return (is_array($post_type) && array_intersect($post_type, $post_types) || in_array($post_type, $post_types));
	}

	/*
	 * return taxonomies that need to be translated
	 *
	 * @since 1.2
	 *
	 * @param bool $filter true if we should return only valid registered taxonmies
	 * @return array array of registered taxonomy names for which Polylang manages languages and translations
	 */
	public function get_translated_taxonomies($filter = true) {
		static $taxonomies = array();

		if (!$taxonomies || !did_action('after_setup_theme')) {
			$taxonomies = array('category' => 'category', 'post_tag' => 'post_tag');

			if (is_array($this->options['taxonomies']))
				$taxonomies = array_merge($taxonomies, $this->options['taxonomies']);

			$taxonomies = apply_filters('pll_get_taxonomies', $taxonomies, false);
		}

		return $filter ? array_intersect($taxonomies, get_taxonomies()) : $taxonomies;
	}

	/*
	 * returns true if Polylang manages languages and translations for this post type
	 *
	 * @since 1.2
	 *
	 * @param string|array $tax taxonomy name or array of taxonomy names
	 */
	public function is_translated_taxonomy($tax) {
		$taxonomies = $this->get_translated_taxonomies(false);
		return (is_array($tax) && array_intersect($tax, $taxonomies) || in_array($tax, $taxonomies));
	}

	/*
	 * it is possible to have several terms with the same name in the same taxonomy (one per language)
	 * but the native term_exists will return true even if only one exists
	 * so here the function adds the language parameter
	 *
	 * @since 1.4
	 *
	 * @param string $term_name the term name
	 * @param string $taxonomy taxonomy name
	 * @param int $parent parent term id
	 * @param string|object $language the language slug or object
	 * @return null|int the term_id of the found term
	 */
	public function term_exists($term_name, $taxonomy, $parent, $language) {
		global $wpdb;

		$select = "SELECT t.term_id FROM $wpdb->terms AS t";
		$join = " INNER JOIN $wpdb->term_taxonomy AS tt ON t.term_id = tt.term_id";
		$join .= $this->join_clause('term');
		$where = $wpdb->prepare(" WHERE tt.taxonomy = %s AND t.name = %s", $taxonomy, $term_name);
		$where .= $this->where_clause($this->get_language($language), 'term');

		if ($parent > 0)
			$where .= $wpdb->prepare(" AND tt.parent = %d", $parent);

		return $wpdb->get_var($select . $join . $where);
	}

	/*
	 * gets the number of posts per language in a date, author or post type archive
	 *
	 * @since 1.2
	 *
	 * @param object lang
	 * @param array $q WP_Query arguments (accepted: post_type, m, year, monthnum, day, author, author_name, post_format)
	 * @return int
	 */
	public function count_posts($lang, $q = array()) {
		global $wpdb;

		if (empty($q['post_type']))
			$q['post_type'] = array('post'); // we *need* a post type

		if (!is_array($q['post_type']))
			$q['post_type'] = array($q['post_type']);

		$cache_key = md5(serialize($q));
		$counts = wp_cache_get($cache_key, 'pll_count_posts');

		if (false === $counts) {
			$select = "SELECT pll_tr.term_taxonomy_id, COUNT(*) AS num_posts FROM {$wpdb->posts} AS p";
			$join = $this->join_clause('post');
			$where = " WHERE post_status = 'publish'";
			$where .= " AND p.post_type IN ('" . join("', '", $q['post_type'] ) . "')";
			$where .= $this->where_clause($this->get_languages_list(), 'post');
			$groupby = " GROUP BY pll_tr.term_taxonomy_id";

			if (!empty($q['m'])) {
				$q['m'] = '' . preg_replace('|[^0-9]|', '', $q['m']);
				$where .= $wpdb->prepare(" AND YEAR(p.post_date) = %d", substr($q['m'], 0, 4));
				if ( strlen($q['m']) > 5 )
					$where .= $wpdb->prepare(" AND MONTH(p.post_date) = %d", substr($q['m'], 4, 2));
				if ( strlen($q['m']) > 7 )
					$where .= $wpdb->prepare(" AND DAYOFMONTH(p.post_date) = %d", substr($q['m'], 6, 2));
			}

			if (!empty($q['year']))
				$where .= $wpdb->prepare(" AND YEAR(p.post_date) = %d", $q['year']);

			if (!empty($q['monthnum']))
				$where .= $wpdb->prepare(" AND MONTH(p.post_date) = %d", $q['monthnum']);

			if (!empty($q['day']))
				$where .= $wpdb->prepare(" AND DAYOFMONTH(p.post_date) = %d", $q['day']);

			if (!empty($q['author_name'])) {
				$author = get_user_by('slug',  sanitize_title_for_query($q['author_name']));
				if ($author)
					$q['author'] = $author->ID;
			}

			if (!empty($q['author']))
				$where .= $wpdb->prepare(" AND p.post_author = %d", $q['author']);

			if (!empty($q['post_format'])) {
				$join .= " INNER JOIN {$wpdb->term_relationships} AS tr ON tr.object_id = p.ID";
				$join .= " INNER JOIN {$wpdb->term_taxonomy} AS tt ON tt.term_taxonomy_id = tr.term_taxonomy_id";
				$join .= " INNER JOIN {$wpdb->terms} AS t ON t.term_id = tt.term_id";
				$where .= $wpdb->prepare(" AND t.slug = %s", $q['post_format']);
			}

			$res = $wpdb->get_results($select . $join . $where . $groupby, ARRAY_A);
			foreach ((array) $res as $row)
				$counts[$row['term_taxonomy_id']] = $row['num_posts'];

			wp_cache_set($cache_key, $counts, 'pll_count_posts');
		}

		return empty($counts[$lang->term_taxonomy_id]) ? 0 : $counts[$lang->term_taxonomy_id];
	}

	/*
	 * returns ids of objects in a language similarly to get_objects_in_term for a taxonomy
	 * faster than get_objects_in_term as it avoids a JOIN
	 *
	 * @since 1.4
	 *
	 * @param object $lang a PLL_Language object
	 * @param string $type optional, either 'post' or 'term', defaults to 'post'
	 * @return array
	 */
	public function get_objects_in_language($lang, $type = 'post') {
		global $wpdb;
		return $wpdb->get_col($wpdb->prepare("
			SELECT object_id FROM $wpdb->term_relationships WHERE term_taxonomy_id = %d",
			'term' == $type ? $lang->tl_term_taxonomy_id : $lang->term_taxonomy_id
		));
	}

	/*
	 * setup the links model based on options
	 *
	 * @since 1.2
	 *
	 * @return object implementing "links_model interface"
	 */
	public function get_links_model() {
		$c = array('Directory', 'Directory', 'Subdomain', 'Domain');
		$class = get_option('permalink_structure') ? 'PLL_Links_' .$c[$this->options['force_lang']] : 'PLL_Links_Default';
		return new $class($this);
	}
}
