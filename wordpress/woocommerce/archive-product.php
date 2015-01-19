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
<div class="row clearfix main archive">
  <div class="container">
      <div class="column full">
  		<?php if ( apply_filters( 'woocommerce_show_page_title', true ) ) : ?>
        <h1 class="page-title" itemprop="headline"><?php woocommerce_page_title(); ?></h1>
            <div class="byline vcard">
              <p>Buy all your shit right here!</p>
                <div class="cat"></div>
            </div>
		<?php endif; ?>
		
		<?php
        $args=array(
        'post_type' => 'product',
        'meta_key' => '_featured',
        'meta_value' => 'yes',
        'caller_get_posts'=> 1
        );
        $my_query = null;
        $my_query = new WP_Query($args);
        if( $my_query->have_posts() ) {
        while ($my_query->have_posts()) : $my_query->the_post(); 
        ?>
        
		<div class="row clearfix featuretitle">
		  <div class="column full">
		    <h2><a href="<?php the_permalink() ?>"><?php the_title(); ?></a><span class="price"><?php echo $product->get_price_html(); ?></span></h2>
		  </div>
		</div>
		<div class="row clearfix feature">
          <div class="column half vero">
            <?php the_post_thumbnail( 'medium' ); ?>
          </div>
          <div class="column half info">
            <?php the_content(); ?>
          </div>
		</div>
		<div class="row clearfix feature2">
          <div class="column full">
            <div class="buybutton">
              <a href="<?php the_permalink() ?>">
                <button class="link">Go to product</button>
              </a>
            </div>
            <a class="verolink" href="https://getvero.tv" target="_blank">Go to getvero.tv for the presentation</a>
           
          </div>
        </div>
        
        <?php endwhile;
        } //if ($my_query)
        wp_reset_query(); // Restore global post data stomped by the_post().
        ?>
        
		<div class="productlist">
        <h2 class="title">Products</h2>
		  <?php echo do_shortcode( '[product_category category="products" orderby="menu_order" order="asc"]' ); ?>
        </div>
    
    </div>
  </div>
</div>

<?php get_footer( 'shop' ); ?>