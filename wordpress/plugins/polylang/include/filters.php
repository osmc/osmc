<?php

/*
 * setup filters common to admin and frontend
 *
 * @since 1.4
 */
class PLL_Filters {
	public $links_model, $model, $options, $curlang;

	/*
	 * constructor: setups filters
	 *
	 * @since 1.4
	 *
	 * @param object $polylang
	 */
	public function __construct(&$polylang) {
		$this->links_model = &$polylang->links_model;
		$this->model = &$polylang->model;
		$this->options = &$polylang->options;
		$this->curlang = &$polylang->curlang;

		// filters the comments according to the current language
		add_filter('comments_clauses', array(&$this, 'comments_clauses'), 10, 2);

		// filters the get_pages function according to the current language
		add_filter('get_pages', array(&$this, 'get_pages'), 10, 2);

		// adds cache domain when querying terms
		add_filter('get_terms_args', array(&$this, 'get_terms_args'));
	}

	/*
	 * filters the comments according to the current language
	 * used by the recent comments widget and admin language filter
	 *
	 * @since 0.2
	 *
	 * @param array $clauses sql clauses
	 * @param object $query WP_Comment_Query object
	 * @return array modified $clauses
	 */
	public function comments_clauses($clauses, $query) {
		global $wpdb;

		$lang = empty($query->query_vars['lang']) ? $this->curlang : $this->model->get_language($query->query_vars['lang']);

		if (!empty($lang)) {
			// if this clause is not already added by WP
			if (!strpos($clauses['join'], '.ID'))
				$clauses['join'] .= " JOIN $wpdb->posts ON $wpdb->posts.ID = $wpdb->comments.comment_post_ID";

			$clauses['join'] .= $this->model->join_clause('post');
			$clauses['where'] .= $this->model->where_clause($lang, 'post');
		}
		return $clauses;
	}

	/*
	 * filters get_pages per language
	 *
	 * @since 1.4
	 *
	 * @param array $pages an array of pages already queried
	 * @param array $args get_pages arguments
	 * @return array modified list of pages
	 */
	public function get_pages($pages, $args) {
		$language = empty($args['lang']) ? $this->curlang : $this->model->get_language($args['lang']);

		if (empty($language) || empty($pages) || !$this->model->is_translated_post_type($args['post_type']))
			return $pages;

		static $once = false;

		// obliged to redo the get_pages query if we want to get the right number
		if (!empty($args['number']) && !$once) {
			$once = true; // avoid infinite loop

			$r = array(
				'lang' => 0, // so this query is not filtered
				'numberposts' => -1,
				'nopaging'    => true,
				'post_type'   => $args['post_type'],
				'fields'      => 'ids',
				'tax_query'   => array(array(
					'taxonomy' => 'language',
					'field'    => 'term_taxonomy_id', // since WP 3.5
					'terms'    => $language->term_taxonomy_id,
					'operator' => 'NOT IN'
				))
			);

			$args['exclude'] = array_merge($args['exclude'], get_posts($r));
			$pages = get_pages($args);
		}

		$ids = wp_list_pluck($pages, 'ID');

		// filters the queried list of pages by language
		if (!$once) {
			$ids = array_intersect($ids, $this->model->get_objects_in_language($language));

			foreach ($pages as $key => $page) {
				if (!in_array($page->ID, $ids))
					unset($pages[$key]);
			}
		}

		// not done by WP but extremely useful for performance when manipulating taxonomies
		update_object_term_cache($ids, $args['post_type']);

		$once = false; // in case get_pages is called another time
		return $pages;
	}

	/*
	 * adds language dependent cache domain when querying terms
	 * useful as the 'lang' parameter is not included in cache key by WordPress
	 *
	 * @since 1.3
	 */
	public function get_terms_args($args) {
		if (!empty($args['lang']))
			$lang = $args['lang'];
		elseif (!empty($this->curlang))
			$lang = $this->curlang->slug;

		if (isset($lang)) {
			$key = '_' . (is_array($lang) ? implode(',', $lang) : $lang);
			$args['cache_domain'] = empty($args['cache_domain']) ? 'pll' . $key : $args['cache_domain'] . $key;
		}

		return $args;
	}
}
