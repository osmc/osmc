<p><?php
printf(
	__('Polylang is provided with an extensive %sdocumentation%s (in English only). It includes information on how to set up your multilingual site and use it on a daily basis, a FAQ, as well as a documentation for programmers to adapt their plugins and themes.', 'polylang'),
	'<a href="http://polylang.wordpress.com/documentation/">',
	'</a>'
);
echo ' ';
printf(
	__("You will also find useful information in the %ssupport forum%s. However don't forget to make a search before posting a new topic.", 'polylang'),
	'<a href="http://wordpress.org/support/plugin/polylang">',
	'</a>'
);?>
</p>
<p><?php
printf(
	__('Polylang is free of charge and is released under the same license as WordPress, the %sGPL%s.', 'polylang'),
	'<a href="http://wordpress.org/about/gpl/">',
	'</a>'
);
echo ' ';
printf(
	__('If you wonder how you can help the project, just %sread this%s.', 'polylang'),
	'<a href="http://polylang.wordpress.com/documentation/contribute/">',
	'</a>'
);
echo ' ';
_e('Finally if you like this plugin or if it helps your business, donations to the author are greatly appreciated.', 'polylang')?>
</p>
<form action="https://www.paypal.com/cgi-bin/webscr" method="post">
	<input type="hidden" name="cmd" value="_s-xclick">
	<input type="hidden" name="hosted_button_id" value="CCWWYUUQV8F4E">
	<input type="image" src="https://www.paypalobjects.com/en_US/i/btn/btn_donate_SM.gif" border="0" name="submit" alt="PayPal - The safer, easier way to pay online!">
	<img alt="" border="0" src="https://www.paypalobjects.com/en_US/i/scr/pixel.gif" width="1" height="1">
</form>
