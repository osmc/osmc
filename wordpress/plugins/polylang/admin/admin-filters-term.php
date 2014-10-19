<?php

/*
 * manages filters and actions related to terms on admin side
 *
 * @since 1.2
 */
class PLL_Admin_Filters_Term {
	public $links, $model, $options, $curlang, $pref_lang;
	protected $pre_term_name; // used to store the term name before creating a slug if needed

	/*
	 * constructor: setups filters and actions
	 *
	 * @param object $polylang
	 */
	public function __construct(&$polylang) {
		$this->links = &$polylang->links;
		$this->model = &$polylang->model;
		$this->options = &$polylang->options;
		$this->curlang = &$polylang->curlang;
		$this->pref_lang = &$polylang->pref_lang;

		foreach ($this->model->get_translated_taxonomies() as $tax) {
			// adds the language field in the 'Categories' and 'Post Tags' panels
			add_action($tax.'_add_form_fields', array(&$this, 'add_term_form'));

			// adds the language field and translations tables in the 'Edit Category' and 'Edit Tag' panels
			add_action($tax.'_edit_form_fields', array(&$this, 'edit_term_form'));

			// adds action related to languages when deleting categories and post tags
			add_action('delete_'.$tax, array(&$this, 'delete_term'));
		}

		// adds actions related to languages when creating or saving categories and post tags
		add_filter('wp_dropdown_cats', array(&$this, 'wp_dropdown_cats'));
		add_filter('pre_insert_term', array(&$this, 'pre_insert_term'), 10, 2);
		add_action('create_term', array(&$this, 'save_term'), 999, 3);
		add_action('edit_term', array(&$this, 'save_term'), 999, 3); // late as it may conflict with other plugins, see http://wordpress.org/support/topic/polylang-and-wordpress-seo-by-yoast
		add_filter('pre_term_name', array(&$this, 'pre_term_name'));
		add_filter('pre_term_slug', array(&$this, 'pre_term_slug'), 10, 2);

		// ajax response for edit term form
		add_action('wp_ajax_term_lang_choice', array(&$this,'term_lang_choice'));
		add_action('wp_ajax_pll_terms_not_translated', array(&$this,'ajax_terms_not_translated'));

		// filters categories and post tags by language
		add_filter('terms_clauses', array(&$this, 'terms_clauses'), 10, 3);

		// backward compatibility WP < 3.7
		version_compare($GLOBALS['wp_version'], '3.7', '<') ?
			add_action('wp_ajax_polylang-ajax-tag-search', array(&$this,'ajax_tag_search')) :
			add_action('wp_ajax_polylang-ajax-tag-search', 'wp_ajax_ajax_tag_search'); // take profit of new filter, cache...

		add_filter('option_default_category', array(&$this, 'option_default_category'));
	}

	/*
	 * adds the language field in the 'Categories' and 'Post Tags' panels
	 *
	 * @since 0.1
	 */
	public function add_term_form() {
		$taxonomy = $_GET['taxonomy'];
		$post_type = isset($GLOBALS['post_type']) ? $GLOBALS['post_type'] : $_REQUEST['post_type'];
		$lang = isset($_GET['new_lang']) ? $this->model->get_language($_GET['new_lang']) : $this->pref_lang;
		$dropdown = new PLL_Walker_Dropdown();

		wp_nonce_field('pll_language', '_pll_nonce');

		printf('
			<div class="form-field">
				<label for="term_lang_choice">%s</label>
				%s
				<p>%s</p>
			</div>',
			__('Language', 'polylang'),
			$dropdown->walk($this->model->get_languages_list(), array('name' => 'term_lang_choice', 'value' => 'term_id', 'selected' => $lang ? $lang->term_id : '')),
			__('Sets the language', 'polylang')
		);

		if (!empty($_GET['from_tag']))
			printf('<input type="hidden" name="from_tag" value="%d" />', $_GET['from_tag']);

		// adds translation fields
		echo '<div id="term-translations" class="form-field">';
		if ($lang)
			include(PLL_ADMIN_INC.'/view-translations-term.php');
		echo '</div>'."\n";
	}

	/*
	 * adds the language field and translations tables in the 'Edit Category' and 'Edit Tag' panels
	 *
	 * @since 0.1
	 */
	public function edit_term_form($tag) {
		$term_id = $tag->term_id;
		$lang = $this->model->get_term_language($term_id);
		$taxonomy = $tag->taxonomy;
		$post_type = isset($GLOBALS['post_type']) ? $GLOBALS['post_type'] : $_REQUEST['post_type'];
		$dropdown = new PLL_Walker_Dropdown();

		wp_nonce_field('pll_language', '_pll_nonce');

		printf('
			<tr class="form-field">
				<th scope="row">
					<label for="term_lang_choice">%s</label>
				</th>
				<td>
					%s<br />
					<span class="description">%s</span>
				</td>
			</tr>',
			__('Language', 'polylang'),
			$dropdown->walk($this->model->get_languages_list(), array('name' => 'term_lang_choice', 'value' => 'term_id', 'selected' => $lang ? $lang->term_id : '')),
			__('Sets the language', 'polylang')
		);

		echo '<tr id="term-translations" class="form-field">';
		if ($lang)
			include(PLL_ADMIN_INC.'/view-translations-term.php');
		echo '</tr>'."\n";
	}

	/*
	 * translates term parent if exists when using "Add new" (translation)
	 *
	 * @since 0.7
	 *
	 * @param string html markup for dropdown list of categories
	 * @return string modified html
	 */
	public function wp_dropdown_cats($output) {
		if (isset($_GET['taxonomy'], $_GET['from_tag'], $_GET['new_lang']) && $id = get_term($_GET['from_tag'], $_GET['taxonomy'])->parent) {
			if ($parent = $this->model->get_translation('term', $id, $_GET['new_lang']))
				return str_replace('"'.$parent.'"', '"'.$parent.'" selected="selected"', $output);
		}
		return $output;
	}

	/*
	 * prevents duplicating a term translation
	 *
	 * @since 1.3
	 *
	 * @param string $term The term to add or update.
	 * @param string $taxonomy The taxonomy to which to add the term
	 *
	 * @return object|string WP_Error object if the translation already exits, unmodified $term otherwise
	 */
	public function pre_insert_term($term, $taxonomy) {
		if (isset($_POST['action'], $_POST['from_tag'], $_POST['term_lang_choice']) && 'add-tag' == $_POST['action'] && $this->model->get_translation('term', $_POST['from_tag'], $_POST['term_lang_choice'])) {
			$from_term = get_term($_POST['from_tag'], $taxonomy);
			return new WP_Error('term_translation_exists', sprintf(
				__('A translation does already exist for %s', 'polylang'),
				$from_term->name
			));
		}
		return $term;
	}

	/*
	 * allows to set a language by default for terms if it has no language yet
	 *
	 * @since 1.5.4
	 *
	 * @param int $term_id
	 * @param string $taxonomy
	 */
	protected function set_default_language($term_id, $taxonomy) {
		if (!$this->model->get_term_language($term_id)) {
			// sets language from term parent if exists thanks to Scott Kingsley Clark
			if (($term = get_term($term_id, $taxonomy)) && !empty($term->parent) && $parent_lang = $this->model->get_term_language($term->parent))
				$this->model->set_term_language($term_id, $parent_lang);

			else
				$this->model->set_term_language($term_id, $this->pref_lang);
		}
	}

	/*
	 * saves language
	 *
	 * @since 1.5
	 *
	 * @param int $term_id
	 * @param string $taxonomy
	 */
	protected function save_language($term_id, $taxonomy) {
		// security checks are necessary to accept language modifications
		// as 'wp_update_term' can be called from outside WP admin

		// edit tags
		if (isset($_POST['term_lang_choice'])) {
			if ('add-' . $taxonomy == $_POST['action'])
				check_ajax_referer($_POST['action'], '_ajax_nonce-add-' . $taxonomy); // category metabox

			else
				check_admin_referer('pll_language', '_pll_nonce'); // edit tags or tags metabox

			$this->model->set_term_language($term_id, $_POST['term_lang_choice']);
		}

		// quick edit
		elseif (isset($_POST['inline_lang_choice'])) {
			check_ajax_referer('taxinlineeditnonce', '_inline_edit');

			if (isset($_POST['inline-save-tax']) && $this->model->get_term_language($term_id)->slug != $_POST['inline_lang_choice'])
				$this->model->delete_translation('term', $term_id);

			$this->model->set_term_language($term_id, $_POST['inline_lang_choice']);
		}

		// edit post
		elseif (isset($_POST['post_lang_choice'])) {// FIXME should be useless now
			check_admin_referer('pll_language', '_pll_nonce');
			$this->model->set_term_language($term_id, $_POST['post_lang_choice']);
		}

		else
			$this->set_default_language($term_id, $taxonomy);
	}

	/*
	 * save translations from our form
	 *
	 * @since 1.5
	 *
	 * @param int $term_id
	 * @return array
	 */
	protected function save_translations($term_id) {
		// security check
		// as 'wp_update_term' can be called from outside WP admin
		check_admin_referer('pll_language', '_pll_nonce');

		// save translations after checking the translated term is in the right language (as well as cast id to int)
		foreach ($_POST['term_tr_lang'] as $lang => $tr_id) {
			$tr_lang = $this->model->get_term_language((int) $tr_id);
			$translations[$lang] = $tr_lang && $tr_lang->slug == $lang ? (int) $tr_id : 0;
		}

		$this->model->save_translations('term', $term_id, $translations);

		return $translations;
	}

	/*
	 * called when a category or post tag is created or edited
	 * saves language and translations
	 *
	 * @since 0.1
	 *
	 * @param int $term_id
	 * @param int $tt_id term taxononomy id
	 * @param string $taxonomy
	 */
	public function save_term($term_id, $tt_id, $taxonomy) {
		// does nothing except on taxonomies which are filterable
		if (!$this->model->is_translated_taxonomy($taxonomy))
			return;

		// capability check
		// as 'wp_update_term' can be called from outside WP admin
		// 2nd test for creating tags when creating / editing a post
		$tax = get_taxonomy($taxonomy);
		if (current_user_can($tax->cap->edit_terms) || (isset($_POST['tax_input'][$taxonomy]) && current_user_can($tax->cap->assign_terms))) {
			$this->save_language($term_id, $taxonomy);

			if (isset($_POST['term_tr_lang']))
				$translations = $this->save_translations($term_id);

			do_action('pll_save_term', $term_id, $taxonomy, empty($translations) ? $this->model->get_translations('term', $term_id) : $translations);
		}

		// attempts to set a default language even if no capability
		else
			$this->set_default_language($term_id, $taxonomy);
	}

	/*
	 * stores the term name for use in pre_term_slug
	 *
	 * @since 0.9.5
	 *
	 * @param string $name term name
	 * @return string unmodified term name
	 */
	public function pre_term_name($name) {
		return $this->pre_term_name = $name;
	}

	/*
	 * creates the term slug in case the term already exists in another language
	 *
	 * @since 0.9.5
	 *
	 * @param string $slug
	 * @param string $taxonomy
	 * @return string
	 */
	public function pre_term_slug($slug, $taxonomy) {
		$name = sanitize_title($this->pre_term_name);

		// if the new term has the same name as a language, we *need* to differentiate the term
		// see http://core.trac.wordpress.org/ticket/23199
		if (term_exists($name, 'language') && !term_exists($name, $taxonomy) && (!$slug || $slug == $name))
			$slug = $name . '-' . $taxonomy; // a convenient slug which may be modified later by the user

		// if the term already exists in another language
		if (!$slug && $this->model->is_translated_taxonomy($taxonomy) && term_exists($name, $taxonomy)) {
			if (isset($_POST['term_lang_choice']))
				$slug = $name . '-' . $this->model->get_language($_POST['term_lang_choice'])->slug;

			elseif (isset($_POST['inline_lang_choice']))
				$slug = $name . '-' . $this->model->get_language($_POST['inline_lang_choice'])->slug;
		}

		return $slug;
	}

	/*
	 * called when a category or post tag is deleted
	 * deletes language and translations
	 *
	 * @since 0.1
	 *
	 * @param int $term_id
	 */
	public function delete_term($term_id) {
		$this->model->delete_translation('term', $term_id);
		$this->model->delete_term_language($term_id);
	}

	/*
	 * ajax response for edit term form
	 *
	 * @since 0.2
	 */
	public function term_lang_choice() {
		check_ajax_referer('pll_language', '_pll_nonce');

		$lang = $this->model->get_language($_POST['lang']);
		$term_id = isset($_POST['term_id']) ? $_POST['term_id'] : null;
		$taxonomy = $_POST['taxonomy'];
		$post_type = $_POST['post_type'];

		ob_start();
		if ($lang)
			include(PLL_ADMIN_INC.'/view-translations-term.php');
		$x = new WP_Ajax_Response(array('what' => 'translations', 'data' => ob_get_contents()));
		ob_end_clean();

		// parent dropdown list (only for hierarchical taxonomies)
		// $args copied from edit_tags.php except echo
		if (is_taxonomy_hierarchical($taxonomy)) {
			$args = array(
				'hide_empty' => 0,
				'hide_if_empty' => false,
				'taxonomy' => $taxonomy,
				'name' => 'parent',
				'orderby' => 'name',
				'hierarchical' => true,
				'show_option_none' => __('None'),
				'echo' => 0,
			);
			$x->Add(array('what' => 'parent', 'data' => wp_dropdown_categories($args)));
		}

		// tag cloud
		// tests copied from edit_tags.php
		else {
			$tax = get_taxonomy($taxonomy);
		 	if (!is_null($tax->labels->popular_items)) {
				$args = array('taxonomy' => $taxonomy, 'echo' => false);
				if (current_user_can($tax->cap->edit_terms))
					$args = array_merge($args, array('link' => 'edit'));

				if ($tag_cloud = wp_tag_cloud($args))
					$x->Add(array('what' => 'tag_cloud', 'data' => '<h3>'.$tax->labels->popular_items.'</h3>'.$tag_cloud));
			}
		}

		$x->send();
	}

	/*
	 * ajax response for input in translation autocomplete input box
	 *
	 * @since 1.5
	 */
	public function ajax_terms_not_translated() {
		check_ajax_referer('pll_language', '_pll_nonce');

		$return = array();

		// it is more efficient to use one common query for all languages as soon as there are more than 2
		// pll_get_terms_not_translated arg to identify this query in terms_clauses filter
		foreach (get_terms($_REQUEST['taxonomy'], 'hide_empty=0&pll_get_terms_not_translated=1&name__like=' . $_REQUEST['term']) as $term) {
			$lang = $this->model->get_term_language($term->term_id);

			if ($lang && $lang->slug == $_REQUEST['translation_language'] && !$this->model->get_translation('term', $term->term_id, $_REQUEST['term_language']))
				$return[] = array(
					'id' => $term->term_id,
					'value' => $term->name,
					'link' => $this->edit_translation_link($term->term_id, $_REQUEST['taxonomy'], $_REQUEST['post_type'])
				);
		}

		// add current translation in list
		// not in add term for as term_id is not set
		if ('undefined' !== $_REQUEST['term_id'] && $term_id = $this->model->get_translation('term', $_REQUEST['term_id'], $_REQUEST['translation_language'])) {
			$term = get_term($term_id, $_REQUEST['taxonomy']);
			array_unshift($return, array(
				'id' => $term_id,
				'value' => $term->name,
				'link' => $this->edit_translation_link($term->term_id, $_REQUEST['taxonomy'], $_REQUEST['post_type'])
			));
		}

		wp_die(json_encode($return));
	}

	/*
	 * filters categories and post tags by language when needed on admin side
	 *
	 * @since 0.5
	 *
	 * @param array $clauses list of sql clauses
	 * @param array $taxonomies list of taxonomies
	 * @param array $args get_terms arguments
	 * @return array modified sql clauses
	 */
	public function terms_clauses($clauses, $taxonomies, $args) {
		// does nothing except on taxonomies which are filterable
		foreach ($taxonomies as $tax)
			if (!$this->model->is_translated_taxonomy($tax))
				return $clauses;

		if (function_exists('get_current_screen'))
			$screen = get_current_screen(); // since WP 3.1, may not be available the first time(s) get_terms is called

		// don't filter nav menus on nav menus screen
		if (isset($screen) && 'nav-menus' == $screen->base && in_array('nav_menu', $taxonomies))
			return $clauses;

		// if get_terms is queried with a 'lang' parameter
		if (!empty($args['lang']))
			return $this->model->terms_clauses($clauses, $args['lang']);

		// does nothing in Languages and dasboard admin panels
		if (isset($screen) && in_array($screen->base, array('toplevel_page_mlang', 'dashboard')))
			return $clauses;

		// do not filter 'get_terms_not_translated'
		if (!empty($args['pll_get_terms_not_translated']))
			return $clauses;

		// admin language filter for ajax paginate_links in taxonomies metabox in nav menus panel
		if (!empty($_POST['action']) && !empty($this->curlang) && 'menu-get-metabox' == $_POST['action'])
			return $this->model->terms_clauses($clauses, $this->curlang);

		// The only ajax response I want to deal with is when changing the language in post metabox
		if (isset($_POST['action']) && !in_array($_POST['action'], array('post_lang_choice', 'term_lang_choice', 'get-tagcloud')))
			return $clauses;

		// I only want to filter the parent dropdown list when editing a term in a hierarchical taxonomy
		if (isset($_POST['action']) && $_POST['action'] == 'term_lang_choice' && !(isset($args['class']) || isset($args['unit'])))
			return $clauses;

		// ajax response for changing the language in the post metabox (or in the edit-tags panels)
		if (isset($_POST['lang']))
			$lang = $this->model->get_language($_POST['lang']);

		// ajax tag search since WP 3.7
		elseif (!empty($_GET['lang']) && isset($_GET['action']) && 'polylang-ajax-tag-search' == $_GET['action'])
			$lang = $this->model->get_language($_GET['lang']);

		// the post (or term) is created with the 'add new' (translation) link
		// test of $args['page'] to avoid filtering the terms list table in edit-tags panel
		elseif (!empty($_GET['new_lang']) && empty($args['page']))
			$lang = $this->model->get_language($_GET['new_lang']);

		// FIXME can we simplify how we deal with the admin language filter?
		// the language filter selection has just changed
		// test $screen->base to avoid interference between the language filter and the post language selection and the category parent dropdown list
		elseif (!empty($_GET['lang']) && !(isset($screen) && in_array($screen->base, array('post', 'edit-tags')))) {
			if ($_GET['lang'] != 'all')
				$lang = $this->model->get_language($_GET['lang']);
			elseif ($screen->base == 'edit-tags' && isset($args['class']))
				$lang = $this->pref_lang; // parent dropdown
		}

		// again the language filter
		elseif (!empty($this->curlang) && (isset($screen) && $screen->base != 'post' && !($screen->base == 'edit-tags' && isset($args['class'])))) // don't apply to post edit and the category parent dropdown list
		 	$lang = $this->curlang;

		elseif (isset($_GET['post']))
			$lang = $this->model->get_post_language($_GET['post']);

		// for the parent dropdown list in edit term
		elseif (isset($_GET['tag_ID']))
			$lang = $this->model->get_term_language($_GET['tag_ID']);

		// when a new category is created in the edit post panel
		elseif (isset($_POST['term_lang_choice']))
			$lang = $this->model->get_language($_POST['term_lang_choice']);

		// for a new post (or the parent dropdown list of a new term)
		elseif (isset($screen) && ($screen->base == 'post' || ($screen->base == 'edit-tags' && isset($args['class']))))
			$lang = $this->pref_lang;

		// adds our clauses to filter by current language
		return !empty($lang) ? $this->model->terms_clauses($clauses, $lang) : $clauses;
	}

	/*
	 * replaces ajax tag search of WP to filter tags by language
	 * backward compatibility WP < 3.7
	 * see http://core.trac.wordpress.org/ticket/25231
	 *
	 * @since 0.7
	 */
	public function ajax_tag_search() {
		global $wpdb;

		if ( isset( $_GET['tax'] ) ) {
			$taxonomy = sanitize_key( $_GET['tax'] );
			$tax = get_taxonomy( $taxonomy );
			if ( ! $tax )
				die( '0' );
			if ( ! current_user_can( $tax->cap->assign_terms ) )
				die( '-1' );
		} else {
			die('0');
		}

		$s = stripslashes( $_GET['q'] );

		if ( false !== strpos( $s, ',' ) ) {
			$s = explode( ',', $s );
			$s = $s[count( $s ) - 1];
		}
		$s = trim( $s );
		if ( strlen( $s ) < 2 )
			die; // require 2 chars for matching

		$lang = $this->model->get_language($_GET['lang']);

		$results = $wpdb->get_col( $wpdb->prepare(
			"SELECT t.name FROM $wpdb->term_taxonomy AS tt
			INNER JOIN $wpdb->terms AS t ON tt.term_id = t.term_id
			INNER JOIN $wpdb->term_relationships AS pll_tr ON pll_tr.object_id = t.term_id
			WHERE tt.taxonomy = %s AND t.name LIKE (%s) AND pll_tr.term_taxonomy_id = %d",
			$taxonomy, '%' . like_escape( $s ) . '%', $lang->tl_term_taxonomy_id ) );

		echo join( $results, "\n" );
		die;
	}

	/*
	 * hack to avoid displaying delete link for the default category in all languages
	 * also returns the default category in the right language when called from wp_delete_term
	 *
	 * @since 1.2
	 *
	 * @param int $value
	 * @return int
	 */
	public function option_default_category($value) {
		$traces = debug_backtrace();

		if (isset($traces[4])) {
			if (in_array($traces[4]['function'], array('column_cb', 'column_name')) && in_array($traces[4]['args'][0]->term_id, $this->model->get_translations('term', $value)))
				return $traces[4]['args'][0]->term_id;

			if ('wp_delete_term' == $traces[4]['function'])
				return $this->model->get_term($value, $this->model->get_term_language($traces[4]['args'][0]));
		}
		return $value;
	}

	/*
	 * returns html markup for a translation link
	 *
	 * @since 1.4
	 *
	 * @param object $term_id translation term id
	 * @param string $taxonomy
	 * @param string $post_type
	 * @return string
	 */
	public function edit_translation_link($term_id, $taxonomy, $post_type) {
		return sprintf(
			'<a href="%1$s" class="pll_icon_edit title="%2$s"></a></td>',
			esc_url(get_edit_term_link($term_id, $taxonomy, $post_type)),
			__('Edit','polylang')
		);
	}
}
