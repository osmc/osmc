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

				<div id="inner-content" class="wrap clearfix">

					<div id="main" class="eightcol first clearfix" role="main">

						<article id="post-not-found" class="hentry clearfix">

							<header class="article-header">

								<h1><?php _e( '404 - Article Not Found', 'bonestheme' ); ?></h1>

							</header> <?php // end article header ?>

							<section class="entry-content">
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
							else
							{
								echo '404-text-test';
							}
							?>

							</section> <?php // end article section ?>

						</article> <?php // end article ?>

					</div> <?php // end #main ?>

				</div> <?php // end #inner-content ?>

			</div> <?php // end #content ?>

<?php get_footer(); ?>
