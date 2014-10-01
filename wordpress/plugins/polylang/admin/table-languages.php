<?php

if(!class_exists('WP_List_Table')){
	require_once( ABSPATH . 'wp-admin/includes/class-wp-list-table.php' ); // since WP 3.1
}

/*
 * a class to create the languages table in Polylang settings
 * Thanks to Matt Van Andel (http://www.mattvanandel.com) for its plugin "Custom List Table Example" !
 *
 * @since 0.1
 */
class PLL_Table_Languages extends WP_List_Table {

	/*
	 * constructor
	 *
	 * @since 0.1
	 */
	function __construct() {
		parent::__construct(array(
			'plural'   => 'Languages', // do not translate (used for css class)
			'ajax'	   => false
		));
	}

	/*
	 * displays the item information in a column (default case)
	 *
	 * @since 0.1
	 *
	 * @param object $item
	 * @param string $column_name
	 * @return string
	 */
	function column_default($item, $column_name) {
		return $item->$column_name;
	}

	/*
	 * displays the item information in the column 'name'
	 * displays the edit and delete links
	 *
	 * @since 0.1
	 *
	 * @param object $item
	 * @return string
	 */
	function column_name($item) {
		return $item->name . $this->row_actions(array(
			'edit'   => sprintf(
				'<a href="%s">%s</a>',
				esc_url(admin_url('admin.php?page=mlang&amp;pll_action=edit&amp;lang=' . $item->term_id)),
				__('Edit','polylang')
			),
			'delete' => sprintf(
				'<a href="%s" onclick = "return confirm(\'%s\');">%s</a>',
				wp_nonce_url('?page=mlang&amp;pll_action=delete&amp;noheader=true&amp;lang=' . $item->term_id, 'delete-lang'),
				__('You are about to permanently delete this language. Are you sure?', 'polylang'),
				__('Delete','polylang')
			)
		));
	}

	/*
	 * gets the list of columns
	 *
	 * @since 0.1
	 *
	 * @return array the list of column titles
	 */
	function get_columns() {
		return array(
			'name'       => __('Full name', 'polylang'),
			'locale'     => __('Locale', 'polylang'),
			'slug'       => __('Code', 'polylang'),
			'term_group' => __('Order', 'polylang'),
			'flag'       => __('Flag', 'polylang'),
			'count'      => __('Posts', 'polylang')
		);
	}

	/*
	 * gets the list of sortable columns
	 *
	 * @since 0.1
	 *
	 * @return array
	 */
	function get_sortable_columns() {
		return array(
			'name'		=> array('name', true), // sorted by name by default
			'locale' => array('locale', false),
			'slug'		=> array('slug', false),
			'term_group'  => array('term_group', false),
			'count'	   => array('count', false)
		);
	}

	/*
	 * prepares the list of items for displaying
	 *
	 * @since 0.1
	 *
	 * @param array $data
	 */
	function prepare_items($data = array()) {
		$per_page = $this->get_items_per_page('pll_lang_per_page');
		$this->_column_headers = array($this->get_columns(), array(), $this->get_sortable_columns());

		function usort_reorder($a, $b){
			$orderby = !empty($_REQUEST['orderby']) ? $_REQUEST['orderby'] : 'name';
			$result = strcmp($a->$orderby, $b->$orderby); // determine sort order
			return (empty($_REQUEST['order']) || $_REQUEST['order'] == 'asc') ? $result : -$result; // send final sort direction to usort
		};

		usort($data, 'usort_reorder');

		$total_items = count($data);
		$this->items = array_slice($data, ($this->get_pagenum() - 1) * $per_page, $per_page);

		$this->set_pagination_args(array(
			'total_items' => $total_items,
			'per_page'	=> $per_page,
			'total_pages' => ceil($total_items/$per_page)
		));
	}
}
