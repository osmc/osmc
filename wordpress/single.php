<?php get_header(); ?>

<div class="row clearfix main">
  <div class="container">
    <div class="column three-fourths">
      <?php if (have_posts()) : while (have_posts()) : the_post(); ?>

      <article id="post-<?php the_ID(); ?>" itemscope itemtype="http://schema.org/BlogPosting">

        <header class="article-header">

          <h1 class="page-title" itemprop="headline"><?php the_title(); ?></h1>
          <div class="byline vcard">
            <p>
              <?php the_date(); ?>
            </p>
            <div class="cat">
              <?php the_category( ', ') ?>
            </div>
          </div>

        </header>

        <section class="entry-content" itemprop="articleBody">
          <?php the_content(); ?>
        </section>

        <div class="article-footer">
          <?php the_tags( '<p class="tags"><span class="tags-title">' . __( '', 'bonestheme' ) . '</span> ', ' &nbsp;', ' </p>' ); ?>

        </div>

        <?php comments_template(); ?>

      </article>
      <?php // end article ?>

      <?php endwhile; ?>

      <?php else : ?>

      <article id="post-not-found" class="hentry clearfix">
        <header class="article-header">
          <h1><?php _e( 'Oops, Post Not Found!', 'bonestheme' ); ?></h1>
        </header>
        <section class="entry-content">
          <p>
            <?php _e( 'Uh Oh. Something is missing. Try double checking things.', 'bonestheme' ); ?>
          </p>
        </section>
        <div class="article-footer">
          <p>
            <?php _e( 'This is the error message in the single.php template.', 'bonestheme' ); ?>
          </p>
        </div>
      </article>

      <?php endif; ?>
    </div>

    <?php get_sidebar(); ?>

  </div>

</div>

<?php get_footer(); ?>