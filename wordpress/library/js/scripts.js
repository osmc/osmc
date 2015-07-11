jQuery(function () {
  FastClick.attach(document.body);
});

jQuery("#nav-res-toggle").click(function () {

  if (jQuery(this).hasClass("open"))  {
    jQuery(".top-nav").removeClass("open");
    jQuery("#nav-res-toggle").removeClass("open");
  } else  {
    jQuery(".top-nav").addClass("open");
    jQuery("#nav-res-toggle").addClass("open");
  }

});

// FRONT PAGE //

// Video

if (jQuery("body").hasClass("home")) {

  var player = new Clappr.Player({
    source: 'https://osmc.tv/homepage_tour.mp4',
    poster: 'https://osmc.tv/wp-content/themes/osmc/library/images/video-poster.png',
    preload: 'none',
    loop: 'true',
    width: '100%',
    height: '100%',
    parentId: '#player',
    chromeless: 'true'
  });

  jQuery(".home .vignette-overlay").click(function ()  {

    var vid = jQuery("body.home .video-wrap video").get(0);
    var play = jQuery("body.home .video-wrap button.media-control-button[data-playpause]");
    var overlay = jQuery("body.home .video-overlay");
    var icon = jQuery("body.home .playicon");

    if (vid.paused) {
      play.click();
      overlay.addClass("hidden");
      icon.addClass("hidden");
    } else {
      play.click();
      overlay.removeClass("hidden");
      icon.removeClass("hidden");
    }

  });


  // set height 100%

  var hwindow = jQuery(window).height();
  var h130 = jQuery(window).height() * 1.55;
  jQuery(".home .firstn").css("height", hwindow);
  setTimeout(function () {
    jQuery(".home .firstn-wrap .full").addClass("show");
  }, 100);
  jQuery(".home .firstn-back").css("height", h130);

  // images

  currentZ = 1;
  currentImg = 1;

  jQuery(".home .thirdn li").click(function ()  {
    var lclass = jQuery(this).attr("class");
    var nr = lclass.substr(lclass.length - 1);

    if (jQuery.isNumeric(nr)) {

      jQuery(".home .thirdn img").removeClass("show");
      jQuery(".home .thirdn .img-wrap" + currentImg + " img").addClass("show");
      jQuery(".home .thirdn .img-wrap" + nr).css("z-index", currentZ + 1);
      jQuery(".home .thirdn .img-wrap" + nr + " img").addClass("show");
      jQuery(".home .thirdn li").removeClass("show");
      jQuery(".home .thirdn li.link" + nr).addClass("show");

      oldImg = currentImg;
      currentImg = nr;

      setTimeout(function () {
        jQuery(".home .thirdn .img-wrap" + oldImg + " img").removeClass("show");
      }, 400);

    }
    currentZ = currentZ + 1;

  });

};

// NEWSLETTER FORM //

jQuery(".newsletter_subscribe").submit(function (e) {

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
    success: function (response)  {
      button.prop('disabled', false);
      button.text("Subscribe");
      form.find(".email").val("You are now subscribed!");
      form.removeClass("wait").addClass("success");
    }
  });
});

// DONATION //

jQuery(".donationwidget button").click(function () {
  jQuery('.donationwidget button').removeClass("clicked");
  jQuery(this).addClass("clicked");
});

jQuery.each(jQuery(".donationwidget form"), function (index, oneForm)  {  
  jQuery(oneForm).validate({
    rules: {
      amount: {
        required: true,
        digits: true
      }
    },
    submitHandler: function () {
      var form = jQuery(oneForm);

      var button = form.find(".clicked");
      var amount = form.find(".amount").val();
      var currency = form.find(".radio:checked").val();

      if (button.hasClass("paypal"))  {
        
        var currentUrl = window.location.host + window.location.pathname;
        var paypallink = "https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=email@samnazarko.co.uk&item_name=OSMC%20Blog%20Donation&item_number=" + currentUrl + "&no_shipping=1&&no_note=1&tax=0&currency_code=" + currency + "&amount=" + amount;

        window.open(paypallink);

      }
      if (button.hasClass("stripe")) {

        button.prop('disabled', true);
        button.addClass("loading");
        button.find(".svg").addClass("hidden");
        button.append('<img src="https://osmc.tv/wp-content/themes/osmc/library/images/preloader.gif">');
        var newamount = amount + "00";

        jQuery.getScript("https://checkout.stripe.com/checkout.js", function () {
          stripe(newamount, currency);
        });
      }
    }
  });
});

// POPUP //

// check hash on load
var url_load = document.URL.substr(document.URL.indexOf('#') + 1);
if (url_load === "donate") {
  popup_donate();
};

// check hash on change
jQuery(window).on('hashchange', function () {
  var hash = location.hash.slice(1);
  if (hash == "donate") {
    popup_donate();
  }
});

function popup_donate() {
  var overlay = jQuery(".overlay");
  var popup = jQuery(".popup_donate");
  overlay.addClass("show");
  popup.addClass("show");

  setTimeout(function () {
    overlay.addClass("fade");
    popup.addClass("fade");
  }, 100);

};

jQuery(".overlay").click(function () {
  jQuery(".popup_donate").removeClass("show fade");
  jQuery(".overlay").removeClass("show fade");
  window.location.hash = "exit";
});

// STRIPE //

function stripe(am, cur) {
  var handler = StripeCheckout.configure({
    key: 'pk_live_HEfJk95fTFmjEBYMYVTxWFZk',
    image: '/wp-content/themes/osmc/library/images/favicons/apple-touch-icon-180x180.png',
    token: function (token) {
      window.location.href = "https://osmc.tv/contribute/donate/thanks/";
    }
  });

  // Open Checkout with further options
  handler.open({
    name: 'OSMC Donation',
    description: "",
    amount: am,
    currency: cur,
    opened: function () {
      var button = jQuery(".donationwidget form").find(".clicked");
      button.prop('disabled', false);
      button.removeClass("loading");
      button.find(".svg").removeClass("hidden");
      button.find("img").remove();
    }
  });
};

// Close Checkout on page navigation
jQuery(window).on('popstate', function () {
  handler.close();
});

// DOWNLOAD SCROLL TO //

jQuery(".download.devices .wrapper").click(function () {
  jQuery('html, body').animate({
    scrollTop: jQuery(".getstarted").offset().top - 40
  }, 800);
});

// CHARTIST.JS

function pieChart(items) {
  
  var nr = 0;
  var sum = function(a, b) { return a + b };
  
  var data = {
    names: [],
    series: []
  }
    
  for (i = 0; i < items.length; i++ ) {
    if (i % 2 === 0) {
      data.names.push(items[i]);
    } else {
      data.series.push(items[i]);
    }
  }
  
  var options = {
    labelInterpolationFnc: function(value) {
      var math = parseFloat((value / data.series.reduce(sum) * 100)).toFixed(2) + '%';
      if ( (nr >= data.names.length) == false ) {
        pieLegend(nr, math);
        nr += 1;
      }
      return math;
    },
    chartPadding: 0,
    labelOffset: 15,
  };
    
  Chartist.Pie('.ct-chart', data, options);

  jQuery(".ct-chart").after('<div class="ct-list"></div>');
  var legend = jQuery(".ct-list");
  
  function pieLegend(i, calc) {
    var listItem = "<button class='ct-series-" + i + "'>" + data.names[i] + " - " + calc + "</button>";
    legend.append(listItem);
  };
  
};

// JS TABLE //

var tablecount = 0;
var tableclass = "";

function diskimages(device, title)  {

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
    success: function (response)  {

      var array = response.split("\n");

      jQuery.each(array, function (index, value) {

        if (value.length > 2) {

          table.append("<tr class='tr" + index + "'>");

          var name = value.substr(0, value.indexOf(" "));
          var address = value.split(" ").pop();
          var adrsplit = address.split("http://download.osmc.tv/").pop();
          var md5adr = citm + (adrsplit.substr(0, adrsplit.length - 7)) + ".md5";

          jQuery.ajax({
            url: md5adr,
            type: "GET",
            success: function (response)  {

              var md5 = response.slice(0, response.indexOf(" "));
              table.find(".tr" + index).append("<td><a href='" + address + "'>" + name + "</a></td><td>" + md5 + "</td>");

            }
          });
        }

      });
    }
  });

};