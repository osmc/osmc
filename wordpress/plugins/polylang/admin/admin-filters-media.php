<?php

/*
 * manages filters and actions related to media on admin side
 * capability to edit / create media is checked before loading this class
 *
 * @since 1.2
 */
class PLL_Admin_Filters_Media extends PLL_Admin_Filters_Post_Base {
	/*
	 * constructor: setups filters and actions
	 *
	 * @since 1.2
	 *
	 * @param object $polylang
	 */
	public function __construct(&$polylang) {
		parent::__construct($polylang);

		// adds the language field and translations tables in the 'Edit Media' panel
		add_filter('attachment_fields_to_edit', array(&$this, 'attachment_fields_to_edit'), 10, 2);

		// ajax response for changing the language in media form
		add_action('wp_ajax_media_lang_choice', array(&$this,'media_lang_choice'));

		// adds actions related to languages when creating, saving or deleting media
		add_action('add_attachment', array(&$this, 'set_default_language'));
		add_filter('attachment_fields_to_save', array(&$this, 'save_media'), 10, 2);
		add_filter('wp_delete_file', array(&$this, 'wp_delete_file'));

		// creates a media translation
		if (isset($_GET['action'], $_GET['new_lang'], $_GET['from_media']) && $_GET['action'] == 'translate_media')
			add_action('admin_init', array(&$this, 'translate_media'));
	}

	/*
	 * adds the language field and translations tables in the 'Edit Media' panel
	 * needs WP 3.5+
	 *
	 * @since 0.9
	 *
	 * @param array $fields list of form fields
	 * @param object $post
	 * @return array modified list of form fields
	 */
	public function attachment_fields_to_edit($fields, $post) {
		if ($GLOBALS['pagenow'] == 'post.php')
			return $fields; // don't add anything on edit media panel for WP 3.5+ since we have the metabox

		$post_id = $post->ID;
		$lang = $this->model->get_post_language($post_id);

		$dropdown = new PLL_Walker_Dropdown();
		$fields['language'] = array(
			'label' => __('Language', 'polylang'),
			'input' => 'html',
			'html'  => $dropdown->walk($this->model->get_languages_list(), array(
				'name'     => sprintf('attachments[%d][language]', $post_id),
				'class'    => 'media_lang_choice',
				'selected' => $lang ? $lang->slug : ''
			))
		);

		return $fields;
	}

	/*
	 * ajax response for changing the language in media form
	 * needs WP 3.5+
	 *
	 * @since 0.9
	 */
	public function media_lang_choice() {
		check_ajax_referer('pll_language', '_pll_nonce');

		preg_match('#([0-9]+)#', $_POST['post_id'], $matches);
		$post_id = $matches[1];
		$lang = $this->model->get_language($_POST['lang']);

		ob_start();
		if ($lang) {
			include(PLL_ADMIN_INC.'/view-translations-media.php');
			$data = ob_get_contents();
		}
		$x = new WP_Ajax_Response(array('what' => 'translations', 'data' => $data));
		ob_end_clean();

		$x->send();
	}

	/*
	 * creates a media translation
	 *
	 * @since 0.9
	 */
	public function translate_media() {
		//security check
		check_admin_referer('translate_media');

		$post = get_post($_GET['from_media']);
		$post_id = $post->ID;

		// create a new attachment (translate attachment parent if exists)
		$post->ID = null; // will force the creation
		$post->post_parent = ($post->post_parent && $tr_parent = $this->model->get_translation('post', $post->post_parent, $_GET['new_lang'])) ? $tr_parent : 0;
		$tr_id = wp_insert_attachment($post);
		add_post_meta($tr_id, '_wp_attachment_metadata', get_post_meta($post_id, '_wp_attachment_metadata', true));
		add_post_meta($tr_id, '_wp_attached_file', get_post_meta($post_id, '_wp_attached_file', true));

		$translations = $this->model->get_translations('post', $post_id);
		if (!$translations && $lang = $this->model->get_post_language($post_id))
			$translations[$lang->slug] = $post_id;

		$translations[$_GET['new_lang']] = $tr_id;
		$this->model->save_translations('post', $tr_id, $translations);

		wp_redirect(admin_url(sprintf('post.php?post=%d&action=edit', $tr_id))); // WP 3.5+
		exit;
	}

	/*
	 * called when a media is saved
	 * saves language and translations
	 *
	 * @since 0.9
	 *
	 * @param array $post
	 * @param array $attachment
	 * @return array unmodified $post
	 */
	public function save_media($post, $attachment) {
		// language is filled in attachment by the function applying the filter 'attachment_fields_to_save'
		// all security checks have been done by functions applying this filter
		if (!empty($attachment['language']))
			$this->model->set_post_language($post['ID'], $attachment['language']);

		if (isset($_POST['media_tr_lang']))
			$this->save_translations($post['ID'], $_POST['media_tr_lang']);

		return $post;
	}

	/*
	 * prevents WP deleting files when there are still media using them
	 * thanks to Bruno "Aesqe" Babic and its plugin file gallery in which I took all the ideas for this function
	 *
	 * @since 0.9
	 *
	 * @param string $file
	 * @return string unmodified $file
	 */
	public function wp_delete_file($file) {
		global $wpdb;

		$uploadpath = wp_upload_dir();
		$ids = $wpdb->get_col($wpdb->prepare("
			SELECT post_id FROM $wpdb->postmeta
			WHERE meta_key = '_wp_attached_file' AND meta_value = '%s'",
			ltrim($file, $uploadpath['basedir'])
		));

		if (!empty($ids)) {
			// regenerate intermediate sizes if it's an image (since we could not prevent WP deleting them before)
			wp_update_attachment_metadata($ids[0], wp_generate_attachment_metadata($ids[0], $file));
			return ''; // prevent deleting the main file
		}

		return $file;
	}
}
