jQuery(function() {
    FastClick.attach(document.body);
});

jQuery(".top-nav").addClass("nav-res");

jQuery("#nav-res-toggle").toggle(function(){

	jQuery(".top-nav.nav-res").removeClass("animated fadeOutRight").addClass("animated fadeInRight").css('display','block');
	jQuery("#nav-res-toggle, .top-nav.nav-res a").css('color','#F0F0F0');

}, function() {
	jQuery(".top-nav.nav-res").removeClass("animated fadeInRight").addClass("animated fadeOutRight");
	jQuery("#nav-res-toggle").css('color','#F0F0F0');
	setTimeout(function(){
    jQuery(".top-nav.nav-res").css('display','none')}, 1000);
});

jQuery(window).resize(function(){
	if(window.innerWidth > 767) {
		jQuery(".top-nav.nav-res, .top-nav.nav-res a").removeAttr("style");
		jQuery("#nav-res-toggle").removeAttr("style");
		jQuery(".top-nav.nav-res").removeClass("animated fadeInRight fadeOutRight")
	}
});