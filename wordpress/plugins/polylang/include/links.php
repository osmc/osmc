<?php

/*
 * manages links filters needed on both frontend and admin
 *
 * @since 1.2
 */
class PLL_Links {
	public $links_model, $model, $options;
	protected $_links;

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

		// adds our domains or subdomains to allowed hosts for safe redirection
		add_filter('allowed_redirect_hosts', array(&$this, 'allowed_redirect_hosts'));

		// low priority on links filters to come after any other modifications
		if ($this->options['force_lang']) {
			foreach (array('post_link', '_get_page_link', 'post_type_link') as $filter)
				add_filter($filter, array(&$this, 'post_link'), 20, 2);

			add_filter('term_link', array(&$this, 'term_link'), 20, 3);
		}
	}

	/*
	 * adds our domains or subdomains to allowed hosts for safe redirection
	 *
	 * @since 1.4.3
	 *
	 * @param array $hosts allowed hosts
	 * @return array
	 */
	public function allowed_redirect_hosts($hosts) {
		return array_unique(array_merge($hosts, $this->links_model->get_hosts()));
	}

	/*
	 * modifies post & page links
	 *
	 * @since 0.7
	 *
	 * @param string $link post link
	 * @param object|int $post post object or post ID
	 * @return string modified post link
	 */
	public function post_link($link, $post) {
		if (isset($this->_links[$link]))
			return $this->_links[$link];

		if ('post_type_link' == current_filter() && !$this->model->is_translated_post_type($post->post_type))
			return $this->_links[$link] = $link;

		if ('_get_page_link' == current_filter()) // this filter uses the ID instead of the post object
			$post = get_post($post);

		// /!\ when post_status is not "publish", WP does not use pretty permalinks
		return $this->_links[$link] = $post->post_status != 'publish' ? $link : $this->links_model->add_language_to_link($link, $this->model->get_post_language($post->ID));
	}

	/*
	 * modifies term link
	 *
	 * @since 0.7
	 *
	 * @param string $link term link
	 * @param object $post term object
	 * @param string $tax taxonomy name
	 * @return string modified term link
	 */
	public function term_link($link, $term, $tax) {
		if (isset($this->_links[$link]))
			return $this->_links[$link];

		return $this->_links[$link] = $this->model->is_translated_taxonomy($tax) ?
			$this->links_model->add_language_to_link($link, $this->model->get_term_language($term->term_id)) : $link;
	}

	/*
	 * returns the home url in the requested language
	 *
	 * @since 1.3
	 *
	 * @param object|string $language
	 * @param bool $is_search optional wether we need the home url for a search form, defaults to false
	 */
	public function get_home_url($language, $is_search = false) {
		$language = is_object($language) ? $language : $this->model->get_language($language);
		return $is_search ? $language->search_url : $language->home_url;
	}

	/*
	 * get the link to create a new post translation
	 *
	 * @since 1.5
	 *
	 * @param int $post_id
	 * @param object $language
	 * @return string
	 */
	public function get_new_post_translation_link($post_id, $language) {
		$post_type = get_post_type($post_id);

		if ('attachment' == $post_type) {
			$args = array(
				'action' => 'translate_media',
				'from_media' => $post_id,
				'new_lang'  => $language->slug
			);

			// add nonce for media as we will directly publish a new attachment from a clic on this link
			return wp_nonce_url(add_query_arg($args, admin_url('admin.php')), 'translate_media');
		}
		else {
			$args = array(
				'post_type' => $post_type,
				'from_post' => $post_id,
				'new_lang'  => $language->slug
			);

			return add_query_arg($args, admin_url('post-new.php'));
		}
	}

	/*
	 * get the link to create a new term translation
	 *
	 * @since 1.5
	 *
	 * @param int $term_id
	 * @param string $taxonomy
	 * @param string $post_type
	 * @param object $language
	 * @return string
	 */
	public function get_new_term_translation_link($term_id, $taxonomy, $post_type, $language) {
 		$args = array(
			'taxonomy'  => $taxonomy,
			'post_type' => $post_type,
			'from_tag'  => $term_id,
			'new_lang'  => $language->slug
		);

		return add_query_arg($args, admin_url('edit-tags.php'));
	}
}

