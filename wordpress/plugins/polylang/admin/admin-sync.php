<?php

/*
 * manages copy and synchronization of terms and post metas
 *
 * @since 1.2
 */
class PLL_Admin_Sync {

	/*
	 * constructor
	 *
	 * @since 1.2
	 *
	 * @param object $polylang
	 */
	public function __construct(&$polylang) {
		$this->model = &$polylang->model;
		$this->options = &$polylang->options;

		add_filter('wp_insert_post_parent', array(&$this, 'wp_insert_post_parent'));
		add_action('add_meta_boxes', array(&$this, 'add_meta_boxes'), 10, 2);

		add_action('pll_save_post', array(&$this, 'pll_save_post'), 10, 3);

		if (in_array('taxonomies', $this->options['sync']))
			add_action('pll_save_term', array(&$this, 'pll_save_term'), 10, 3);
	}

	/*
	 * translate post parent if exists when using "Add new" (translation)
	 *
	 * @since 0.6
	 */
	function wp_insert_post_parent($post_parent) {
		return isset($_GET['from_post'], $_GET['new_lang']) && ($id = wp_get_post_parent_id($_GET['from_post'])) && ($parent = $this->model->get_translation('post', $id, $_GET['new_lang'])) ? $parent : $post_parent;
	}

	/*
	 * copy post metas, menu order, comment and ping status when using "Add new" (translation)
	 * formerly used dbx_post_advanced deprecated in WP 3.7
	 *
	 * @since 1.2
	 *
	 * @param string $post_type unused
	 * @param object $post current post object
	 */
	function add_meta_boxes($post_type, $post) {
		if ('post-new.php' == $GLOBALS['pagenow'] && isset($_GET['from_post'], $_GET['new_lang'])) {
			if (!$this->model->is_translated_post_type($post->post_type))
			return;

			// capability check already done in post-new.php
			$this->copy_post_metas($_GET['from_post'], $post->ID, $_GET['new_lang']);

			$from_post = get_post($_GET['from_post']);
			foreach (array('menu_order', 'comment_status', 'ping_status') as $property)
				$post->$property = $from_post->$property;

			if (in_array('sticky_posts', $this->options['sync']) && is_sticky($_GET['from_post']))
				stick_post($post->ID);
		}
	}

	/*
	 * copy or synchronize terms and metas
	 *
	 * @since 0.9
	 *
	 * @param int $from id of the post from which we copy informations
	 * @param int $to id of the post to which we paste informations
	 * @param string $lang language slug
	 * @param bool $sync true if it is synchronization, false if it is a copy, defaults to false
	 */
	protected function copy_post_metas($from, $to, $lang, $sync = false) {
		// copy or synchronize terms
		if (!$sync || in_array('taxonomies', $this->options['sync'])) {
			// FIXME quite a lot of query in foreach
			foreach ($this->model->get_translated_taxonomies() as $tax) {
				$newterms = array();
				$terms = get_the_terms($from, $tax);
				if (is_array($terms)) {
					foreach ($terms as $term) {
						if ($term_id = $this->model->get_translation('term', $term->term_id, $lang))
							$newterms[] = (int) $term_id; // cast is important otherwise we get 'numeric' tags
					}
				}

				// for some reasons, the user may have untranslated terms in the translation. don't forget them.
				if ($sync) {
					$tr_terms = get_the_terms($to, $tax);
					if (is_array($tr_terms)) {
						foreach ($tr_terms as $term) {
							if (!$this->model->get_translation('term', $term->term_id, $this->model->get_post_language($from)))
								$newterms[] = (int) $term->term_id;
						}
					}
				}

				if (!empty($newterms) || $sync)
					wp_set_object_terms($to, $newterms, $tax); // replace terms in translation
			}
		}

		// copy or synchronize post formats
		if (!$sync || in_array('post_format', $this->options['sync']))
			($format = get_post_format($from)) ? set_post_format($to, $format) : set_post_format($to, '');

		// copy or synchronize post metas and allow plugins to do the same
		$metas = get_post_custom($from);

		// get public meta keys (including from translated post in case we just deleted a custom field)
		if (!$sync || in_array('post_meta', $this->options['sync'])) {
			foreach ($keys = array_unique(array_merge(array_keys($metas), array_keys(get_post_custom($to)))) as $k => $meta_key)
				if (is_protected_meta($meta_key))
					unset ($keys[$k]);
		}

		// add page template and featured image
		foreach (array('_wp_page_template', '_thumbnail_id') as $meta)
			if (!$sync || in_array($meta, $this->options['sync']))
				$keys[] = $meta;

		$keys = array_unique(apply_filters('pll_copy_post_metas', empty($keys) ? array() : $keys, $sync));

		// and now copy / synchronize
		foreach ($keys as $key) {
			delete_post_meta($to, $key); // the synchronization process of multiple values custom fields is easier if we delete all metas first
			if (isset($metas[$key])) {
				foreach ($metas[$key] as $value) {
					// important: always maybe_unserialize value coming from get_post_custom. See codex.
					// thanks to goncalveshugo http://wordpress.org/support/topic/plugin-polylang-pll_copy_post_meta
					$value = maybe_unserialize($value);
					// special case for featured images which can be translated
					add_post_meta($to, $key, ($key == '_thumbnail_id' && $tr_value = $this->model->get_translation('post', $value, $lang)) ? $tr_value : $value);
				}
			}
		}
	}

	/*
	 * synchronizes terms and metas in translations
	 *
	 * @since 1.2
	 *
	 * @param int $post_id post id
	 * @param object $post post object
	 * @param array translations post translations
	 */
	public function pll_save_post($post_id, $post, $translations) {
		global $wpdb;

		// prepare properties to synchronize
		foreach (array('comment_status', 'ping_status', 'menu_order', 'post_date') as $property)
			if (in_array($property, $this->options['sync']))
				$postarr[$property] = $post->$property;

		if (in_array('post_date', $this->options['sync']))
			$post_arr['post_date_gmt'] = $post->post_date_gmt;

		// synchronise terms and metas in translations
		foreach ($translations as $lang => $tr_id) {
			if (!$tr_id)
				continue;

			// synchronize terms and metas
			$this->copy_post_metas($post_id, $tr_id, $lang, true);

			// sticky posts
			if (in_array('sticky_posts', $this->options['sync']))
				isset($_REQUEST['sticky']) ? stick_post($tr_id) : unstick_post($tr_id);

			// synchronize comment status, ping status, menu order...
			if (!empty($postarr))
				$wpdb->update($wpdb->posts, $postarr, array('ID' => $tr_id));

			// FIXME: optimize the 2 db update in 1
			// post parent
			// do not udpate the translation parent if the user set a parent with no translation
			if (in_array('post_parent', $this->options['sync'])) {
				$post_parent = ($parent_id = wp_get_post_parent_id($post_id)) ? $this->model->get_translation('post', $parent_id, $lang) : 0;
				if (!($parent_id && !$post_parent))
					$wpdb->update($wpdb->posts, array('post_parent'=> $post_parent), array( 'ID' => $tr_id ));
			}
		}
	}

	/*
	 * synchronize translations of a term in all posts
	 *
	 * @since 1.2
	 *
	 * @param int $term_id term id
	 * @param string $taxonomy taxonomy name of the term
	 * @param array $translations translations of the term
	 */
	public function pll_save_term($term_id, $taxonomy, $translations) {
		// get all posts associated to this term
		$posts = get_posts(array(
			'numberposts' => -1,
			'nopaging'    => true,
			'post_type'   => 'any',
			'post_status' => 'any',
			'fields'      => 'ids',
			'tax_query'   => array(array(
				'taxonomy' => $taxonomy,
				'field'    => 'id',
				'terms'    => array_merge(array($term_id), array_values($translations)),
			))
		));

		// associate translated term to translated post
		// FIXME quite a lot of query in foreach
		foreach ($this->model->get_languages_list() as $language) {
			if ($translated_term = $this->model->get_term($term_id, $language)) {
				foreach ($posts as $post_id) {
					if ($translated_post = $this->model->get_post($post_id, $language))
						wp_set_object_terms($translated_post, $translated_term, $taxonomy, true);
				}
			}
		}

		// synchronize parent in translations
		// calling clean_term_cache *after* this is mandatory otherwise the $taxonomy_children option is not correctly updated
		// but clean_term_cache can be called (efficiently) only one time due to static array which prevents to update the option more than once
		// this is the reason to use the edit_term filter and not edited_term
		// take care that $_POST contains the only valid values for the current term
		// FIXME can I synchronize parent without using $_POST instead?
		if (isset($_POST['term_tr_lang'])) {
			foreach ($_POST['term_tr_lang'] as $lang => $tr_id) {
				if (!$tr_id)
					continue;

				if (isset($_POST['parent']) && $_POST['parent'] != -1) // since WP 3.1
					$term_parent = $this->model->get_translation('term', $_POST['parent'], $lang);

				global $wpdb;
				$wpdb->update($wpdb->term_taxonomy,
					array('parent'=> isset($term_parent) ? $term_parent : 0),
					array('term_taxonomy_id' => get_term($tr_id, $taxonomy)->term_taxonomy_id));
			}
		}
	}
}
