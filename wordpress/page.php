<?php
function parseWiki($url)
{
	$ch = curl_init();
	$BASE_URL = "https://raw.githubusercontent.com/samnazarko/osmc-wiki/master/";
	curl_setopt($ch, CURLOPT_URL, $BASE_URL . $url);
	curl_setopt($ch, CURLOPT_HEADER, 0);
	curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
	curl_setopt($ch, CURLOPT_CONNECTTIMEOUT , 10);
	curl_setopt($ch, CURLOPT_TIMEOUT, 10);
	$output = curl_exec($ch);
	curl_close($ch);
    echo $output;
}
?>
<?php get_header(); ?>

			<div id="content">

				<div id="inner-content" class="wrap clearfix pure-g-r">

						<div id="main" class="eightcol first clearfix pure-u-3-4" role="main">
						<div class="boxpad">
							<?php if (have_posts()) : while (have_posts()) : the_post(); ?>

							<article id="post-<?php the_ID(); ?>" <?php post_class( 'clearfix' ); ?> role="article" itemscope itemtype="http://schema.org/BlogPosting">

								<header class="article-header">

									<h1 class="page-title" itemprop="headline"><?php the_title(); ?></h1>
									<p class="byline vcard"></p>

								</header> <?php // end article header ?>

								<section class="entry-content clearfix" itemprop="articleBody">
									<?php the_content(); ?>
							</section> <?php // end article section ?>

								<footer class="article-footer">
									<?php the_tags( '<span class="tags">' . __( 'Tags:', 'bonestheme' ) . '</span> ', ', ', '' ); ?>

								</footer> <?php // end article footer ?>

								<?php comments_template(); ?>

							</article>
							<?php // end article ?>

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
							<?php
							$url = $_SERVER['REQUEST_URI'];
							if (strpos($url, 'wiki/')) {
								$isMailWikiPage = (strcmp($url, '/help/wiki/') == 0);
								$page = 'main';
								if (!$isMailWikiPage) {
									$page = 'pages/' . trim(str_replace('/help/wiki/', '', $url), '/');
								}

								if (!$isMailWikiPage) {
									echo '<a href="http://osmc.tv/wiki">Back to Wiki</a>';
								}
								echo '</br>';
								echo '<a href="https://github.com/samnazarko/osmc-wiki/blob/master/' . $page . '" target="_blank">Edit this page</a>';
								echo '<br/>';
								parseWiki($page);
							}
							?>
						</div>
						</div> <?php // end #main ?>
						<?php get_sidebar(); ?>

				</div> <?php // end #inner-content ?>

			</div> <?php // end #content ?>

<?php get_footer(); ?>
