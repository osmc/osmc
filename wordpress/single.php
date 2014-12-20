<?php get_header(); ?>

			<div id="content">

				<div id="inner-content" class="wrap clearfix pure-g-r">

					<div id="main" class="eightcol first clearfix pure-u-3-4" role="main">
					<div class="boxpad">
						<?php if (have_posts()) : while (have_posts()) : the_post(); ?>

							<article id="post-<?php the_ID(); ?>" <?php post_class('clearfix'); ?> role="article" itemscope itemtype="http://schema.org/BlogPosting">

								<header class="article-header">

									<h1 class="entry-title single-title page-title" itemprop="headline"><?php the_title(); ?></h1>
									<div class="byline vcard">
									  <p><?php the_date(); ?></p>
										<div class="cat"><?php the_category(', ') ?></div>
								  </div>

								</header> <?php // end article header ?>

								<section class="entry-content clearfix" itemprop="articleBody">
									<?php the_content(); ?>
								</section> <?php // end article section ?>

								<footer class="article-footer">
									<?php the_tags( '<p class="tags"><span class="tags-title">' . __( '', 'bonestheme' ) . '</span> ', ' &nbsp;', ' </p>' ); ?>
									<?php
										$url = $_SERVER['REQUEST_URI'];

										if (strpos($url,'wiki/'))
										{
										    echo '<a href="http://osmc.tv/wiki">Back to Wiki</a>';
										}

									?>

								</footer> <?php // end article footer ?>

								<?php comments_template(); ?>

							</article> <?php // end article ?>

						<?php endwhile; ?>

						<?php else : ?>

							<article id="post-not-found" class="hentry clearfix">
									<header class="article-header">
										<h1><?php _e( 'Oops, Post Not Found!', 'bonestheme' ); ?></h1>
									</header>
									<section class="entry-content">
										<p><?php _e( 'Uh Oh. Something is missing. Try double checking things.', 'bonestheme' ); ?></p>
									</section>
									<footer class="article-footer">
											<p><?php _e( 'This is the error message in the single.php template.', 'bonestheme' ); ?></p>
									</footer>
							</article>

						<?php endif; ?>
					</div>
					</div> <?php // end #main ?>

					<?php get_sidebar(); ?>

				</div> <?php // end #inner-content ?>

			</div> <?php // end #content ?>

<?php get_footer(); ?>
