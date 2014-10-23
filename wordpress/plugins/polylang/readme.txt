=== Polylang ===
Contributors: Chouby
Donate link: https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=CCWWYUUQV8F4E
Tags: multilingual, bilingual, translate, translation, language, multilanguage, international, localization
Requires at least: 3.5
Tested up to: 4.0
Stable tag: 1.5.6
License: GPLv2 or later

Polylang adds multilingual content management support to WordPress.

== Description ==

= Features  =

Polylang allows you to create a bilingual or multilingual WordPress site. You write posts, pages and create categories and post tags as usual, and then define the language for each of them. The translation of a post, whether it is in the default language or not, is optional. The translation has to be done by the site editor as Polylang does not integrate any automatic or professional translation service.

* You can use as many languages as you want. RTL language scripts are supported. WordPress languages files are automatically downloaded and updated.
* You can translate posts, pages, media, categories, post tags, menus, widgets... Custom post types, custom taxonomies, sticky posts and post formats, RSS feeds and all default WordPress widgets are supported.
* The language is either set by the content or by the language code in url (either directory or subdomain), or you can use one different domain per language
* Categories, post tags as well as some other metas are automatically copied when adding a new post or page translation
* A customizable language switcher is provided as a widget or in the nav menu
* The admin interface is of course multilingual too and each user can set the WordPress admin language in its profile

= Translators =

The plugin admin interface is currently available in 34 languages: English, French, German by [Christian Ries](http://www.singbyfoot.lu), Russian by [yoyurec](http://yoyurec.in.ua) and unostar, Greek by [theodotos](http://www.ubuntucy.org), Dutch by [AlbertGn](http://wordpress.org/support/profile/albertgn), Hebrew by [ArielK](http://www.arielk.net), Polish by [Peter Paciorkiewicz](http://www.paciorkiewicz.pl), [Bartosz](http://www.dfactory.eu/) and Sebastian Janus, Latvian by [@AndyDeGroo](http://twitter.com/AndyDeGroo), Italian by [Luca Barbetti](http://wordpress.org/support/profile/lucabarbetti), Danish by [Compute](http://wordpress.org/support/profile/compute), Spanish by Curro, Portuguese by [Vitor Carvalho](http://vcarvalho.com/), Lithuanian by [Naglis Jonaitis](http://najo.lt/), Turkish by [darchws](http://darch.ws/) and [Abdullah Pazarbasi](http://www.abdullahpazarbasi.com/), Finnish by [Jani Alha](http://www.wysiwyg.fi), Bulgarian by [pavelsof](http://wordpress.org/support/profile/pavelsof), Belarusian by [Alexander Markevitch](http://fourfeathers.by/), Afrikaans by [Kobus Joubert](http://translate3d.com/), Hungarian by Csaba Erdei, Norwegian by [Tom Boersma](http://www.oransje.com/), Slovak by [Branco (WebHostingGeeks.com)](http://webhostinggeeks.com/user-reviews/), Swedish by [matsii](http://wordpress.org/support/profile/matsii) and [Jon Täng](http://jontang.se), Catalan by [Núria Martínez Berenguer](http://www.linkedin.com/profile/view?id=127867004&trk=nav_responsive_tab_profile&locale=en_US), Ukrainian by [cmd soft](http://www.cmd-soft.com/) and [http://getvoip.com/](http://getvoip.com/), Estonian by [Ahto Naris](http://profiles.wordpress.org/ahtonaris/), Venetian by Michele Brunelli, simplified Chinese by [Changmeng Hu](http://www.wpdaxue.com), Indonesian by [ajoull](http://www.ajoull.com/), Arabic by [Anas Sulaiman](http://ahs.pw/), Traditional Chinese by [香腸](http://sofree.cc/), Czech by [Přemysl Karbula](http://www.premyslkarbula.cz), Serbian by Sinisa, Myanmar by Sithu Thwin
= Credits =

Most of the flags included with Polylang are coming from [famfamfam](http://famfamfam.com/) and are public domain. Icons are coming from [Icomoon](http://icomoon.io/) and are licensed under GPL. Wherever third party code has been used, credit has been given in the code’s comments.

= Do you like Polylang? =

Don't hesitate to [give your feedback](http://wordpress.org/support/view/plugin-reviews/polylang#postform). It will help making the plugin better. Other [contributions](http://polylang.wordpress.com/documentation/contribute/) (such as new translations or helping other users on the support forum) are welcome !

== Installation ==

1. Make sure you are using WordPress 3.1 or later and that your server is running PHP5 (if you are using WordPress 3.2 or newer, it does !)
1. If you tried other multilingual plugins, deactivate them before activating Polylang, otherwise, you may get unexpected results !
1. Download the plugin
1. Extract all the files.
1. Upload everything (keeping the directory structure) to the `/wp-content/plugins/` directory.
1. Activate the plugin through the 'Plugins' menu in WordPress.
1. Go to the languages settings page and create the languages you need
1. Add the 'language switcher' widget to let your visitors switch the language.
1. Take care that your theme must come with the corresponding .mo files (Polylang downloads them for themes bundled with WordPress). If your theme is not internationalized yet, please refer to the [codex](http://codex.wordpress.org/I18n_for_WordPress_Developers#I18n_for_theme_and_plugin_developers) or ask the theme author to internationalize it.

== Frequently Asked Questions ==

= Where to find help ? =

* Read the [documentation](http://polylang.wordpress.com/documentation/). It includes [guidelines to start working with Polylang](http://polylang.wordpress.com/documentation/setting-up-a-wordpress-multilingual-site-with-polylang/), a [FAQ](http://polylang.wordpress.com/documentation/frequently-asked-questions/) and the [documentation for programmers](http://polylang.wordpress.com/documentation/documentation-for-developers/).
* Search the [support forum](http://wordpress.org/support/plugin/polylang). I know that searching in the WordPress forum is not very convenient, but please give it a try. You can use generic search engines such as Google too as the WordPress forum SEO is very good. You will most probably find your answer here.
* If you still have a problem, open a new thread in the [support forum](http://wordpress.org/support/plugin/polylang).

= How to contribute? =

See http://polylang.wordpress.com/documentation/contribute/

== Screenshots ==

1. The Polylang languages admin panel in WordPress 3.8

== Upgrade Notice ==

= 1.5.6 =
Polylang 1.2 introduced major internal changes. More than ever, make a database backup before upgrading from 1.1.6 or older! If you are using a version older than 0.8, please ugrade to 0.9.8 before ugrading to 1.5.6

== Changelog ==

= 1.5.6 (2014-10-11) =

* Fix: the admin language filter is not active for paginated taxonomy in nav menu admin panel
* Fix: wrong redirection if a domain is a substring of another domain (ex: mysite.com and mysite.co)
* Fix: impossible to translate numeric values in options defined in wpml-config.xml
* Fix: call to undefined method PLL_Links::get_translation_url() with Avada theme
* Fix: manage_{$this->screen->taxonomy}_custom_icolumn is a filter and not an action

= 1.5.5 (2014-09-10) =

* Fix: missing argument 4 in icl_translate
* Fix: conflict with Vantage theme
* Fix: possible issue with cookie domain on 'localhost'
* Fix: filtering string translations does not work when the group name contains a space
* Fix: Possible 404 error for attachments
* Fix: PHP notice when a shared term is not translated in all taxonomies

= 1.5.4 (2014-08-13) =

* Add new API functions: pll_get_post_language, pll_get_term_language, pll_translate_string
* Add better compatibility with Jetpack 3
* Fix: attachments don't get any language when uploaded from frontend
* Fix: authors cannot create tags
* Fix: too restrictive capability checks for some edge cases
* Fix: conflict with WPSEO: taxonomy metas cannot be saved

= 1.5.3 (2014-07-12) =

* Add: Capability check before creating links in post list table
* Add: Possibility not to cache languages objects with option PLL_CACHE_LANGUAGES (for GoDaddy users)
* Fix: Saving a header or a background in menu Appearance resets nav menus locations (introduced in 1.5)
* Fix: sub-sub-options and deeper levels defined in wpml-config.xml are not translated
* Fix: Fatal error when creating a new site when Polylang is network activated (introduced in v1.5.1)
* Fix: Admin language forced to English when activating Polylang (before creating any new language)
* Fix: 'pll_count_posts' second parameter not taken into account
* Fix: 'edit-post' and 'create-posts' capabilities are not differentiated when saving a post

= 1.5.2 (2014-06-24) =

* Fix: Revert post translations terms cleaning introduced in 1.5 as it seems to cause problems
* Fix: Impossible to delete a biographical info (introduced in 1.5)
* Fix: Security issue reported by [Gregory Viguier](http://www.screenfeed.fr/)

= 1.5.1 (2014-06-19) =

* Add: filter 'pll_settings_tabs' and action 'pll_settings_active_tab_{$tab}'
* Add: possibility to add a path when using multiple domains (same path for all languages)
* Fix: Bad redirection if /language/ is added to urls (introduced in 1.5)
* Fix: Nav menu locations are not saved in customizer (introduced in 1.4)
* Fix: Unable to unset nav menu locations
* Fix: Incorrect link for date archives in language switcher (introduced in 1.5)
* Fix: Fatal error when using featured content in Twenty Fourteen
* Fix: Posts bulk edit broken (introduced in 1.5)
* Fix: Polylang does not play nice with switch_to_blog
* Fix: Warning: reset() expects parameter 1 to be array, null given in admin-filters-columns.php on line 81

= 1.5 (2014-05-29) =

* Add Ukrainian translation contributed by [http://getvoip.com/](http://getvoip.com/)
* Refresh translation metaboxes (again): now translated posts are chosen from an autocomplete input field
* Categories and post tags translations are also chosen in an automplete input field
* Better error management on languages pages
* Use Dashicons instead of Icomoon icons for WP 3.8+
* Check if translated post is readable by the current user before displaying the language switcher
* Minimum Twenty Fourteen version is now 1.1
* Code cleaning
* Add support for Quick draft introduced in WP 3.8
* Add support for object cache plugins for recent posts and recent comments widgets
* Add support for pages with modified query in the language switcher (ex: when multiple post types queried on the same page)
* Add new API functions: pll_languages_list, pll_set_post_language, pll_set_term_language, pll_save_post_translations, pll_save_term_translations, pll_count_posts
* Add new filter pll_the_languages_args
* Add support for ICL_LANGUAGE_CODE == 'all' on admin side
* Fix: Galician flag
* Fix: static page on front pagination is broken
* Fix: search url may be broken
* Fix: PHP notice in icl_get_languages
* Fix: more robust way of detecting language in url when using directory
* Fix: delete translations terms orphans in database
* Fix: inconsistent behavior when setting page on front from customizer
* Fix: deleting a category assigns posts to wrong default category
* Fix: quick edit breaks synchronization
* Fix: some security issues


See changelog.txt for older changelog
