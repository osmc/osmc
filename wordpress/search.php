<?php get_header(); ?>

<div class="row clearfix main">

  <div class="container">

    <div class="column three-fourths">

      <h1 class="page-title"><span>Search Results for: </span><?php echo esc_attr(get_search_query()); ?></h1>

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
            <?php the_category( ', ') ?>
          </div>
        </div>

        <section class="entry-content">
          <?php the_excerpt(); ?>
        </section>
        <?php // end article section ?>

      </article>

      <?php endwhile; ?>
      <?php bones_page_navi(); ?>
      <?php endif; ?>

    </div>

    <?php get_sidebar(); ?>

  </div>

</div>

<?php get_footer(); ?>