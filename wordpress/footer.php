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
				<div class="footerinfo pure-g-r">
                  <div class="middle pure-u-1">
                    <img src="<?php echo get_template_directory_uri(); ?>/library/images/logo-l.png">
                    <br>
                    <a href="http://simonbrunton.com" target="_blank" class="designed">Logo by Simon Brunton</a>
                    <br>
                    <a href="http://markthe.is" target="_blank" class="designed">Designed by Mark Theis Maden</a>
                    <br><br>
                    <p>&#169; 2015 OSMC </p>
                    
                    <div class="corporate">
                      <a href="http://osmc.tv/about/corporate">Corporate</a>
                    </div>
                    <div class="status">
                      <a target="_blank" href="http://stats.osmc.io">System Status</a>
                    </div>
                  </div>
                </div>
                
				</div> <?php // end #inner-footer ?>

			</footer> <?php // end footer ?>

		</div> <?php // end #container ?>

		<?php // all js scripts are loaded in library/bones.php ?>
		<?php wp_footer(); ?>

	</body>

</html> <?php // end page. what a ride! ?>
