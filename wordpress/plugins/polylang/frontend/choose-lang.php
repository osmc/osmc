<?php

/*
 * base class to choose the language
 *
 * @since 1.2
 */
abstract class PLL_Choose_Lang {
	public $links, $links_model, $model, $options;
	public $page_on_front = 0, $page_for_posts = 0, $curlang;

	/*
	 * constructor
	 *
	 * @since 1.2
	 *
	 * @param object $polylang
	 */
	public function __construct(&$polylang) {
		$this->links = &$polylang->links;
		$this->links_model = &$polylang->links_model;
		$this->model = &$polylang->model;
		$this->options = &$polylang->options;

		if ('page' == get_option('show_on_front')) {
			$this->page_on_front = get_option('page_on_front');
			$this->page_for_posts = get_option('page_for_posts');
		}

		if (PLL_AJAX_ON_FRONT || false === stripos($_SERVER['SCRIPT_FILENAME'], 'index.php'))
			$this->set_language(empty($_REQUEST['lang']) ? $this->get_preferred_language() : $this->model->get_language($_REQUEST['lang']));

		add_action('pre_comment_on_post', array(&$this, 'pre_comment_on_post')); // sets the language of comment
		add_filter('parse_query', array(&$this, 'parse_main_query'), 2); // sets the language in special cases
	}

	/*
	 * writes language cookie
	 * loads user defined translations
	 * fires the action 'pll_language_defined'
	 *
	 * @since 1.2
	 *
	 * @param object $curlang current language
	 */
	protected function set_language($curlang) {
		// don't set the language a second time
		if (isset($this->curlang))
			return;

		$this->curlang = $curlang;
		$this->maybe_setcookie();

		$GLOBALS['text_direction'] = $curlang->is_rtl ? 'rtl' : 'ltr';
		do_action('pll_language_defined', $curlang->slug, $curlang);
	}

	/*
	 * set a cookie to remember the language.
	 * possibility to set PLL_COOKIE to false will disable cookie although it will break some functionalities
	 *
	 * @since 1.5
	 */
	protected function maybe_setcookie() {
		// check headers have not been sent to avoid ugly error
		// cookie domain must be set to false for localhost (default value for COOKIE_DOMAIN) thanks to Stephen Harris.
		if (!headers_sent() && PLL_COOKIE !== false && (!isset($_COOKIE[PLL_COOKIE]) || $_COOKIE[PLL_COOKIE] != $this->curlang->slug))
			setcookie(
				PLL_COOKIE,
				$this->curlang->slug,
				time() + 31536000 /* 1 year */,
				COOKIEPATH,
				2 == $this->options['force_lang'] ? parse_url(get_option('home'), PHP_URL_HOST) : COOKIE_DOMAIN
			);
	}

	/*
	 * returns the language according to browser preference or the default language
	 *
	 * @since 0.1
	 *
	 * @return object browser preferred language or default language
	 */
	public function get_preferred_language() {
		// check first if the user was already browsing this site
		if (isset($_COOKIE[PLL_COOKIE]))
			return $this->model->get_language($_COOKIE[PLL_COOKIE]);

		// sets the browsing language according to the browser preferences
		// code adapted from http://www.thefutureoftheweb.com/blog/use-accept-language-header
		if ($this->options['browser']) {
			$accept_langs = array();

			if (isset($_SERVER['HTTP_ACCEPT_LANGUAGE'])) {
				// break up string into pieces (languages and q factors)
				preg_match_all('/([a-z]{1,8}(-[a-z]{1,8})?)\s*(;\s*q\s*=\s*(1|0\.[0-9]+))?/i', $_SERVER['HTTP_ACCEPT_LANGUAGE'], $lang_parse);

				$k = $lang_parse[1];
				$v = $lang_parse[4];

				if ($n = count($k)) {
					// set default to 1 for any without q factor
					foreach ($v as $key => $val)
						if ($val === '') $v[$key] = 1;

					// bubble sort (need a stable sort for Android, so can't use a PHP sort function)
					if ($n > 1) {
						for ($i = 2; $i <= $n; $i++)
							for ($j = 0; $j <= $n-2; $j++)
								if ( $v[$j] < $v[$j + 1]) {
									// swap values
									$temp = $v[$j];
									$v[$j] = $v[$j + 1];
									$v[$j + 1] = $temp;
									//swap keys
									$temp = $k[$j];
									$k[$j] = $k[$j + 1];
									$k[$j + 1] = $temp;
								}
					}
					$accept_langs = array_combine($k,$v);
				}
			}

			// looks through sorted list and use first one that matches our language list
			$listlanguages = $this->model->get_languages_list(array('hide_empty' => true)); // hides languages with no post
			foreach (array_keys($accept_langs) as $accept_lang) {
				foreach ($listlanguages as $language)
					if (empty($pref_lang) && (0 === stripos($accept_lang, $language->slug) || 0 == strcasecmp($accept_lang, str_replace('_', '-', $language->locale))) )
						$pref_lang = $language;
			}
		} // options['browser']

		// allow plugin to modify the preferred language (useful for example to have a different fallback than the default language)
		$slug = apply_filters('pll_preferred_language', isset($pref_lang) ? $pref_lang->slug : false);

		// return default if there is no preferences in the browser or preferences does not match our languages or it is requested not to use the browser preference
		return ($lang = $this->model->get_language($slug)) ? $lang : $this->model->get_language($this->options['default_lang']);
	}

	/*
	 * sets the language when home page is resquested
	 *
	 * @since 1.2
	 */
	protected function home_language() {
		// test referer in case PLL_COOKIE is set to false
		// thanks to Ov3rfly http://wordpress.org/support/topic/enhance-feature-when-front-page-is-visited-set-language-according-to-browser
		$this->set_language(
			$this->options['hide_default'] && ((isset($_SERVER['HTTP_REFERER']) && in_array(parse_url($_SERVER['HTTP_REFERER'], PHP_URL_HOST), $this->links_model->get_hosts())) || !$this->options['browser']) ?
				$this->model->get_language($this->options['default_lang']) :
				$this->get_preferred_language() // sets the language according to browser preference or default language
		);
	}

	/*
	 * to call when the home page has been requested
	 * make sure to call this after 'setup_theme' has been fired as we need $wp_query
	 * performs a redirection to the home page in the current language if needed
	 *
	 * @since 0.9
	 */
	public function home_requested() {
		// we are already on the right page
		if ($this->options['default_lang'] == $this->curlang->slug && $this->options['hide_default']) {
			$this->set_home_query_var();
		}
		// redirect to the home page in the right language
		// test to avoid crash if get_home_url returns something wrong
		// FIXME why this happens? http://wordpress.org/support/topic/polylang-crashes-1
		// don't redirect if $_POST is not empty as it could break other plugins
		// don't forget the query string which may be added by plugins
		elseif (is_string($redirect = $this->links->get_home_url($this->curlang)) && empty($_POST)) {
			$redirect = empty($_SERVER['QUERY_STRING']) ? $redirect : $redirect . (get_option('permalink_structure') ? '?' : '&') . $_SERVER['QUERY_STRING'];
			if ($redirect = apply_filters('pll_redirect_home', $redirect)) {
				wp_redirect($redirect);
				exit;
			}
		}

	}

	/*
	 * Adds query vars to query for home page
	 *
	 * @since 1.5
	 */
	protected function set_home_query_var() {
		if ($this->page_on_front && $page_id = $this->model->get_post($this->page_on_front, $this->curlang))
			set_query_var('page_id', $page_id);
		else
			$this->set_lang_query_var($GLOBALS['wp_query'], $this->curlang);
	}

	/*
	 * set the language when posting a comment
	 *
	 * @since 0.8.4
	 *
	 * @param int $post_id the post beeing commented
	 */
	public function pre_comment_on_post($post_id) {
		$this->set_language($this->model->get_post_language($post_id));
	}

	/*
	 * modifies some main query vars for home page and page for posts
	 * to enable one home page (and one page for posts) per language
	 *
	 * @since 1.2
	 *
	 * @param object $query instance of WP_Query
	 */
	public function parse_main_query($query) {
		if (!$query->is_main_query())
			return;

		$qv = $query->query_vars;

		// redirect the language page to the homepage when using a static front page
		if ($this->options['redirect_lang'] && is_tax('language') && $this->page_on_front && (count($query->query) == 1 || (is_paged() && count($query->query) == 2))) {
			$this->set_language($this->model->get_language(get_query_var('lang')));
			if ($page_id = $this->model->get_post($this->page_on_front, $this->model->get_language(get_query_var('lang')))) {
				$query->set('page_id', $page_id);
				$query->is_singular = $query->is_page = true;
				$query->is_archive = $query->is_tax = false;
				unset($query->query_vars['lang'], $query->queried_object); // reset queried object
			}
			// else : the static front page is not translated
			// let's things as is and the list of posts in the current language will be displayed
		}

		// takes care of paged front page
		if ($this->page_on_front) {
			$page_id = $this->get_page_id($query);

			// correct <!--nextpage--> for page_on_front
			if (!empty($page_id) && $this->model->get_post($page_id, $this->model->get_post_language($this->page_on_front)) == $this->page_on_front && !empty($qv['paged'])) {
				$query->set('page', $qv['paged']);
				unset($query->query_vars['paged']);
			}
		}

		// sets is_home on translated home page when it displays posts
		// is_home must be true on page 2, 3... too
		// as well as when searching an empty string: http://wordpress.org/support/topic/plugin-polylang-polylang-breaks-search-in-spun-theme
		if (!$this->page_on_front && is_tax('language') && (count($query->query) == 1 || (is_paged() && count($query->query) == 2) || (isset($query->query['s']) && !$query->query['s']))) {
			$this->set_language($this->model->get_language(get_query_var('lang'))); // sets the language now otherwise it will be too late to filter sticky posts !
			$query->is_home = true;
			$query->is_archive = $query->is_tax = false;
		}

		// sets the language for posts page in case the front page displays a static page
		if ($this->page_for_posts) {
			$page_id = $this->get_page_id($query);

			if (!empty($page_id) && $this->model->get_post($page_id, $this->model->get_post_language($this->page_for_posts)) == $this->page_for_posts) {
				$this->set_language($this->model->get_post_language($page_id));
				$this->set_lang_query_var($query, $this->curlang);
				$query->is_singular = $query->is_page = false;
				$query->is_home = $query->is_posts_page = true;
			}
		}
	}

	/*
	 * sets the language in query
	 * optimized for (needs) WP 3.5+
	 *
	 * @since 1.3
	 */
	public function set_lang_query_var(&$query, $lang) {
		// defining directly the tax_query (rather than setting 'lang' avoids transforming the query by WP)
		$query->query_vars['tax_query'][] = array(
			'taxonomy' => 'language',
			'field'    => 'term_taxonomy_id', // since WP 3.5
			'terms'    => $lang->term_taxonomy_id,
			'operator' => 'IN'
		);
	}

	/*
	 * get queried page_id (if exists)
	 * If permalinks are used, WordPress does set and use $query->queried_object_id and sets $query->query_vars['page_id'] to 0
	 * and does set and use $query->query_vars['page_id'] if permalinks are not used :(
	 *
	 * @since 1.5
	 *
	 * @param object $query instance of WP_Query
	 * @return int page_id
	 */
	protected function get_page_id($query) {
		if (!empty($query->query_vars['pagename']) && isset($query->queried_object_id))
			return $query->queried_object_id;

		if (isset($query->query_vars['page_id']))
			return $query->query_vars['page_id'];

		return 0; // no page queried
	}
}
