<!doctype html>
<html prefix="og: http://ogp.me/ns#">
<!--[if IE 9]> <html class="ie9"> <![endif]-->

	<head>
		<meta charset="utf-8">
		<meta http-equiv="X-UA-Compatible" content="IE=edge,chrome=1">
		<title><?php bloginfo('name'); ?><?php wp_title(); ?></title>
		<meta name="description" content="<?php echo get_bloginfo ('description');  ?>" />
		<meta property=og:title content="<?php bloginfo('name'); ?><?php wp_title(); ?>" />
		<meta property=og:description content="<?php echo get_bloginfo ('description');  ?>" />
		<meta property=og:image content="<?php echo get_template_directory_uri(); ?>/library/images/osmc_meta.png" />
		<meta name="HandheldFriendly" content="True">
		<meta name="viewport" content="width=device-width, initial-scale=1.0"/>
		
		<link href='//fonts.googleapis.com/css?family=Source+Sans+Pro:300,400,600,400italic' rel='stylesheet' type='text/css'>
		<link rel="shortcut icon" href="<?php echo get_template_directory_uri(); ?>/library/images/favicons/favicon.ico">
		<link rel="apple-touch-icon" sizes="57x57" href="<?php echo get_template_directory_uri(); ?>/library/images/favicons/apple-touch-icon-57x57.png">
		<link rel="apple-touch-icon" sizes="114x114" href="<?php echo get_template_directory_uri(); ?>/library/images/favicons/apple-touch-icon-114x114.png">
		<link rel="apple-touch-icon" sizes="72x72" href="<?php echo get_template_directory_uri(); ?>/library/images/favicons/apple-touch-icon-72x72.png">
		<link rel="apple-touch-icon" sizes="144x144" href="<?php echo get_template_directory_uri(); ?>/library/images/favicons/apple-touch-icon-144x144.png">
		<link rel="apple-touch-icon" sizes="60x60" href="<?php echo get_template_directory_uri(); ?>/library/images/favicons/apple-touch-icon-60x60.png">
		<link rel="apple-touch-icon" sizes="120x120" href="<?php echo get_template_directory_uri(); ?>/library/images/favicons/apple-touch-icon-120x120.png">
		<link rel="apple-touch-icon" sizes="76x76" href="<?php echo get_template_directory_uri(); ?>/library/images/favicons/apple-touch-icon-76x76.png">
		<link rel="apple-touch-icon" sizes="152x152" href="<?php echo get_template_directory_uri(); ?>/library/images/favicons/apple-touch-icon-152x152.png">
		<link rel="apple-touch-icon" sizes="180x180" href="<?php echo get_template_directory_uri(); ?>/library/images/favicons/apple-touch-icon-180x180.png">
		<link rel="icon" type="image/png" href="<?php echo get_template_directory_uri(); ?>/library/images/favicons/favicon-192x192.png" sizes="192x192">
		<link rel="icon" type="image/png" href="<?php echo get_template_directory_uri(); ?>/library/images/favicons/favicon-160x160.png" sizes="160x160">
		<link rel="icon" type="image/png" href="<?php echo get_template_directory_uri(); ?>/library/images/favicons/favicon-96x96.png" sizes="96x96">
		<link rel="icon" type="image/png" href="<?php echo get_template_directory_uri(); ?>/library/images/favicons/favicon-16x16.png" sizes="16x16">
		<link rel="icon" type="image/png" href="<?php echo get_template_directory_uri(); ?>/library/images/favicons/favicon-32x32.png" sizes="32x32">
		<meta name="msapplication-TileColor" content="#f0f0f0">
		<meta name="msapplication-TileImage" content="<?php echo get_template_directory_uri(); ?>/library/images/favicons/mstile-144x144.png">
		<meta name="msapplication-square70x70logo" content="<?php echo get_template_directory_uri(); ?>/library/images/favicons/mstile-70x70.png">
		<meta name="msapplication-square150x150logo" content="<?php echo get_template_directory_uri(); ?>/library/images/favicons/mstile-150x150.png">
		<meta name="msapplication-square310x310logo" content="<?php echo get_template_directory_uri(); ?>/library/images/favicons/mstile-310x310.png">
		<meta name="msapplication-wide310x150logo" content="<?php echo get_template_directory_uri(); ?>/library/images/favicons/mstile-310x150.png">
		<!-- inject:css -->
		<link type="text/css" rel="stylesheet" href="<?php echo get_template_directory_uri(); ?>/library/style/css/style-9204d8ed.css">
		<!-- endinject -->
		
		<!--[if lte IE 8]> 
		<script src="//cdnjs.cloudflare.com/ajax/libs/outdated-browser/1.1.0/outdatedbrowser.min.js"></script><link rel="stylesheet" href="<?php echo get_template_directory_uri(); ?>/library/style/lib/outdatedbrowser.min.css">
		<![endif]-->
		
		<?php wp_head(); ?>
        <link rel="pingback" href="<?php bloginfo('pingback_url'); ?>">
		<link rel="alternate" type="application/rss+xml" title="OSMC Feed" href="/feed?cat=-42" />
		<script>
          (function(i,s,o,g,r,a,m){i['GoogleAnalyticsObject']=r;i[r]=i[r]||function(){
          (i[r].q=i[r].q||[]).push(arguments)},i[r].l=1*new Date();a=s.createElement(o),
          m=s.getElementsByTagName(o)[0];a.async=1;a.src=g;m.parentNode.insertBefore(a,m)
          })(window,document,'script','//www.google-analytics.com/analytics.js','ga');
          ga('create', 'UA-12677925-5', 'auto');
          ga('send', 'pageview');
        </script>
        
	</head>

	<body <?php body_class(); ?>>
    <style>
    .overlay {
      display: none;
    }
    .popup_donate {
      display: none;
    }
    </style>
    <div class="overlay"></div>
    <div class="popup_donate">
      <div class="donationwidget">
        <h3>Donate</h3>
        <p>OSMC is a new and and young project that needs some support to make sure it lands on its feet.</p>
        <form action="#">
          <input type="tel" class="amount" id="amount" name="amount" required>
          <input type="radio" class="radio" id="usd-pop" name="currency" value="USD" checked="checked">
          <label for="usd-pop">USD</label>
          <input type="radio" class="radio" id="eur-pop" name="currency" value="EUR">
          <label for="eur-pop">EUR</label>
          <input type="radio" class="radio" id="gbp-pop" name="currency" value="GBP">
          <label for="gbp-pop">GBP</label>
          <div class="options">
            <button class="paypal" title="Paypal" type="submit" value="Submit"><img src="https://osmc.tv/wp-content/themes/osmc/library/images/paypal.png">
            </button>
            <button class="stripe" title="Credit Card" type="submit" value="Submit">
            <div class="svg card1">
            <svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" version="1.1" id="Layer_1" x="0px" y="0px" width="1920px" height="1536px" viewBox="-408 -843.5 1920 1536" enable-background="new -408 -843.5 1920 1536" xml:space="preserve"><path fill="#D3D3D3" d="M1333.149-825.402c42.963 0 79.7 15.3 110.3 45.892c30.596 30.6 45.9 67.4 45.9 110.3 V518.173c0 42.963-15.297 79.744-45.893 110.337c-30.595 30.596-67.374 45.893-110.337 45.893H-229.149 c-42.963 0-79.742-15.297-110.337-45.893c-30.596-30.594-45.893-67.374-45.893-110.337V-669.173 c0-42.963 15.297-79.742 45.893-110.337c30.596-30.595 67.374-45.892 110.337-45.892H1333.149z"/><path d="M1352-843.5c44 0 81.7 15.7 113 47c31.334 31.3 47 69 47 113v1216c0 44-15.666 81.667-47 113 c-31.333 31.333-69 47-113 47H-248c-44 0-81.666-15.667-113-47s-47-69-47-113v-1216c0-44 15.666-81.666 47-113s69-47 113-47H1352z M-248-715.5c-8.666 0-16.166 3.166-22.5 9.5s-9.5 13.834-9.5 22.5v224h1664v-224c0-8.666-3.167-16.166-9.5-22.5 s-13.833-9.5-22.5-9.5H-248z M1352 564.5c8.667 0 16.167-3.167 22.5-9.5s9.5-13.833 9.5-22.5v-608H-280v608 c0 8.7 3.2 16.2 9.5 22.5s13.834 9.5 22.5 9.5H1352z M-152 436.5v-128h256v128H-152z M232 436.5v-128h384v128H232z"/></svg>
            </div>
            <div class="svg card2">
            <svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" version="1.1" id="Layer_1" x="0px" y="0px" width="1920px" height="1536px" viewBox="-408 -843.5 1920 1536" enable-background="new -408 -843.5 1920 1536" xml:space="preserve"><path fill="#D3D3D3" d="M1333.149-825.402c42.963 0 79.7 15.3 110.3 45.892c30.596 30.6 45.9 67.4 45.9 110.3 V518.173c0 42.963-15.297 79.744-45.893 110.337c-30.595 30.596-67.374 45.893-110.337 45.893H-229.149 c-42.963 0-79.742-15.297-110.337-45.893c-30.596-30.594-45.893-67.374-45.893-110.337V-669.173 c0-42.963 15.297-79.742 45.893-110.337c30.596-30.595 67.374-45.892 110.337-45.892H1333.149z"/><path d="M1352-843.5c44 0 81.7 15.7 113 47c31.334 31.3 47 69 47 113v1216c0 44-15.666 81.667-47 113 c-31.333 31.333-69 47-113 47H-248c-44 0-81.666-15.667-113-47s-47-69-47-113v-1216c0-44 15.666-81.666 47-113s69-47 113-47H1352z M-248-715.5c-8.666 0-16.166 3.166-22.5 9.5s-9.5 13.834-9.5 22.5v224h1664v-224c0-8.666-3.167-16.166-9.5-22.5 s-13.833-9.5-22.5-9.5H-248z M1352 564.5c8.667 0 16.167-3.167 22.5-9.5s9.5-13.833 9.5-22.5v-608H-280v608 c0 8.7 3.2 16.2 9.5 22.5s13.834 9.5 22.5 9.5H1352z M-152 436.5v-128h256v128H-152z M232 436.5v-128h384v128H232z"/></svg>
            </div>
            </button>
          </div>
        </form>
      </div>
    </div>
    <div id="outdated"></div>
    <!--[if lte IE 8]>
    <script>
    outdatedBrowser({
    bgColor: '#41c5cb',
    color: '#ffffff',
    lowerThan: 'transform',
    languagePath: 'your_path/outdatedbrowser/lang/en.html'
    });
    </script>
    <![endif]-->

		<div id="container">
			<header class="header">
				<div class="container">
					<div class="column full">
						<div id="logo-img"><a href="/"><img src="<?php echo get_template_directory_uri(); ?>/library/images/logo-l.png"></a></div>
							<?php // if you'd like to use the site description you can un-comment it below ?>
							<?php // bloginfo('description'); ?>

							<nav role="navigation" class="pure-menu-horizontal nav-header">
								<?php bones_main_nav(); ?>
							</nav>
							<div class="bars" id="nav-res-toggle">
							  <p>Menu</p>
							</div>
					</div>
				</div> <?php // end #inner-header ?>

			</header> <?php // end header ?>
			<div class="container">
              <div class="wc-cart">
                <?php if ( is_user_logged_in() ) { ?>
                  <a href="<?php echo get_permalink( get_option('woocommerce_myaccount_page_id') ); ?>" title="<?php _e('My Account','woothemes'); ?>"><?php _e('My Account','woothemes'); ?></a>
               <?php } 
               else { ?>
                  <a href="<?php echo get_permalink( get_option('woocommerce_myaccount_page_id') ); ?>" title="<?php _e('Login / Register','woothemes'); ?>"><?php _e('Login','woothemes'); ?></a>
               <?php } ?> - 
			   <?php global $woocommerce; ?>
                <a class="cart-contents" href="<?php echo $woocommerce->cart->get_cart_url(); ?>" title="<?php _e('View your shopping cart', 'woothemes'); ?>"><?php echo sprintf(_n('%d item', '%d items', $woocommerce->cart->cart_contents_count, 'woothemes'), $woocommerce->cart->cart_contents_count);?> - <?php echo $woocommerce->cart->get_cart_total(); ?></a>
              </div>
			</div>
