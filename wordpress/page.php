<?php get_header(); ?>

  <div class="row clearfix">
    <div class="container">
      <div class="column three-fourths">
        <?php if (have_posts()) : while (have_posts()) : the_post(); ?>


                <h1 class="page-title" itemprop="headline"><?php the_title(); ?></h1>
                <p class="byline vcard"></p>


            <section class="entry-content clearfix" itemprop="articleBody">
                <?php the_content(); ?>
        </section> <?php // end article section ?>

            <footer class="article-footer">
                <?php the_tags( '<span class="tags">' . __( 'Tags:', 'bonestheme' ) . '</span> ', ', ', '' ); ?>

            </footer> <?php // end article footer ?>

            <?php comments_template(); ?>

        <?php endwhile; else : ?>
                <article id="post-not-found" class="hentry clearfix">
                    <header class="article-header">
                        <h1><?php _e( 'Oops, Post Not Found!', 'bonestheme' ); ?></h1>
                    </header>
                    <section class="entry-content">
                        <p><?php _e( 'Uh Oh. Something is missing. Try double checking things.', 'bonestheme' ); ?></p>
                    </section>
                    <footer class="article-footer">
                            <p><?php _e( 'This is the error message in the page.php template.', 'bonestheme' ); ?></p>
                    </footer>
                </article>

          <?php endif; ?>
        </div> <?php // end #main ?>
      <?php get_sidebar(); ?>
    </div> <?php // end #inner-content ?>
  </div> <?php // end #content ?>
<?php get_footer(); ?>
