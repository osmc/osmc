			<footer class="footer" role="contentinfo">

				<div id="inner-footer" class="wrap clearfix">
				<div id="footer-widgets" class="pure-g-r">

					<div id="footer-widget1" class="pure-u-1-3">
					<div class="boxpad">
					<?php if ( !function_exists('dynamic_sidebar') || !dynamic_sidebar('footer1') ) : ?>
					<?php endif; ?>
					</div>
					</div>

					<div id="footer-widget2" class="pure-u-1-3">
					<div class="boxpad">
					<?php if ( !function_exists('dynamic_sidebar') || !dynamic_sidebar('footer2') ) : ?>
					<?php endif; ?>
					</div>
					</div>

					<div id="footer-widget3" class="pure-u-1-3">
					<div class="boxpad">
					<?php if ( !function_exists('dynamic_sidebar') || !dynamic_sidebar('footer3') ) : ?>
					<?php endif; ?>
					</div>
					</div>
				</div>
				<div style="clear-both"></div>
				<div class="copyright">
				<img src="<?php echo get_template_directory_uri(); ?>/library/images/logo-l.png">
				</div>
                             <?php
                                        /* base footer */
                                        $LOGO_STRING = "Logo by Simon Brunton";
                                        $DESIGN_STRING = "Designed by Mark Theis Madsen";
					$lang = array_shift(explode(".",$_SERVER['HTTP_HOST']));
					switch ($lang)
					{
						case "fr":
						$LOGO_STRING = "Logo réalisé par Simon Brunton";
						$DESIGN_STRING = "Design réalisé par Mark Theis Madsen";
						break;
						case "da":
						$LOGO_STRING = "Logo af Simon Brunton";
						$DESIGN_STRING = "Designet af Mark Theis Madsen";
						break;
						case "se":
						$LOGO_STRING = "Logga av Simon Brunton";
						$DESIGN_STRING = "Designad av Mark Theis Madsen";
					}
                                ?>
				<br>
                                <a href="http://simonbrunton.com" target="_blank" class="designed"><?echo $LOGO_STRING;?></a><br>
                                <a href="http://markthe.is" target="_blank" class="designed"><?echo $DESIGN_STRING;?></a>
				<p class="designed">(c) 2014 OSMC </p>
				<a href="http://osmc.tv/about/corporate" class="designed">Corporate</a>
				</div> <?php // end #inner-footer ?>

			</footer> <?php // end footer ?>

		</div> <?php // end #container ?>

		<?php // all js scripts are loaded in library/bones.php ?>
		<?php wp_footer(); ?>

	</body>

</html> <?php // end page. what a ride! ?>
