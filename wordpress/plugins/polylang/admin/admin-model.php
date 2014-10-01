<?php

/*
 * extends the PLL_Model class with methods needed only in Polylang settings pages
 *
 * @since 1.2
 */
class PLL_Admin_Model extends PLL_Model {

	/*
	 * adds a new language
	 * creates a default category for this language
	 *
	 * list of arguments that $args must contain:
	 * name           -> language name (used only for display)
	 * slug           -> language code (ideally 2-letters ISO 639-1 language code
	 * locale         -> WordPress locale. If something wrong is used for the locale, the .mo files will not be loaded...
	 * rtl            -> 1 if rtl language, 0 otherwise
	 * term_group     -> language order when displayed
	 *
	 * optional arguments that $args can contain:
	 * no_default_cat -> if set, no default category will be created for this language
	 *
	 * @since 1.2
	 *
	 * @param array $args
	 * @return bool true if success / false if failed
	 */
	public function add_language($args) {
		if (!$this->validate_lang($args))
			return false;

		// first the language taxonomy
		$description = serialize(array('locale' => $args['locale'], 'rtl' => $args['rtl']));
		$r = wp_insert_term($args['name'], 'language', array('slug' => $args['slug'], 'description' => $description));
		if (is_wp_error($r)) {
			// avoid an ugly fatal error if something went wrong (reported once in the forum)
			add_settings_error('general', 'pll_add_language', __('Impossible to add the language.', 'polylang'));
			return false;
		}
		wp_update_term((int) $r['term_id'], 'language', array('term_group' => $args['term_group'])); // can't set the term group directly in wp_insert_term

		// the term_language taxonomy
		// don't want shared terms so use a different slug
		wp_insert_term($args['name'], 'term_language', array('slug' => 'pll_' . $args['slug']));

		$this->clean_languages_cache(); // udpate the languages list now !

		if (!isset($this->options['default_lang'])) {
			// if this is the first language created, set it as default language
			$this->options['default_lang'] = $args['slug'];
			update_option('polylang', $this->options);

			// and assign default language to default category
			$this->set_term_language((int) get_option('default_category'), (int) $r['term_id']);
		}
		elseif (empty($args['no_default_cat']))
			$this->create_default_category($args['slug']);

		flush_rewrite_rules(); // refresh rewrite rules

		add_settings_error('general', 'pll_languages_created', __('Language added.', 'polylang'), 'updated');
		return true;
	}

	/*
	 * create a default category for a language
	 *
	 * @since 1.2
	 *
	 * @param object|string|int $lang language
	 */
	public function create_default_category($lang) {
		$lang = $this->get_language($lang);

		// create a new category
		$cat_name = __('Uncategorized');
		$cat_slug = sanitize_title($cat_name . '-' . $lang->slug);
		$cat = wp_insert_term($cat_name, 'category', array('slug' => $cat_slug));

		// check that the category was not previously created (in case the language was deleted and recreated)
		$cat = isset($cat->error_data['term_exists']) ? $cat->error_data['term_exists'] : $cat['term_id'];

		// set language
		$this->set_term_language((int) $cat, $lang);

		// this is a translation of the default category
		$default = (int) get_option('default_category');
		$translations = $this->get_translations('term', $default);
		if (empty($translations)) {
			if ($lg = $this->get_term_language($default))
				$translations[$lg->slug] = $default;
			else
				$translations = array();
		}

		$this->save_translations('term', (int) $cat, $translations);
	}

	/*
	 * delete a language
	 *
	 * @since 1.2
	 *
	 * @param int $lang_id language term_id
	 */
	public function delete_language($lang_id) {
		$lang = $this->get_language((int) $lang_id);
		$default_cats = $this->get_translations('term', get_option('default_category'));

		// delete the translations
		$this->update_translations($lang->slug);

		// delete language option in widgets
		foreach ($GLOBALS['wp_registered_widgets'] as $widget) {
			if (!empty($widget['callback'][0]) && !empty($widget['params'][0]['number'])) {
				$obj = $widget['callback'][0];
				$number = $widget['params'][0]['number'];
				if (is_object($obj) && method_exists($obj, 'get_settings') && method_exists($obj, 'save_settings')) {
					$settings = $obj->get_settings();
					if (isset($settings[$number]['pll_lang']) && $settings[$number]['pll_lang'] == $lang->slug) {
						unset($settings[$number]['pll_lang']);
						$obj->save_settings($settings);
					}
				}
			}
		}


		// delete menus locations
		if (!empty($this->options['nav_menus'])) {
			foreach ($this->options['nav_menus'] as $theme => $locations) {
				foreach ($locations as $location => $languages) {
					unset($this->options['nav_menus'][$theme][$location][$lang->slug]);
				}
			}
		}

		// delete users options
		foreach (get_users(array('fields' => 'ID')) as $user_id) {
			delete_user_meta($user_id, 'user_lang', $lang->locale);
			delete_user_meta($user_id, 'pll_filter_content', $lang->slug);
			delete_user_meta($user_id, 'description_'.$lang->slug);
		}

		// delete the string translations
		$post = get_page_by_title('polylang_mo_' . $lang->term_id, OBJECT, 'polylang_mo');
		if (!empty($post))
			wp_delete_post($post->ID);

		// delete domain
		unset($this->options['domains'][$lang->slug]);

		// delete the language itself
		wp_delete_term($lang->term_id, 'language');
		wp_delete_term($lang->tl_term_id, 'term_language');

		// update languages list
		$this->clean_languages_cache();

		// oops ! we deleted the default language...
		if ($this->options['default_lang'] == $lang->slug)	{
			if ($slugs = $this->get_languages_list(array('fields' => 'slug'))) {
				$this->options['default_lang'] = $slug = reset($slugs); // arbitrary choice...

				// the default category should be in the default language
				if (isset($default_cats[$slug]))
					update_option('default_category', $default_cats[$slug]);
			}
			else
				unset($this->options['default_lang']);
		}

		update_option('polylang', $this->options);
		flush_rewrite_rules(); // refresh rewrite rules
		add_settings_error('general', 'pll_languages_deleted', __('Language deleted.', 'polylang'), 'updated');
	}

	/*
	 * update language properties
	 *
	 * list of arguments that $args must contain:
	 * lang_id    -> term_id of the language to modify
	 * name       -> language name (used only for display)
	 * slug       -> language code (ideally 2-letters ISO 639-1 language code
	 * locale     -> WordPress locale. If something wrong is used for the locale, the .mo files will not be loaded...
	 * rtl        -> 1 if rtl language, 0 otherwise
	 * term_group -> language order when displayed
	 *
	 * @since 1.2
	 *
	 * @param array $args
	 * @return bool true if success / false if failed
	 */
	public function update_language($args) {
		$lang = $this->get_language((int) $args['lang_id']);
		if (!$this->validate_lang($args, $lang))
			return false;

		// Update links to this language in posts and terms in case the slug has been modified
		$slug = $args['slug'];
		$old_slug = $lang->slug;

		// FIXME should do this in an action 'edit_term' to prevent translations to break when sharing a term with nav_menu?
		if ($old_slug != $slug) {
			// update the language slug in translations
			$this->update_translations($old_slug, $slug);

			// update language option in widgets
			foreach ($GLOBALS['wp_registered_widgets'] as $widget) {
				if (!empty($widget['callback'][0]) && !empty($widget['params'][0]['number'])) {
					$obj = $widget['callback'][0];
					$number = $widget['params'][0]['number'];
					if (is_object($obj) && method_exists($obj, 'get_settings') && method_exists($obj, 'save_settings')) {
						$settings = $obj->get_settings();
						if (isset($settings[$number]['pll_lang']) && $settings[$number]['pll_lang'] == $old_slug) {
							$settings[$number]['pll_lang'] = $slug;
							$obj->save_settings($settings);
						}
					}
				}
			}

			// update menus locations
			if (!empty($this->options['nav_menus'])) {
				foreach ($this->options['nav_menus'] as $theme => $locations) {
					foreach ($locations as $location => $languages) {
						if (!empty($this->options['nav_menus'][$theme][$location][$old_slug])) {
							$this->options['nav_menus'][$theme][$location][$slug] = $this->options['nav_menus'][$theme][$location][$old_slug];
							unset($this->options['nav_menus'][$theme][$location][$old_slug]);
						}

					}
				}
			}

			// update domains
			if (!empty($this->options['domains'][$old_slug])) {
				$this->options['domains'][$slug] = $this->options['domains'][$old_slug];
				unset($this->options['domains'][$slug]);
			}

			// update the default language option if necessary
			if ($this->options['default_lang'] == $old_slug)
				$this->options['default_lang'] = $slug;
		}

		update_option('polylang', $this->options);

		// and finally update the language itself
		$description = serialize(array('locale' => $args['locale'], 'rtl' => $args['rtl']));
		wp_update_term((int) $lang->term_id, 'language', array('slug' => $slug, 'name' => $args['name'], 'description' => $description, 'term_group' => $args['term_group']));
		wp_update_term((int) $lang->tl_term_id, 'term_language', array('slug' => 'pll_' . $slug, 'name' =>  $args['name']));

		$this->clean_languages_cache();
		flush_rewrite_rules(); // refresh rewrite rules
		add_settings_error('general', 'pll_languages_updated', __('Language updated.', 'polylang'), 'updated');
		return true;
	}

	/*
	 * validates data entered when creating or updating a language
	 *
	 * @since 0.4
	 *
	 * @param array $args
	 * @param object $lang optional the language currently updated, the language is created if not set
	 * @return bool true if success / false if failed
	 * @see PLL_Admin_Model::add_language
	 */
	protected function validate_lang($args, $lang = null) {
		// validate locale
		if ( !preg_match('#^[A-Za-z_]+$#', $args['locale']))
			add_settings_error('general', 'pll_invalid_locale', __('Enter a valid WordPress locale', 'polylang'));

		// validate slug characters
		if (!preg_match('#^[a-z_-]+$#', $args['slug']))
			add_settings_error('general', 'pll_invalid_slug', __('The language code contains invalid characters', 'polylang'));

		// validate slug is unique
		if ($this->get_language($args['slug']) && ($lang === null || (isset($lang) && $lang->slug != $args['slug'])))
			add_settings_error('general', 'pll_non_unique_slug', __('The language code must be unique', 'polylang'));

		// validate name
		if ($args['name'] == '')
			add_settings_error('general', 'pll_invalid_name',  __('The language must have a name', 'polylang'));

		return get_settings_errors() ? false : true;
	}

	/*
	 * used to set the language of posts or terms in mass
	 *
	 * @since 1.2
	 *
	 * @param string $type either 'post' or 'term'
	 * @param array $ids array of post ids or term ids
	 * @param object|string $lang object or slug
	 */
	public function set_language_in_mass($type, $ids, $lang) {
		global $wpdb;

		$lang = $this->get_language($lang);
		$tt_id = 'term' == $type ? $lang->tl_term_taxonomy_id : $lang->term_taxonomy_id;

		foreach ($ids as $id)
			$values[] = $wpdb->prepare("(%d, %d)", $id, $tt_id);

		if (!empty($values)) {
			$values = array_unique($values);
			$wpdb->query("INSERT INTO $wpdb->term_relationships (object_id, term_taxonomy_id) VALUES " . implode(',', $values));
			$lang->update_count(); // updating term count is mandatory (thanks to AndyDeGroo)
		}
	}

	/*
	 * returns unstranslated posts and terms ids (used in settings)
	 *
	 * @since 0.9
	 *
	 * @return array array made of an array of post ids and an array of term ids
	 */
	public function get_objects_with_no_lang() {
		$posts = get_posts(array(
			'numberposts' => -1,
			'nopaging'    => true,
			'post_type'   => $this->get_translated_post_types(),
			'post_status' => 'any',
			'fields'      => 'ids',
			'tax_query'   => array(array(
				'taxonomy' => 'language',
				'terms'    => $this->get_languages_list(array('fields' => 'term_id')),
				'operator' => 'NOT IN'
			))
		));

		$terms = get_terms($this->get_translated_taxonomies(), array('get'=>'all', 'fields'=>'ids'));
		$groups = $this->get_languages_list(array('fields' => 'tl_term_id'));
		$tr_terms = get_objects_in_term($groups, 'term_language');
		$terms = array_unique(array_diff($terms, $tr_terms)); // array_unique to avoid duplicates if a term is in more than one taxonomy

		return apply_filters('pll_get_objects_with_no_lang', empty($posts) && empty($terms) ? false : array('posts' => $posts, 'terms' => $terms));
	}

	/*
	 * used to delete translations or update the translations when a language slug has been modified in settings
	 *
	 * @since 0.5
	 *
	 * @param string $old_slug the old language slug
	 * @param string $new_slug optional, the new language slug, if not set it means the correspondant has been deleted
	 */
	public function update_translations($old_slug, $new_slug = '') {
		global $wpdb;

		$terms = get_terms(array('post_translations', 'term_translations'));

		foreach ($terms as $term) {
			$tr = unserialize($term->description);
			if (!empty($tr[$old_slug])) {
				if ($new_slug)
					$tr[$new_slug] = $tr[$old_slug]; // suppress this for delete
				else {
					$dr['id'][] = (int) $tr[$old_slug];
					$dr['tt'][] = (int) $term->term_taxonomy_id;
				}
				unset($tr[$old_slug]);

				if (empty($tr) || 1 == count($tr)) {
					$dt['t'][] = (int) $term->term_id;
					$dt['tt'][] = (int) $term->term_taxonomy_id;
				}
				else {
					$ut['case'][] = $wpdb->prepare('WHEN %d THEN %s', $term->term_id, serialize($tr));
					$ut['in'][] = (int) $term->term_id;
				}
			}
		}

		// delete relationships
		if (!empty($dr))
			$wpdb->query("DELETE FROM $wpdb->term_relationships
				WHERE object_id IN ( " . implode(',', $dr['id']) . " )
				AND term_taxonomy_id IN ( " . implode(',', $dr['tt']) . " )");

		// delete terms
		if (!empty($dt)) {
			$wpdb->query("DELETE FROM $wpdb->terms WHERE term_id IN ( " . implode(',', $dt['t']) . " ) ");
			$wpdb->query("DELETE FROM $wpdb->term_taxonomy WHERE term_taxonomy_id IN ( " . implode(',', $dt['tt']) . " ) ");
		}

		// update terms
		if (!empty($ut))
			$wpdb->query("UPDATE $wpdb->term_taxonomy
				SET description = ( CASE term_id " . implode(' ', $ut['case']) . " END )
				WHERE term_id IN ( " . implode(',', $ut['in']) . " )");
	}
}
