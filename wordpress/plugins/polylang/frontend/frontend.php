<?php

/*
 * frontend controller
 * accessible as $polylang global object
 *
 * properties:
 * options          => inherited, reference to Polylang options array
 * model            => inherited, reference to PLL_Model object
 * links_model      => inherited, reference to PLL_Links_Model object
 * links            => reference to PLL_Links object
 * choose_lang      => reference to PLL_Choose_lang object
 * curlang          => current language
 * filters          => reference to PLL_Filters object
 * filters_search   => reference to PLL_Frontend_Filters_Search object
 * nav_menu         => reference to PLL_Frontend_Nav_Menu object
 * auto_translate   => optional, reference to PLL_Auto_Translate object
 *
 * @since 1.2
 */
class PLL_Frontend extends PLL_Base {
	public $curlang;
	public $links, $choose_lang, $filters, $filters_search, $auto_translate;

	/*
	 * constructor
	 *
	 * @since 1.2
	 *
	 * @param object $links_model
	 */
	public function __construct(&$links_model) {
		parent::__construct($links_model);

		add_action('pll_language_defined', array(&$this, 'pll_language_defined'), 1, 2);

		// filters posts by language
		add_filter('parse_query', array(&$this, 'parse_query'), 6);

		// not before 'check_language_code_in_url'
		if (!defined('PLL_AUTO_TRANSLATE') || PLL_AUTO_TRANSLATE)
			add_action('wp', array(&$this, 'auto_translate'), 20);
	}

	/*
	 * setups url modifications based on the links mode
	 * setups the language chooser based on options
	 *
	 * @since 1.2
	 */
	public function init() {
		$this->links = new PLL_Frontend_Links($this);

		$c = array('Content', 'Url', 'Url', 'Domain');
		$class = 'PLL_Choose_Lang_' . $c[$this->options['force_lang'] * (get_option('permalink_structure') ? 1 : 0 )];
		$this->choose_lang = new $class($this);
	}

	/*
	 * setups filters and nav menus once the language has been defined
	 *
	 * @since 1.2
	 *
	 * @param string $slug current language slug
	 * @param object $curlang current language object
	 */
	public function pll_language_defined($slug, $curlang) {
		$this->curlang = $curlang;

		// filters
		$this->filters = new PLL_Frontend_Filters($this);
		$this->filters_search = new PLL_Frontend_Filters_Search($this);

		// nav menu
		$this->nav_menu = new PLL_Frontend_Nav_Menu($this);
	}

	/*
	 * modifies some query vars to "hide" that the language is a taxonomy and avoid conflicts
	 *
	 * @since 1.2
	 *
	 * @param object $query WP_Query object
	 */
	public function parse_query($query) {
		$qv = $query->query_vars;

		// to avoid conflict beetwen taxonomies
		// FIXME generalize post format like taxonomies (untranslated but filtered)
		$has_tax = false;
		if (isset($query->tax_query->queries))
			foreach ($query->tax_query->queries as $tax)
				if ('post_format' != $tax['taxonomy'])
					$has_tax = true;

		// allow filtering recent posts and secondary queries by the current language
		// take care not to break queries for non visible post types such as nav_menu_items
		// do not filter if lang is set to an empty value
		// do not filter single page and translated taxonomies to avoid conflicts
		if (!empty($this->curlang) && !isset($qv['lang']) && !$has_tax && empty($qv['page_id']) && empty($qv['pagename']) && (empty($qv['post_type']) || $this->model->is_translated_post_type($qv['post_type']))) {
			$this->choose_lang->set_lang_query_var($query, $this->curlang);
		}

		// modifies query vars when the language is queried
		if (!empty($qv['lang'])) {
			// remove pages query when the language is set unless we do a search
			if (empty($qv['post_type']) && !$query->is_search)
				$query->set('post_type', 'post');

			// unset the is_archive flag for language pages to prevent loading the archive template
			// keep archive flag for comment feed otherwise the language filter does not work
			if (!$query->is_comment_feed && !$query->is_post_type_archive && !$query->is_date && !$query->is_author && !$query->is_category && !$query->is_tag && !$query->is_tax('post_format'))
				$query->is_archive = false;

			// unset the is_tax flag for authors pages and post types archives
			// FIXME Should I do this for other cases?
			if ($query->is_author || $query->is_post_type_archive || $query->is_date || $query->is_search) {
				$query->is_tax = false;
				unset($query->queried_object);
			}
		}
	}

	/*
	 * auto translate posts and terms ids
	 *
	 * @since 1.2
	 */
	public function auto_translate() {
		$this->auto_translate = new PLL_Frontend_Auto_Translate($this);
	}

	/*
	 * resets some variables when switching blog
	 * overrides parent method
	 *
	 * @since 1.5.1
	 */
	public function switch_blog($new_blog, $old_blog) {
		// need to check that some languages are defined when user is logged in, has several blogs, some without any languages
		if (parent::switch_blog($new_blog, $old_blog) && did_action('pll_language_defined') && $this->model->get_languages_list()) {
			static $restore_curlang;
			if (empty($restore_curlang))
				$restore_curlang = $this->curlang->slug; // to always remember the current language through blogs

			// FIXME need some simplification as there are too many variables storing the same value
			$lang = $this->model->get_language($restore_curlang);
			$this->curlang = $lang ? $lang : $this->model->get_language($this->options['default_lang']);
			$this->choose_lang->curlang = $this->links->curlang = $this->curlang;

			$this->links->home = $this->links_model->home; // set in parent class

			if ('page' == get_option('show_on_front')) {
				$this->choose_lang->page_on_front = $this->links->page_on_front = get_option('page_on_front');
				$this->choose_lang->page_for_posts = $this->links->page_for_posts = get_option('page_for_posts');
			}
			else {
				$this->choose_lang->page_on_front = $this->links->page_on_front = 0;
				$this->choose_lang->page_for_posts = $this->links->page_for_posts = 0;
			}

			$this->filters_search->using_permalinks = $this->links->using_permalinks = (bool) get_option('permalink_structure');
		}
	}
}

