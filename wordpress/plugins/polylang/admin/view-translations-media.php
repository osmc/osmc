<?php
// needs WP 3.5+
?>

<p><strong><?php _e('Translations', 'polylang');?></strong></p>
<table><?php
	foreach ($this->model->get_languages_list() as $language) {
		if ($language->term_id == $lang->term_id)
			continue;?>

		<tr>
			<td class = "pll-media-language-column"><?php echo $language->flag . '&nbsp;' . esc_html($language->name); ?></td>
			<td class = "pll-edit-column"><?php
				// the translation exists
				if (($translation_id = $this->model->get_translation('post', $post_id, $language)) && $translation_id != $post_id) {
					printf(
						'<input type="hidden" name="media_tr_lang[%s]" value="%d" /><a href="%s" title="%s" class="pll_icon_edit"></a>',
						esc_attr($language->slug),
						esc_attr($translation_id),
						esc_url(get_edit_post_link($translation_id)),
						__('Edit','polylang')
					);
				}

				// no translation
				else {
					printf(
						'<a href="%s" title="%s" class="pll_icon_add"></a>',
						esc_url($this->links->get_new_post_translation_link($post_id, $language)),
						__('Add new','polylang')
					);
				}?>
			</td>
		</tr><?php
	} // foreach ?>
</table>
