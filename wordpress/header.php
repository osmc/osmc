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
		<link type="text/css" rel="stylesheet" href="<?php echo get_template_directory_uri(); ?>/library/style/css/style-ce1cc206.css">
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
        <p>OSMC is free and will always remain free. Help support further development by making a donation. No matter how large or small, every contribution is appreciated.</p>
        <form action="#">
          <input type="tel" class="amount" id="amount" name="amount" required>
          <input type="radio" class="radio" id="usd-pop" name="currency" value="USD" checked="checked">
          <label for="usd-pop">USD</label>
          <input type="radio" class="radio" id="eur-pop" name="currency" value="EUR">
          <label for="eur-pop">EUR</label>
          <input type="radio" class="radio" id="gbp-pop" name="currency" value="GBP">
          <label for="gbp-pop">GBP</label>
          <div class="options">
            <button class="paypal" title="Paypal" type="submit" value="Submit">
            <svg xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:cc="http://creativecommons.org/ns#" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" version="1.1" id="Layer_1" x="0px" y="0px" width="207.078px" height="55.181px" viewBox="52.708 15.014 207.078 55.181" enable-background="new 52.708 15.014 207.078 55.181" xml:space="preserve"><path fill="#009CDE" d="M194.301 30.1c-0.881 5.781-5.295 5.781-9.566 5.781h-2.432l1.705-10.795 c0.104-0.651 0.666-1.132 1.326-1.132h1.115c2.906 0 5.7 0 7.1 1.655C194.365 26.6 194.6 28.1 194.3 30.1 M192.441 15.014h-16.105c-1.104 0-2.041 0.802-2.213 1.891l-6.512 41.302c-0.129 0.8 0.5 1.6 1.3 1.551h8.266 c0.77 0 1.426-0.561 1.547-1.32l1.848-11.711c0.172-1.088 1.109-1.891 2.211-1.891h5.096c10.609 0 16.734-5.133 18.334-15.311 c0.721-4.448 0.029-7.945-2.055-10.395C201.891 16.4 197.8 15 192.4 15"/><path fill="#003087" d="M79.415 30.1c-0.88 5.781-5.295 5.781-9.567 5.781h-2.43l1.705-10.795c0.102-0.651 0.665-1.132 1.325-1.132 h1.115c2.908 0 5.7 0 7.1 1.655C79.479 26.6 79.7 28.1 79.4 30.1 M77.557 15.014H61.449 c-1.103 0-2.039 0.802-2.212 1.891l-6.513 41.302c-0.128 0.8 0.5 1.6 1.3 1.551h7.692c1.101 0 2.037-0.801 2.209-1.887 l1.759-11.145c0.17-1.088 1.109-1.891 2.209-1.891h5.096c10.61 0 16.734-5.133 18.333-15.311c0.721-4.448 0.029-7.945-2.054-10.395 C87.006 16.4 82.9 15 77.6 15"/><path fill="#003087" d="M114.951 44.928c-0.744 4.406-4.242 7.365-8.706 7.365c-2.236 0-4.029-0.721-5.179-2.082 c-1.14-1.35-1.57-3.275-1.208-5.416c0.693-4.369 4.249-7.422 8.643-7.422c2.191 0 4 0.7 5.1 2.1 C114.824 40.9 115.3 42.8 115 44.9 M125.701 29.915h-7.713c-0.66 0-1.223 0.479-1.328 1.134l-0.338 2.156l-0.537-0.781 c-1.672-2.424-5.396-3.235-9.114-3.235c-8.521 0-15.8 6.459-17.216 15.515c-0.737 4.5 0.3 8.8 2.9 11.8 c2.353 2.8 5.7 3.9 9.7 3.922c6.87 0 10.678-4.412 10.678-4.412l-0.344 2.145c-0.129 0.8 0.5 1.6 1.3 1.551h6.945 c1.102 0 2.039-0.799 2.211-1.889l4.17-26.403C127.156 30.7 126.5 29.9 125.7 29.9"/><path fill="#009CDE" d="M229.838 44.928c-0.746 4.406-4.244 7.365-8.707 7.365c-2.236 0-4.029-0.721-5.18-2.082 c-1.141-1.35-1.568-3.275-1.207-5.416c0.693-4.369 4.248-7.422 8.643-7.422c2.191 0 4 0.7 5.1 2.1 C229.711 40.9 230.2 42.8 229.8 44.9 M240.586 29.915h-7.713c-0.66 0-1.223 0.479-1.326 1.134l-0.34 2.156l-0.539-0.781 c-1.67-2.424-5.393-3.235-9.111-3.235c-8.52 0-15.799 6.459-17.215 15.515c-0.738 4.5 0.3 8.8 2.9 11.8 c2.354 2.8 5.7 3.9 9.7 3.922c6.869 0 10.68-4.412 10.68-4.412l-0.346 2.145c-0.129 0.8 0.5 1.6 1.3 1.551h6.945 c1.104 0 2.039-0.799 2.213-1.889l4.17-26.403C242.043 30.7 241.4 29.9 240.6 29.9"/><path fill="#003087" d="M166.781 29.915h-7.754c-0.74 0-1.434 0.368-1.85 0.982l-10.696 15.75l-4.531-15.134 c-0.285-0.947-1.156-1.598-2.145-1.598h-7.621c-0.92 0-1.568 0.906-1.27 1.776l8.535 25.057l-8.029 11.3 c-0.629 0.9 0 2.1 1.1 2.117h7.744c0.734 0 1.422-0.359 1.842-0.961l25.78-37.211 C168.5 31.1 167.9 29.9 166.8 29.9"/><path fill="#009CDE" d="M249.678 16.15l-6.611 42.057c-0.129 0.8 0.5 1.6 1.3 1.553h6.648c1.102 0 2.039-0.803 2.211-1.891 l6.518-41.302c0.129-0.814-0.5-1.552-1.326-1.552h-7.439C250.342 15 249.8 15.5 249.7 16.1"/></svg>
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
          <div class="alternative">
            <p>You can also donate via <a href="https://www.coinbase.com/checkouts/6931095171d32c0dce231a9657f6168e" target="_blank" title="Bitcoin">Bitcoin</a> or <a href="https://flattr.com/submit/auto?user_id=OSMC&url=https%3A%2F%2Fosmc.tv" target="_blank" title="Flattr this">Flattr</a>.</p>
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
