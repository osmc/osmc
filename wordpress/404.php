<?php get_header(); ?>

			<div id="content">

				<div id="inner-content" class="wrap clearfix">

					<div id="main" class="eightcol first clearfix pure-u-3-4" role="main">
							<?php
							$url = $_SERVER['REQUEST_URI'];
							if (strpos($url, 'wiki/')) {
								$isMailWikiPage = (strcmp($url, '/help/wiki/') == 0);
								$page = 'main';
								if (!$isMailWikiPage) {
									$page = 'pages/' . trim(str_replace('/help/wiki/', '', $url), '/');
								}
								$ch = curl_init();
								$BASE_URL = "https://raw.githubusercontent.com/samnazarko/osmc/master/wiki/";
								curl_setopt($ch, CURLOPT_URL, $BASE_URL . $page);
								curl_setopt($ch, CURLOPT_HEADER, 0);
								curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
								curl_setopt($ch, CURLOPT_CONNECTTIMEOUT , 10);
								curl_setopt($ch, CURLOPT_TIMEOUT, 10);
								$output = curl_exec($ch);
								curl_close($ch);
								$lines = explode("\n",$output);
								echo '<article id="wiki-page" class="wiki-page page type-page status-publish hentry clearfix" role="article" itemscope itemtype="http://schema.org/BlogPosting">';
								echo '<header class="article-header">';
								echo '<h1 class="page-title" itemprop="headline">' . $lines[0] . '</h1>';
								echo '<p class="byline vcard"></p>';
								echo '</header>';
								echo '<br>';
								echo '<br>';
								if (!$isMailWikiPage) {
									echo '<a href="http://osmc.tv/help/wiki">Back to Wiki</a>';
								}
								echo '<br>';
								echo '<a href="https://github.com/samnazarko/osmc/blob/master/wiki/' . $page . '" target="_blank">Edit this page</a>';
								echo '<br>';
								echo '<br>';
								echo '<section class="entry-content clearfix" itemprop="articleBody">';
								$count = count($lines);
								for ($i = 1; $i <= $count; $i++)
								{
									echo $lines[$i];
								}
								http_response_code(200); // We aren't really 404ing. 
							}
							else
							{
								echo '<article id="post-not-found" class="hentry clearfix">';
								echo '<header class="article-header"><h1>Page not found</h1></header>';
								echo '<section class="entry-content">';
								echo 'The page you requested could not be found!';
							}
							?>

							</section> <?php // end article section ?>

						</article> <?php // end article ?>
					<?php get_sidebar(); ?>
					</div> <?php // end #main ?>

				</div> <?php // end #inner-content ?>

			</div> <?php // end #content ?>

<?php get_footer(); ?>
