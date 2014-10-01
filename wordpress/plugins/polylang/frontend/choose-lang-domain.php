<?php

/*
 * Choose the language when the language is managed by different domains
 *
 * @since 1.5
 */
class PLL_Choose_Lang_Domain extends PLL_Choose_Lang_Url {

	/*
	 * don't set any language cookie
	 *
	 * @since 1.5
	 */
	protected function maybe_setcookie() {}

	/*
	 * don't redirect according to browser preferences
	 *
	 * @since 1.5
	 */
	public function get_preferred_language() {
		return $this->model->get_language($this->links_model->get_language_from_url());
	}

	/*
	 * Adds query vars to query for home pages in all languages
	 *
	 * @since 1.5
	 */
	public function home_requested() {
		$this->set_home_query_var();
	}
}
