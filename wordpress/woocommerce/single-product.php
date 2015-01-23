<?php
/**
 * The Template for displaying all single products.
 *
 * Override this template by copying it to yourtheme/woocommerce/single-product.php
 *
 * @author 		WooThemes
 * @package 	WooCommerce/Templates
 * @version     1.6.4
 */

if ( ! defined( 'ABSPATH' ) ) exit; // Exit if accessed directly

get_header( 'shop' ); ?>
<div class="row clearfix main single">
  <div class="container">
      <div class="column three-fourths">
        <h1 class="page-title" itemprop="headline"><?php the_title(); ?></h1>
        <div class="byline vcard">
        </div>
		<?php while ( have_posts() ) : the_post(); ?>

        <?php wc_get_template_part( 'content', 'single-product' ); ?>
			
        <div class="content"><?php the_content(); ?></div>
		<?php endwhile; // end of the loop. ?>
    </div>
    <?php
          /**
           * woocommerce_sidebar hook
           *
           * @hooked woocommerce_get_sidebar - 10
           */
          do_action( 'woocommerce_sidebar' );
      ?>
	<?php
      /**
       * woocommerce_after_main_content hook
       *
       * @hooked woocommerce_output_content_wrapper_end - 10 (outputs closing divs for the content)
       */
      do_action( 'woocommerce_after_main_content' );
	?>
    
  </div>
</div>

<?php get_footer( 'shop' ); ?>