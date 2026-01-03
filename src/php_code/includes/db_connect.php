<?php

//debug harness
ini_set('display_errors', '0');          // do not show to browser
ini_set('log_errors', '1');              // do log
ini_set('error_reporting', E_ALL);

set_error_handler(function($severity, $message, $file, $line) {
    error_log("PHP ERROR: $message in $file:$line");
    return false; // let PHP handle it too
});

set_exception_handler(function(Throwable $e) {
    error_log("PHP EXCEPTION: ".$e->getMessage()." in ".$e->getFile().":".$e->getLine()."\n".$e->getTraceAsString());
    http_response_code(500);
    exit;
});
// end of debug harness

//Extremely important to set this right, otherwise the connection will fail and web page might not work at all.
$db_host = "localhost";               // Database Host
$db_user = "username";                   // Database User
$db_pass = "password";                 // Database Password
$db_name = "website_visitor_counter"; // Database Name
$conn = mysqli_connect($db_host, $db_user, $db_pass, $db_name); // Connect to Database
echo "connecting to website_visitor_counter";
if(!$conn) // Check connection
{
  die("Connection failed: " . mysqli_connect_error()); // Display error if not connected
}
else 
{
  echo "\nConnected successfully to website_visitor_counter";
}
?>
