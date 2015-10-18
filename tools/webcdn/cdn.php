<?php

// If DEBUG is set to yes - no redirection will happen
$DEBUG = "no";

// Path of the CSV file. Full path on the OS in
// case this script is in one other directory
$CSVFILE = "mirrors.csv";

// FallBack URL - in case the mirrors.csv file is not accessible or empty.
$FALLBACK = "http://mirrorservice.org/sites/edge.samnazarko.co.uk/";


//=======================================================================
// Nothing to change below this point
//=======================================================================


// Remote IP as provided by WebClient to Server
$IPArray = explode(',', $_SERVER['HTTP_X_FORWARDED_FOR']);
$IP_tocheck=str_replace(' ', '', $IPArray[count($IPArray) -1]);

// Checking validity of IP
// Will remove reserved IP's -> local loopback
if (strlen($IP_tocheck) > 0) {
    if (filter_var($IP_tocheck, FILTER_VALIDATE_IP, FILTER_FLAG_NO_RES_RANGE)) {
        $IP['valid'] = htmlentities("$IP_tocheck", ENT_QUOTES, 'UTF-8');
        if ("$DEBUG" == "yes") {
            print ">>> Valid REMOTE IP {$IP['valid']} found<br />";
        }
        // Compute Continent out of IP.
        $continent = geoip_continent_code_by_name($IP['valid']);

    } else {
        $IP['invalid'] = htmlentities("$IP_tocheck", ENT_QUOTES, 'UTF-8');
        $continent = '';
        if ("$DEBUG" == "yes") {
            print "*** Invalid REMOTE IP {$IP['invalid']} found.<br />";
        }
    }
}
// Has to be set while developing - as RFC1918 or Reserved IP's won't return a continent
// $continent = 'EU'; 

$downloadurl = array();
$active = array();
$name = array();
$speed = array();
$cont = array();

// Perform some sanity checks on the CSV file
if (is_readable("$CSVFILE")) {
    if (filesize("$CSVFILE") > 0) {
        $readcsv = "read";
    } else {
        $readcsv = "skip";
    }
} else {
    $readcsv = "skip";
}

if ($readcsv == "read") {

    // Cycle through the csv-elements, and assign the content to their respective arrays.
    $csvline = array_map("str_getcsv", file($CSVFILE, FILE_SKIP_EMPTY_LINES));
    foreach ($csvline as $ikey => $data) {
        $key = $data['0']; // Set array key. Easier for later usage
        $active["$key"] = $data['1']; // link active tag to key
        $downloadurl["$key"] = $data['3']; // link download URL to key
        $name["$key"] = $data['2']; // link mirror name to key
        $speed["$key"] = $data['4']; // link speed to key
        $cont["$key"] = $data['5']; // link to continent
    } // Foreach loop
    
    // This previous foreach loop has provided us 5 arrays.
    // Active, Download URL, name of the Mirror, Speed and continent.
    
    // Speed ordered remote - hence fastest mirror is on top, while keeping the keys.
    arsort($speed);
    
    // We now want the mirror to be chosen according to:
    // 1. Speed
    // 2. Active
    // 3. Continent
    // In that order, as we ordered 

    // Seeking for Continent
    $chosen_key = "none";
    foreach ($speed as $key => $data) {
        // If that content is active, check for the continent
        if ($active["$key"] == 1) {
            if ($cont["$key"] == $continent) {
                // We have a match: Fastest on the list, Active and continent
                $chosen_key = $key;
            }
        } else {
            // Remove inactive elements from active keys
            // Needed later for random extraction (Fallback method).
            unset($active["$key"]);
        }
    } // Foreach
    
    // Handle the exception - If we found nothing, take a randome entry.
    if ("$chosen_key" == "none") {
        $chosen_key = array_rand($active, 1);
        if ("$DEBUG" == "yes") {
            print "*** No entry found - falling back to random. <br />";
        }
    }

    // Make a simple variable out of result
    $downurl = $downloadurl["$chosen_key"];

} else {
// Something is wrong with the file. Hardcode the required data here.
    if ("$DEBUG" == "yes") {
        print "*** $CSVFILE not accessible or emtpy. Falling back to failsafe<br />";
    }

    $downurl = $FALLBACK;
}

//redirect
if ("$DEBUG" == "yes") {
    print ">>> redirecting to: {$downurl}{$_SERVER['REQUEST_URI']}";
} else {
    $url = $downurl . $_SERVER['REQUEST_URI'];
    header("location: $url");
}
exit;
?>
