<?php
/**
 * The Template for displaying product archives, including the main shop page which is a post type archive.
 *
 * Override this template by copying it to yourtheme/woocommerce/archive-product.php
 *
 * @author 		WooThemes
 * @package 	WooCommerce/Templates
 * @version     2.0.0
 */

if ( ! defined( 'ABSPATH' ) ) exit; // Exit if accessed directly

get_header( 'shop' ); ?>
<div id="content">
  <div id="inner-content" class="wrap clearfix">
      <div id="main" class="eightcol first clearfix pure-u-1" role="main">
        <div class="boxpad">


		<?php if ( apply_filters( 'woocommerce_show_page_title', true ) ) : ?>
			<header class="article-header">
              <h1 class="entry-title single-title page-title" itemprop="headline"><?php woocommerce_page_title(); ?></h1>
              <div class="byline vcard">
                <p></p>
                  <div class="cat"></div>
              </div>
            </header> <?php // end article header ?>

		<?php endif; ?>
		<div class="productfeature">
		  <?php echo do_shortcode( '[featured_products per_page="1" columns="1"]' ); ?>
		</div>
		<div class="productlist">
		  <?php echo do_shortcode( '[product_category category="products" orderby="menu_order" order="asc"]' ); ?>
        </div>
    
      </div>
    </div>
  </div>
</div>

<?php get_footer( 'shop' ); ?>