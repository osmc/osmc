<?php

/*
 * some common code for PLL_Admin_Filters_Post and PLL_Admin_Filters_Media
 *
 * @since 1.5
 */
abstract class PLL_Admin_Filters_Post_Base {
	public $links, $model, $pref_lang;

	/*
	 * constructor: setups filters and actions
	 *
	 * @since 1.2
	 *
	 * @param object $polylang
	 */
	public function __construct(&$polylang) {
		$this->links = &$polylang->links;
		$this->model = &$polylang->model;
		$this->pref_lang = &$polylang->pref_lang;
	}

	/*
	 * allows to set a language by default for posts if it has no language yet
	 *
	 * @since 1.5
	 *
	 * @param int $post_id
	 */
	public function set_default_language($post_id) {
		if (!$this->model->get_post_language($post_id)) {
			if (isset($_GET['new_lang']))
				$this->model->set_post_language($post_id, $_GET['new_lang']);

			elseif (($parent_id = wp_get_post_parent_id($post_id)) && $parent_lang = $this->model->get_post_language($parent_id))
				$this->model->set_post_language($post_id, $parent_lang);

			else
				$this->model->set_post_language($post_id, $this->pref_lang);
		}
	}

	/*
	 * save translations from language metabox
	 *
	 * @since 1.5
	 *
	 * @param int $post_id
	 * @param array $arr
	 * @return array
	 */
	protected function save_translations($post_id, $arr) {
		// security check
		// as 'wp_insert_post' can be called from outside WP admin
		check_admin_referer('pll_language', '_pll_nonce');

		// save translations after checking the translated post is in the right language
		foreach ($arr as $lang => $tr_id)
			$translations[$lang] = ($tr_id && $this->model->get_post_language((int) $tr_id)->slug == $lang) ? (int) $tr_id : 0;

		$this->model->save_translations('post', $post_id, $translations);

		// refresh language cache when a static front page has been translated
		if (($pof = get_option('page_on_front')) && in_array($pof, $translations))
			$this->model->clean_languages_cache();

		return $translations;
	}
}
