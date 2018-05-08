<?php

$answer = true;
$jsonData = array (
    'number' => array (
        'negative' => -4,
        'zero' => 0,
        'positive' => 4
    ), 'string' => array (
        'lower' => 'bear',
        'upper' => 'BULL'
    ), 'boolean' => array (
        'true'  => true,
        'false' => false
    )
);

##################################################################
msg_log ("  +---+ Testing XHR implementation without body upload:");

msg_log ("  +---+---+ Synchronous mode for a successful request...");
// TODO: test HTTP codes 200, 204, 403, 404, 500 and 503
foreach (array (200, 204, 403, 404, 500, 503) as $httpStatus) {
    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'number.zero', '==', '0', array ('response' => $httpStatus), STDOUT_EXPECT_MATCH, array (STDERR_EXPECT_XHR_EVENT_LOADEND, sprintf (STDERR_EXPECT_XHR_STATUS_format, $httpStatus))));
}

msg_log ("  +---+---+ Synchronous mode for a failed request...");

msg_log ("  +---+---+ XHR 'onreadystatechange' event for a successful request...");
// TODO: test HTTP codes 200, 204, 403, 404, 500 and 503

msg_log ("  +---+---+ XHR 'onreadystatechange' event for a failed request...");

msg_log ("  +---+---+ XHR 'onloadend' event for a successful request...");

msg_log ("  +---+---+ XHR 'onloadend' event for a failed request...");

msg_log ("  +---+---+ XHR 'onloadstart', 'onprogress' and 'onload' events...");

msg_log ("  +---+---+ XHR 'onloadstart', 'onprogress' and 'onload' events using a big response content...");

msg_log ("  +---+---+ XHR 'ontimeout' event...");

msg_log ("  +---+---+ XHR 'onabort' event...");

msg_log ("  +---+---+ XHR 'onerror' event...");

msg_log ("  +---+---+ XHR 'onerror' event...");

msg_log ("  +---+---+ HTTP status 301 handling...");

msg_log ("  +---+---+ HTTP status 302 handling...");

msg_log ("  +---+---+ HTTP status 303 handling...");

msg_log ("  +---+---+ HTTP status 307 handling...");

msg_log ("  +---+---+ HTTP status 308 handling...");

##################################################################

msg_log ("  +---+ Testing XHR implementation with body upload:");

msg_log ("  +---+---+ Synchronous mode for a successful request...");
// TODO: test HTTP codes 200, 204, 403, 404, 500 and 503

msg_log ("  +---+---+ Synchronous mode for a failed request...");

msg_log ("  +---+---+ XHR 'onreadystatechange' event for a successful request...");
// TODO: test HTTP codes 200, 204, 403, 404, 500 and 503

msg_log ("  +---+---+ XHR 'onreadystatechange' event for a failed request...");

msg_log ("  +---+---+ XHR 'onloadend' event for a successful request...");

msg_log ("  +---+---+ XHR 'onloadend' event for a failed request...");

msg_log ("  +---+---+ XHR 'onloadstart', 'onprogress' and 'onload' events...");

msg_log ("  +---+---+ XHR 'onloadstart', 'onprogress' and 'onload' using with a big response content...");

msg_log ("  +---+---+ XHR 'ontimeout' event...");

msg_log ("  +---+---+ XHR 'onabort' event...");

msg_log ("  +---+---+ XHR 'onerror' event...");

msg_log ("  +---+---+ XHR 'onerror' event...");

msg_log ("  +---+---+ HTTP status 301 handling...");

msg_log ("  +---+---+ HTTP status 302 handling...");

msg_log ("  +---+---+ HTTP status 303 handling...");

msg_log ("  +---+---+ HTTP status 307 handling...");

msg_log ("  +---+---+ HTTP status 308 handling...");

##################################################################

return ($answer);
