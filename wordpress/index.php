<?php get_header(); ?>

<div class="row clearfix main">

  <div class="container">

    <div class="column three-fourths">

      <h1 class="page-title" itemprop="headline">Blog</h1>

      <?php if (have_posts()) : while (have_posts()) : the_post(); ?>

      <article id="post-<?php the_ID(); ?>" role="article">

        <h2 class="post-title">
          <a href="<?php the_permalink() ?>" title="<?php the_title_attribute(); ?>"><?php the_title(); ?></a>
        </h2>

        <div class="byline">
          <p>
            <?php the_date(); ?>
          </p>
          <div class="cat">
            <span><?php comments_number('No replies', '1 reply'); ?></span>
          </div>
        </div>

      </article>

      <?php endwhile; ?>
      <?php bones_page_navi(); ?>
      <?php endif; ?>

    </div>

    <?php get_sidebar(); ?>

  </div>

</div>

<?php get_footer(); ?>