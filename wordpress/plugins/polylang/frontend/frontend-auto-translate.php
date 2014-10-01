<?php
/*
 * auto translates the posts and terms ids
 * useful for example for themes querying a specific cat
 *
 * @since 1.1
 */
class PLL_Frontend_Auto_Translate {
	public $model;


	/*
	 * constructor
	 *
	 * @since 1.1
	 *
	 * @param object $polylang
	 */
	public function __construct(&$polylang) {
		$this->model = &$polylang->model;

		add_action('pre_get_posts', array(&$this, 'pre_get_posts')); // after main Polylang filter
		add_filter('get_terms_args', array(&$this, 'get_terms_args'), 10, 2);
	}

	/*
	 * filters posts query to automatically translate included ids
	 *
	 * @since 1.1
	 *
	 * @param object $query WP_Query object
	 */
	public function pre_get_posts($query) {
		global $wpdb;
		$qv = &$query->query_vars;

		if ($query->is_main_query() || !empty($qv['lang']) || (!empty($qv['post_type']) && !$this->model->is_translated_post_type($qv['post_type'])))
			return;

		$sign = create_function('$n', 'return $n > 0 ? 1 : ($n < 0 ? -1 : 0);');

		// /!\ always keep untranslated as is

		// term ids separated by a comma
		$arr = array();
		if (!empty($qv['cat'])) {
			foreach (explode(',', $qv['cat']) as $cat)
				$arr[] = ($tr = $sign($cat) * pll_get_term(abs($cat))) ? $tr : $cat;

			$qv['cat'] = implode(',', $arr);
		}

		// category_name
		$arr = array();
		if (!empty($qv['category_name'])) {
			foreach (explode(',', $qv['category_name']) as $slug)
				$arr[] = (($cat = get_category_by_slug($slug)) && ($tr_id = pll_get_term($cat->term_id)) && !is_wp_error($tr = get_category($tr_id))) ?
					$tr->slug : $slug;

			$qv['category_name'] = implode(',', $arr);
		}

		// array of term ids
		foreach (array('category__and', 'category__in', 'category__not_in', 'tag__and', 'tag__in', 'tag__not_in') as $key) {
			$arr = array();
			if (!empty($qv[$key])) {
				foreach ($qv[$key] as $cat)
					$arr[] = ($tr = pll_get_term($cat)) ? $tr : $cat;

				$qv[$key] = $arr;
			}
		}

		// tag
		$arr = array();
		if (!empty($qv['tag'])) {
			$sep = strpos($qv['tag'], ',') !== false ? ',' : '+'; // two possible separators for tag slugs
			foreach (explode($sep, $qv['tag']) as $slug)
				$arr[] = (($tag = get_term_by('slug', $slug, 'post_tag')) && ($tr_id = pll_get_term($tag->term_id)) && !is_wp_error($tr = get_tag($tr_id))) ?
					$tr->slug : $slug;

			$qv['tag'] = implode($sep, $arr);
		}

		// tag_id can only take one id
		if (!empty($qv['tag_id']) && $tr_id = pll_get_term($qv['tag_id']))
			$qv['tag_id'] = $tr_id;


		// array of tag slugs
		foreach (array('tag_slug__and', 'tag_slug__in') as $key) {
			$arr = array();
			if (!empty($qv[$key])) {
				foreach ($qv[$key] as $slug)
					$arr[] = (($tag = get_term_by('slug', $slug, 'post_tag')) && ($tr_id = pll_get_term($tag->term_id)) && !is_wp_error($tr = get_tag($tr_id))) ?
						$tr->slug : $slug;

				$qv[$key] = $arr;
			}
		}


		// custom taxonomies
		// according to codex, this type of query is deprecated as of WP 3.1 but it does not appear in WP 3.5 source code
		foreach (array_intersect($this->model->get_translated_taxonomies(), get_taxonomies(array('_builtin' => false))) as $taxonomy) {
			$tax = get_taxonomy($taxonomy);
			$arr = array();
			if (!empty($qv[$tax->query_var])) {
				$sep = strpos($qv[$tax->query_var], ',') !== false ? ',' : '+'; // two possible separators
				foreach (explode($sep, $qv[$tax->query_var]) as $slug)
					$arr[] = (($tag = get_term_by('slug', $slug, $taxonomy)) && ($tr_id = pll_get_term($tag->term_id)) && !is_wp_error($tr = get_term($tr_id, $taxonomy))) ?
						$tr->slug : $slug;

				$qv[$tax->query_var] = implode($sep, $arr);
			}
		}

		// tax_query since WP 3.1
		if (!empty($qv['tax_query']) && is_array($qv['tax_query'])) {
			foreach ($qv['tax_query'] as $key => $q) {
				if (isset($q['taxonomy']) && $this->model->is_translated_taxonomy($q['taxonomy'])) {
					$arr = array();
					$field = isset($q['field']) && in_array($q['field'], array('slug', 'name')) ? $q['field'] : 'term_id';
					foreach ( (array) $q['terms'] as $t)
						$arr[] = (($tag = get_term_by($field, $t, $q['taxonomy'])) && ($tr_id = pll_get_term($tag->term_id)) && !is_wp_error($tr = get_term($tr_id, $q['taxonomy']))) ?
							$tr->$field : $t;

					$qv['tax_query'][$key]['terms'] = $arr;
				}
			}
		}


		// p, page_id, post_parent can only take one id
		foreach (array('p', 'page_id', 'post_parent') as $key)
			if (!empty($qv[$key]) && $tr_id = pll_get_post($qv[$key]))
				$qv[$key] = $tr_id;

		// name, pagename can only take one slug
		foreach (array('name', 'pagename') as $key) {
			if (!empty($qv[$key])) {
				// no function to get post by name except get_posts itself
				$post_type = empty($qv['post_type']) ? 'post' : $qv['post_type'];
				$id = $wpdb->get_var($wpdb->prepare("SELECT ID from $wpdb->posts WHERE post_type=%s AND post_name=%s", $post_type, $qv[$key]));
				$qv[$key] = ($id && ($tr_id = pll_get_post($id)) && $tr = get_post($tr_id)) ? $tr->post_name : $qv[$key];
			}
		}

		// array of post ids
		// post_parent__in & post_parent__not_in since WP 3.6
		foreach (array('post__in', 'post__not_in', 'post_parent__in', 'post_parent__not_in') as $key) {
			$arr = array();
			if (!empty($qv[$key])) {
				// post__in used by the 2 functions below
				// useless to filter them as output is already in the right language and would result in performance loss
				foreach (debug_backtrace() as $trace)
					if (in_array($trace['function'], array('wp_nav_menu', 'gallery_shortcode')))
						return;

				foreach ($qv[$key] as $p)
					$arr[] = ($tr = pll_get_post($p)) ? $tr : $p;

				$qv[$key] = $arr;
			}
		}
	}

	/*
	 * filters terms query to automatically translate included ids
	 *
	 * @since 1.1.1
	 *
	 * @param array $args
	 * @param array $taxonomies
	 * @return array modified $args
	 */
	public function get_terms_args($args, $taxonomies) {
		if (!empty($args['include']) && $this->model->is_translated_taxonomy($taxonomies)) {
			foreach(wp_parse_id_list($args['include']) as $id)
				$arr[] = ($tr = pll_get_term($id)) ? $tr : $id;

			$args['include'] = $arr;
		}
		return $args;
	}
}
