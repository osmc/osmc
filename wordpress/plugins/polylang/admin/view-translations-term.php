<?php
// displays the translations fields

if (isset($term_id)) {
	// edit term form?>
	<th scope="row"><?php _e('Translations', 'polylang');?></th>
	<td><?php
}
else {
	// add term form?>
	<label><?php _e('Translations', 'polylang');?></label><?php
}?>
<table class="widefat term-translations">
	<?php foreach ($this->model->get_languages_list() as $language) {
		if ($language->term_id == $lang->term_id)
			continue;

		// look for any existing translation in this language
		$translation = 0;
		if (isset($term_id) && $translation_id = $this->model->get_translation('term', $term_id, $language))
			$translation = get_term($translation_id, $taxonomy);
		if (isset($_GET['from_tag']) && $translation_id = $this->model->get_term((int)$_GET['from_tag'], $language))
			$translation = get_term($translation_id, $taxonomy);


		if (isset($term_id)) { // do not display the add new link in add term form ($term_id not set !!!) {
			$link = $add_link = sprintf(
				'<a href="%1$s" class="pll_icon_add" title="%2$s"></a>',
				esc_url($this->links->get_new_term_translation_link($term_id, $taxonomy, $post_type, $language)),
				__('Add new','polylang')
			);
		}

		if ($translation) {
			$link = $this->edit_translation_link($translation->term_id, $taxonomy, $post_type);
		} ?>

		<tr><?php
			if (isset($term_id)) { ?>
				<td class = "pll-language-column"><?php echo $language->flag . '&nbsp;' . esc_html($language->name); ?></td>
				<td class = "hidden"><?php echo $add_link;?></td>
				<td class = "pll-edit-column"><?php echo $link;?></td><?php
			}
			else { ?>
				<td class = "pll-language-column"><?php echo $language->flag ? $language->flag : esc_html($language->slug); ?></td><?php
			} ?>
			<td class = "pll-translation-column"><?php
				printf('
					<input type="hidden" name="term_tr_lang[%1$s]" id="htr_lang_%1$s" value="%2$s"/>
					<input type="text" class="tr_lang" id="tr_lang_%1$s" value="%3$s">',
					esc_attr($language->slug),
					empty($translation) ? 0 : esc_attr($translation->term_id),
					empty($translation) ? '' : esc_attr($translation->name)
				); ?>
			</td>
		</tr><?php
	} // foreach ?>
</table><?php

if (isset($term_id)) {
	// edit term form?>
	</td><?php
}
