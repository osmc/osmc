jQuery(function() {
    FastClick.attach(document.body);
});

jQuery("#nav-res-toggle").toggle(function(){
  
  jQuery(".top-nav").addClass("open");
  jQuery("#nav-res-toggle").addClass("open");
  
  console.log("yes");

}, function() {
  
  jQuery(".top-nav").removeClass("open");
  jQuery("#nav-res-toggle").removeClass("open");
  
});

// NEWSLETTER FORM //

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
    image: '/wp-content/themes/osmc/library/images/favicons/apple-touch-icon-180x180.png'
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

// JS TABLE //

var tablecount = 0;
var tableclass = "";

function diskimages(device, title) {
  
  tablecount = tablecount + 1;

  var url = "https://osmc.tv/citm.php?citm=installers/versions_" + device;
  var citm = "https://osmc.tv/citm.php?citm=";
  var div = jQuery(".disktables");
  
  if (tablecount % 2 == 0) {
  
    div.find("." + tableclass).append("<div class='table'><h3>" + title + "</h3><table class='" + device + "'><tr><th>Release</th><th>Checksum (MD5)</th></table></div>");
  
  } else {
  
    tableclass = "column" + tablecount;
    
    div.append("<div class='row " + tableclass + "'><div class='table'><h3>" + title + "</h3><table class='" + device + "'><tr><th>Release</th><th>Checksum (MD5)</th></table></div></div>");
  }
  
  var table = div.find("." + device);
  
  jQuery.ajax({
    url: url,
    type: "GET",
    success: function(response) {
      
      var array = response.split("\n");
      
      jQuery.each(array, function(index, value) {
        
        if ( value.length > 2 ) {
          
          table.append("<tr class='tr" + index + "'>");
        
          var name = value.substr(0, value.indexOf(" "));
          var address = value.split(" ").pop();
          var adrsplit = address.split("http://download.osmc.tv/").pop();
          var md5adr = citm + (adrsplit.substr(0, adrsplit.length-7)) + ".md5";
          
          jQuery.ajax({
            url: md5adr,
            type: "GET",
            success: function(response) {
              
              var md5 = response.slice(0, response.indexOf(" "));
              table.find(".tr" + index).append("<td><a href='" + address + "'>" + name + "</a></td><td>" + md5 + "</td>");
              
            }
          });
        }
        
      });
    }
  });
  
};