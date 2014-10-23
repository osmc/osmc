<?php

/*
 * links model for use when using one domain per language
 * for example mysite.com/sth and mysite.fr/qqch
 * implements the "links_model interface"
 *
 * @since 1.2
 */
class PLL_Links_Domain extends PLL_Links_Model {

	/*
	 * adds the language code in url
	 * links_model interface
	 *
	 * @since 1.2
	 *
	 * @param string $url url to modify
	 * @param object $lang language
	 * @return string modified url
	 */
	public function add_language_to_link($url, $lang) {
		if (!empty($lang) && !empty($this->options['domains'][$lang->slug]))
			$url = str_replace($this->home, $this->options['domains'][$lang->slug], $url);
		return $url;
	}

	/*
	 * returns the url without language code
	 * links_model interface
	 *
	 * @since 1.2
	 *
	 * @param string $url url to modify
	 * @return string modified url
	 */
	public function remove_language_from_link($url) {
		if (!empty($this->options['domains']))
			$url = str_replace((is_ssl() ? 'https://' : 'http://') . parse_url($url, PHP_URL_HOST), $this->home, $url);
		return $url;
	}

	/*
	 * returns the language based on language code in url
	 * links_model interface
	 *
	 * @since 1.2
	 *
	 * @return string language slug
	 */
	public function get_language_from_url() {
		return ($lang = array_search( (is_ssl() ? 'https://' : 'http://') . $_SERVER['HTTP_HOST'] . parse_url($this->home, PHP_URL_PATH), $this->options['domains'] ) ) ? $lang : $this->options['default_lang'];
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
	function home_url($lang) {
		return empty($this->options['domains'][$lang->slug]) ? $this->home : $this->options['domains'][$lang->slug];
	}

	/*
	 * get hosts managed on the website
	 *
	 * @since 1.5
	 *
	 * @return array list of hosts
	 */
	public function get_hosts() {
		return array_map(create_function('$v', 'return parse_url($v, PHP_URL_HOST);'), $this->options['domains']);
	}
}
