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

jQuery(".newsletter_subscribe").submit(function(e) {
  
  e.preventDefault();
  
  var form = jQuery(this);
  var button = form.find("button");
  
  form.removeClass("wait success error");
  
  form.addClass("wait");
  
  button.prop('disabled', true);
  button.text("loading");
  
  jQuery.ajax({
    
    url: jQuery(this).attr("action"),
    type: "POST",
    data: jQuery(this).serialize(),
    success: function(response) {
      button.prop('disabled', false);
      button.text("Subscribe");
      form.find(".email").val("You are now subscribed!");
      form.removeClass("wait").addClass("success");
    }
  });
});

// DONATION //


jQuery('.donationwidget button').click(function(){
    jQuery('.donationwidget button').removeClass("clicked");
    jQuery(this).addClass("clicked");
});

jQuery(".donationwidget form").submit(function(e) {
  e.preventDefault();
  
  var form = jQuery(this);
  
  var button = form.find(".clicked");
  var amount = form.find(".amount").val();
  var currency = form.find(".radio:checked").val();
  
  if ( button.hasClass("paypal") ) {
    
  var paypallink = "https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=email@samnazarko.co.uk&item_name=OSMC%20Blog%20Donation&item_number=main_page_tracker&no_shipping=1&&no_note=1&tax=0&currency_code=" + currency + "&amount=" + amount
    
    window.open(paypallink);
    
  }
  if ( button.hasClass("stripe") ) {
    
    button.prop('disabled', true);
    button.addClass("loading");
    button.find("img").attr('src', 'https://osmc.tv/wp-content/themes/osmc/library/images/preloader.gif');
    var newamount = amount + "00";
    
    jQuery.getScript("https://checkout.stripe.com/checkout.js", function() {
      stripe(newamount, currency);
    });
  }
});

// STRIPE //


function stripe(am, cur) {
  var handler = StripeCheckout.configure({
    key: 'pk_live_HEfJk95fTFmjEBYMYVTxWFZk',
    image: 'wp-content/themes/osmc/library/images/favicons/apple-touch-icon-180x180.png'
  });
    
  // Open Checkout with further options
  handler.open({
    name: 'Stripe Donation',
    description: "",
    amount: am,
    currency: cur,
    opened: function() {
      var button = jQuery(".donationwidget form").find(".clicked");
      button.prop('disabled', false);
      button.removeClass("loading");
      button.find("img").attr('src', 'https://osmc.tv/wp-content/themes/osmc/library/images/stripe.png');
    }
  });
};

// Close Checkout on page navigation
jQuery(window).on('popstate', function() {
  handler.close();
});











