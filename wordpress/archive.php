<?php get_header(); ?>
 
  <div class="row clearfix main">
    <div class="container">
      <div class="column three-fourths">
      
      <?php if (is_category()) { ?>
        <h1 class="page-title">
          <span><?php _e( '', 'bonestheme' ); ?></span><?php single_cat_title(); ?>
        </h1>

      <?php } elseif (is_tag()) { ?>
        <h1 class="page-title">
          <span><?php _e( 'Posts Tagged:', 'bonestheme' ); ?></span> <?php single_tag_title(); ?>
        </h1>

      <?php } elseif (is_author()) {
        global $post;
        $author_id = $post->post_author;
      ?>

        <h1 class="page-title">
          <span><?php _e( 'Posts By:', 'bonestheme' ); ?></span> <?php the_author_meta('display_name', $author_id); ?>
        </h1>
        
      <?php } elseif (is_day()) { ?>
        <h1 class="page-title">
          <span><?php _e( 'Daily Archives:', 'bonestheme' ); ?></span> <?php the_time('l, F j, Y'); ?>
        </h1>

      <?php } elseif (is_month()) { ?>
        <h1 class="page-title">
          <span><?php _e( 'Monthly Archives:', 'bonestheme' ); ?></span> <?php the_time('F Y'); ?>
        </h1>

      <?php } elseif (is_year()) { ?>
        <h1 class="page-title">
          <span><?php _e( 'Yearly Archives:', 'bonestheme' ); ?></span> <?php the_time('Y'); ?>
        </h1>
      <?php } ?>

      <?php if (have_posts()) : while (have_posts()) : the_post(); ?>

        <article id="post-<?php the_ID(); ?>">

          <h3 class="post-title">
            <a href="<?php the_permalink() ?>" title="<?php the_title_attribute(); ?>"><?php the_title(); ?></a>
          </h3>

          <div class="byline">
            <p>
              <?php the_date(); ?>
            </p>
            <div class="cat">
              <?php the_category(', ') ?>
            </div>
          </div>

          <section class="entry-content">
            <?php the_excerpt(); ?>
          </section>

        </article>

      <?php endwhile; ?>

        <?php if ( function_exists( 'bones_page_navi' ) ) { ?>
          <?php bones_page_navi(); ?>
        <?php } else { ?>
          <nav class="wp-prev-next">
            <ul class="clearfix">
              <li class="prev-link"><?php next_posts_link( __( '&laquo; Older Entries', 'bonestheme' )) ?></li>
              <li class="next-link"><?php previous_posts_link( __( 'Newer Entries &raquo;', 'bonestheme' )) ?></li>
            </ul>
          </nav>
        <?php } ?>

      <?php else : ?>

        <article id="post-not-found" class="hentry clearfix">
            <header class="article-header">
                <h1><?php _e( 'Oops, Post Not Found!', 'bonestheme' ); ?></h1>
            </header>
            <section class="entry-content">
                <p><?php _e( 'Uh Oh. Something is missing. Try double checking things.', 'bonestheme' ); ?></p>
            </section>
        </article>

      <?php endif; ?>
      
      </div>

      <?php get_sidebar(); ?>

    </div>
  </div>
<?php get_footer(); ?>