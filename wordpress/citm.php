<?php
$ch = curl_init();
curl_setopt($ch, CURLOPT_URL, "http://173.255.206.49/osmc/download/" . $_GET["citm"]);
curl_setopt($ch, CURLOPT_HEADER, 0);
curl_setopt($ch, CURLOPT_CONNECTTIMEOUT , 20);
curl_setopt($ch, CURLOPT_TIMEOUT, 20); //timeout in seconds
curl_exec($ch);
curl_close($ch);
?>
