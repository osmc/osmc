<?php

/*
 * displays languages in a dropdown list
 *
 * @since 1.2
 */
class PLL_Walker_Dropdown extends Walker {
	var $db_fields = array ('parent' => 'parent', 'id' => 'id');

	/*
	 * outputs one element
	 *
	 * @since 1.2
	 *
	 * @see Walker::start_el
	 */
	function start_el( &$output, $element, $depth = 0, $args = array(), $current_object_id = 0 ) {
		$value = isset($args['value']) && $args['value'] ? $args['value'] : 'slug';
		$output .= sprintf(
			"\t".'<option value="%s"%s>%s</option>'."\n",
			esc_attr($element->$value),
			isset($args['selected']) && $args['selected'] === $element->$value ? ' selected="selected"' : '',
			esc_html($element->name)
		);
	}

	/*
	 * overrides Walker::display_element as expects an object with a parent property
	 *
	 * @since 1.2
	 *
	 * @see Walker::display_element
	 */
	function display_element( $element, &$children_elements, $max_depth, $depth = 0, $args, &$output ) {
		$element = (object) $element; // make sure we have an object
		$element->parent = $element->id = 0; // don't care about this
		parent::display_element( $element, $children_elements, $max_depth, $depth, $args, $output );
	}

	/*
	 * starts the output of the dropdown list
	 *
	 * @since 1.2
	 *
	 * @param array $elements elements to display
	 * @param array $args
	 * @return string
	 */
	function walk($elements, $args = array()) {
		return sprintf(
			'<select name="%1$s" %2$s%3$s>' . "\n" . '%4$s' . "\n" . '</select>'."\n",
			($name = empty($args['name']) ? 'lang_choice' : esc_attr($args['name'])),
			isset($args['id']) && !$args['id'] ? '' : ' id="' . (empty($args['id']) ? $name : esc_attr($args['id'])) . '"',
			empty($args['class']) ? '' : ' class="' . esc_attr($args['class']) . '"',
			parent::walk($elements, -1, $args)
		);
	}
}
