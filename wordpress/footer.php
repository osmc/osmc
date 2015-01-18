			<footer class="footer">
			  <div class="container">
                <div class="contentcontainer">
                  <div class="row clearfix">

                    <div class="column third widget1">
                      <?php if ( !function_exists('dynamic_sidebar') || !dynamic_sidebar('footer1') ) : ?>
                      <?php endif; ?>
                    </div>

                    <div class="column third widget2">
                      <?php if ( !function_exists('dynamic_sidebar') || !dynamic_sidebar('footer2') ) : ?>
                      <?php endif; ?>
                    </div>

                    <div class="column third widget3">
                      <?php if ( !function_exists('dynamic_sidebar') || !dynamic_sidebar('footer3') ) : ?>
                      <?php endif; ?>
                    </div>
                  </div>
                  <div class="footerinfo">
                    <div class="middle">
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
                </div>
                
				</div> <?php // end #inner-footer ?>

			</footer> <?php // end footer ?>

		</div> <?php // end #container ?>

		<?php // all js scripts are loaded in library/bones.php ?>
		<?php wp_footer(); ?>

	</body>

</html> <?php // end page. what a ride! ?>
