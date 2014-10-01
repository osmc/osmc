<?php

/*
 * links model for default permalinks
 * for example mysite.com/?somevar=something&lang=en
 * implements the "links_model interface"
 *
 * @since 1.2
 */
class PLL_Links_Default extends PLL_Links_Model {

	/*
	 * adds language information to an url
	 * links_model interface
	 *
	 * @since 1.2
	 *
	 * @param string $url url to modify
	 * @param object $lang language
	 * @return string modified url
	 */
	public function add_language_to_link($url, $lang) {
		return !empty($lang) && '_get_page_link' != current_filter() ? add_query_arg( 'lang', $lang->slug, $url ) : $url;
	}

	/*
	 * removes the language information from an url
	 * links_model interface
	 *
	 * @since 1.2
	 *
	 * @param string $url url to modify
	 * @return string modified url
	 */
	public function remove_language_from_link($url) {
		return remove_query_arg('lang', $url);
	}

	/*
	 * returns the link to the first page
	 * links_model interface
	 *
	 * @since 1.2
	 *
	 * @param string $url url to modify
	 * @return string modified url
	 */
	function remove_paged_from_link($url) {
		return remove_query_arg('paged', $url);
	}


	/*
	 * returns the link to the paged page when using pretty permalinks
	 *
	 * @since 1.5
	 *
	 * @param string $url url to modify
	 * @param int $page
	 * @return string modified url
	 */
	public function add_paged_to_link($url, $page) {
		return add_query_arg(array('paged' => $page), $url);
	}


	/*
	 * gets the language slug from the url if present
	 * links_model interface
	 *
	 * @since 1.2
	 *
	 * @return string language slug
	 */
	public function get_language_from_url() {
		$pattern = '#lang=('.implode('|', $this->model->get_languages_list(array('fields' => 'slug'))).')#';
		return preg_match($pattern, trailingslashit($_SERVER['REQUEST_URI']), $matches) ? $matches[1] : ''; // $matches[1] is the slug of the requested language
	}

	/*
	 * returns the home url
	 * links_model interface
	 *
	 * @since 1.3.1
	 *
	 * @param object $lang PLL_Language object
	 * @return string
	 */
	public function home_url($lang) {
		return $this->options['hide_default'] && $lang->slug == $this->options['default_lang'] ? $this->home : get_term_link($lang, 'language');
	}
}
