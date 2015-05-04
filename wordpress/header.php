<!doctype html>

<!--[if IE 9]> <html class="ie9"> <![endif]-->

	<head>
		<meta charset="utf-8">
		<meta http-equiv="X-UA-Compatible" content="IE=edge,chrome=1">
		<title><?php bloginfo('name'); ?> <?php wp_title(); ?></title>
		<meta name="description" content="<?php echo get_bloginfo ( 'description' );  ?>">
		<meta name="HandheldFriendly" content="True">
		<meta name="MobileOptimized" content="320">
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
        
		<link rel="pingback" href="<?php bloginfo('pingback_url'); ?>">
		<link rel="alternate" type="application/rss+xml" title="OSMC Feed" href="/feed?cat=-42" />
		
		<!--[if lte IE 8]> 
		<script src="//cdnjs.cloudflare.com/ajax/libs/outdated-browser/1.1.0/outdatedbrowser.min.js"></script><link rel="stylesheet" href="<?php echo get_template_directory_uri(); ?>/library/style/lib/outdatedbrowser.min.css">
		<![endif]-->
		
		<?php wp_head(); ?>
        <!-- inject:css -->
        <link type="text/css" rel="stylesheet" href="<?php echo get_template_directory_uri(); ?>/library/style/css/style-4caba1ae.css">
        <!-- endinject -->
        
	</head>

	<body <?php body_class(); ?>>
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
