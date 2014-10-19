<?php

/*
 * adds the language column in posts and terms list tables
 * manages quick edit and bulk edit as well
 *
 * @since 1.2
 */
class PLL_Admin_Filters_Columns {
	public $links, $model, $curlang;

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
		$this->curlang = &$polylang->curlang;

		// add the language and translations columns in 'All Posts', 'All Pages' and 'Media library' panels
		foreach ($this->model->get_translated_post_types() as $type) {
			// use the latest filter late as some plugins purely overwrite what's done by others :(
			// specific case for media
			add_filter('manage_'. ($type == 'attachment' ? 'upload' : 'edit-'. $type) .'_columns', array(&$this, 'add_post_column'), 100);
			add_action('manage_'. ($type == 'attachment' ? 'media' : $type .'_posts') .'_custom_column', array(&$this, 'post_column'), 10, 2);
		}

		// quick edit and bulk edit
		add_filter('quick_edit_custom_box', array(&$this, 'quick_edit_custom_box'), 10, 2);
		add_filter('bulk_edit_custom_box', array(&$this, 'quick_edit_custom_box'), 10, 2);

		// adds the language column in the 'Categories' and 'Post Tags' tables
		foreach ($this->model->get_translated_taxonomies() as $tax) {
			add_filter('manage_edit-'.$tax.'_columns', array(&$this, 'add_term_column'));
			add_filter('manage_'.$tax.'_custom_column', array(&$this, 'term_column'), 10, 3);
		}

	}

	/*
	 * adds languages and translations columns in posts, pages, media, categories and tags tables
	 *
	 * @since 0.8.2
	 *
	 * @param array $columns list of table columns
	 * @param string $before the column before which we want to add our languages
	 * @return array modified list of columns
	 */
	protected function add_column($columns, $before) {
		if ($n = array_search($before, array_keys($columns))) {
			$end = array_slice($columns, $n);
			$columns = array_slice($columns, 0, $n);
		}

		foreach ($this->model->get_languages_list() as $language) {
			// don't add the column for the filtered language
			if (empty($this->curlang) || $language->slug != $this->curlang->slug)
				$columns['language_'.$language->slug] = $language->flag ? $language->flag : esc_html($language->slug);
		}

		return isset($end) ? array_merge($columns, $end) : $columns;
	}

	/*
	 * returns the first language column in the posts, pages and media library tables
	 *
	 * @since 0.9
	 *
	 * @return string first language column name
	 */
	protected function get_first_language_column() {
		foreach ($this->model->get_languages_list() as $language) {
			if (empty($this->curlang) || $language->slug != $this->curlang->slug)
				$columns[] = 'language_'.$language->slug;
		}

		return empty($columns) ? '' : reset($columns);
	}

	/*
	 * adds the language and translations columns (before the comments column) in the posts, pages and media library tables
	 *
	 * @since 0.1
	 *
	 * @param array $columns list of posts table columns
	 * @return array modified list of columns
	 */
	function add_post_column($columns) {
		return $this->add_column($columns, 'comments');
	}

	/*
	 * fills the language and translations columns in the posts, pages and media library tables
	 * take care that when doing ajax inline edit, the post may not be updated in database yet
	 * FIXME if inline edit breaks translations, the rows corresponding to these translations are not updated
	 *
	 * @since 0.1
	 *
	 * @param string $column column name
	 * @param int $post_id
	 */
	public function post_column($column, $post_id) {
		$inline = defined('DOING_AJAX') && $_REQUEST['action'] == 'inline-save' && isset($_POST['inline_lang_choice']);
		$lang = $inline ? $this->model->get_language($_POST['inline_lang_choice']) : $this->model->get_post_language($post_id);

		if (false === strpos($column, 'language_') || !$lang)
			return;

		$language = $this->model->get_language(substr($column, 9));

		// hidden field containing the post language for quick edit
		if ($column == $this->get_first_language_column())
			printf('<div class="hidden" id="lang_%d">%s</div>', esc_attr($post_id), esc_html($lang->slug));

		$post_type_object = get_post_type_object(get_post_type($post_id));

		// link to edit post (or a translation)
		// use $_POST['old_lang'] to detect if the language has been modified in quick edit
		// check capabilities before creating links thanks to Solinx. See http://wordpress.org/support/topic/feature-request-incl-code-check-for-capabilities-in-admin-screens
		if ($id = ($inline && $lang->slug != $_POST['old_lang']) ? ($language->slug == $lang->slug ? $post_id : 0) : $this->model->get_post($post_id, $language)) {
			if (current_user_can($post_type_object->cap->edit_post, $post_id)) {
				printf('<a class="%1$s" title="%2$s" href="%3$s"></a>',
					$id == $post_id ? 'pll_icon_tick' : 'pll_icon_edit',
					esc_attr(get_post($id)->post_title),
					esc_url(get_edit_post_link($id))
				);
			}
			elseif ($id == $post_id) {
				echo '<span class="pll_icon_tick"></span>';
			}
		}
		// link to add a new translation
		elseif (current_user_can($post_type_object->cap->create_posts)) {
			printf('<a class="pll_icon_add" title="%1$s" href="%2$s"></a>',
				__('Add new translation', 'polylang'),
				esc_url($this->links->get_new_post_translation_link($post_id, $language))
			);
		}
	}

	/*
	 * quick edit & bulk edit
	 *
	 * @since 0.9
	 *
	 * @param string $column column name
	 * @param string $type either 'edit-tags' for terms list table or post type for posts list table
	 * @return string unmodified $column
	 */
	public function quick_edit_custom_box($column, $type) {
		if ($column == $this->get_first_language_column()) {

			$elements = $this->model->get_languages_list();
			if (current_filter() == 'bulk_edit_custom_box')
				array_unshift($elements, (object) array('slug' => -1, 'name' => __('&mdash; No Change &mdash;')));

			$dropdown = new PLL_Walker_Dropdown();
			// the hidden field 'old_lang' allows to pass the old language to ajax request
			printf(
				'<fieldset class="inline-edit-col-left">
					<div class="inline-edit-col">
						<input type="hidden" value="" name="old_lang">
						<label class="alignleft">
							<span class="title">%s</span>
							%s
						</label>
					</div>
				</fieldset>',
				__('Language', 'polylang'),
				$dropdown->walk($elements, array('name' => 'inline_lang_choice', 'id' => ''))
			);
		}
		return $column;
	}

	/*
	 * adds the language column (before the posts column) in the 'Categories' or 'Post Tags' table
	 *
	 * @since 0.1
	 *
	 * @param array $columns list of terms table columns
	 * @return array modified list of columns
	 */
	public function add_term_column($columns) {
		return $this->add_column($columns, 'posts');
	}

	/*
	 * fills the language column in the 'Categories' or 'Post Tags' table
	 * FIXME if inline edit breaks translations, the rows corresponding to these translations are not updated
	 *
	 * @since 0.1
	 *
	 * @param string $out
	 * @param string $column column name
	 * @param int term_id
	 */
	public function term_column($out, $column, $term_id) {
		$inline = defined('DOING_AJAX') && $_REQUEST['action'] == 'inline-save-tax' && isset($_POST['inline_lang_choice']);
		if (false === strpos($column, 'language_') || !($lang = $inline ? $this->model->get_language($_POST['inline_lang_choice']) : $this->model->get_term_language($term_id)))
			return $out;

		$post_type = isset($GLOBALS['post_type']) ? $GLOBALS['post_type'] : $_REQUEST['post_type']; // 2nd case for quick edit
		$taxonomy = isset($GLOBALS['taxonomy']) ? $GLOBALS['taxonomy'] : $_REQUEST['taxonomy'];
		$language = $this->model->get_language(substr($column, 9));

		if ($column == $this->get_first_language_column())
			$out = sprintf('<div class="hidden" id="lang_%d">%s</div>', esc_attr($term_id), esc_html($lang->slug));

		$id = ($inline && $lang->slug != $_POST['old_lang']) ? ($language->slug == $lang->slug ? $term_id : 0) : $this->model->get_term($term_id, $language);
		// link to edit term (or a translation)
		if ($id && $term = get_term($id, $taxonomy)) {
			$out .= sprintf('<a class="%1$s" title="%2$s" href="%3$s"></a>',
				$id == $term_id ? 'pll_icon_tick' : 'pll_icon_edit',
				esc_attr($term->name),
				esc_url(get_edit_term_link($id, $taxonomy, $post_type))
			);
		}

		// link to add a new translation
		else {
			$out .= sprintf('<a class="pll_icon_add" title="%1$s" href="%2$s"></a>',
				__('Add new translation', 'polylang'),
				esc_url($this->links->get_new_term_translation_link($term_id, $taxonomy, $post_type, $language))
			);
		}

		return $out;
	}
}
