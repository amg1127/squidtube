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

$httpStatusCodes = array ('200', '203', '403', '404', '500', '503');

function xhrFact ($eventName) {
    return ('DEBUG:\\s*(|\\[QJS\\]\s+)XHR-' . strtoupper ($eventName) . '\\s*\\(');
}

function xhrProp ($propertyName, $propertyValue) {
    return ('DEBUG:\\s*(|\\[QJS\\]\s+)\\|\\s+' . str_replace ('.', '\\.', $propertyName) . '\\s*=\\s*' . $propertyValue . '\\s*\\(');
}

##################################################################
msg_log ("  +---+ Testing XHR implementation without body upload:");

msg_log ("  +---+---+ Synchronous mode for a successful request...");
foreach ($httpStatusCodes as $httpStatus) {
    $answer = ($answer && stdinSend (randomChannel (), '/sync/', $jsonData, 'number.zero', '==', '0', array ('response' => ($httpStatus . ' XHR test')), STDOUT_EXPECT_MATCH, array (xhrFact ('event-loadend'), xhrProp ('XHR.status', $httpStatus))));
}

msg_log ("  +---+---+ Synchronous mode for a failed request...");
$answer = ($answer && stdinSend (randomChannel (), $GLOBALS['invalidAddress'] . '/sync/', $jsonData, 'number.zero', '==', '0', array (), STDOUT_EXPECT_NOHELPER, xhrFact ('exception')));

msg_log ("  +---+---+ XHR 'onreadystatechange' event for a successful request...");
foreach ($httpStatusCodes as $httpStatus) {
    $answer = ($answer && stdinSend (randomChannel (), '/async/', $jsonData, 'number.zero', '==', '0', array ('response' => ($httpStatus . ' XHR test')), STDOUT_EXPECT_MATCH, array (
        xhrFact ('event-readystatechange'),
        xhrProp ('XHR.readyState', '1'),
        xhrFact ('event-readystatechange'),
        xhrProp ('XHR.readyState', '2'),
        xhrFact ('event-readystatechange'),
        xhrProp ('XHR.readyState', '3'),
        xhrFact ('event-readystatechange'),
        xhrProp ('XHR.readyState', '4')
    )));
}

msg_log ("  +---+---+ XHR 'onreadystatechange' event for a failed request...");
$answer = ($answer && stdinSend (randomChannel (), $GLOBALS['invalidAddress'] . '/async/', $jsonData, 'number.zero', '==', '0', array (), STDOUT_EXPECT_NOHELPER, array (
    xhrFact ('event-readystatechange'),
    xhrProp ('XHR.readyState', '1'),
    xhrFact ('event-readystatechange'),
    xhrProp ('XHR.readyState', '4')
)));

msg_log ("  +---+---+ XHR 'onloadend' event for a successful request...");
foreach ($httpStatusCodes as $httpStatus) {
    $answer = ($answer && stdinSend (randomChannel (), '/async/', $jsonData, 'number.zero', '==', '0', array ('response' => ($httpStatus . ' XHR test')), STDOUT_EXPECT_MATCH, array (
        xhrFact ('event-loadend'),
        xhrProp ('loaded', '\\d+'),
        xhrProp ('total', '\\d+'),
        xhrProp ('XHR.status', $httpStatus)
    )));
}

msg_log ("  +---+---+ XHR 'onloadend' event for a failed request...");
$answer = ($answer && stdinSend (randomChannel (), $GLOBALS['invalidAddress'] . '/async/', $jsonData, 'number.zero', '==', '0', array (), STDOUT_EXPECT_NOHELPER, array (
    xhrFact ('event-loadend'),
    xhrProp ('loaded', '\\d+'),
    xhrProp ('total', '\\d+'),
    xhrProp ('XHR.status', '0')
)));

msg_log ("  +---+---+ XHR 'onloadstart', 'onprogress' and 'onload' events...");
foreach ($httpStatusCodes as $httpStatus) {
    $answer = ($answer && stdinSend (randomChannel (), '/async/', $jsonData, 'number.zero', '==', '0', array ('response' => ($httpStatus . ' XHR test')), STDOUT_EXPECT_MATCH, array (
        xhrFact ('event-loadstart'),
        xhrProp ('loaded', '0'),
        xhrProp ('total', '0'),
        xhrFact ('event-progress'),
        xhrProp ('loaded', '\\d+'),
        xhrProp ('total', '\\d+'),
        xhrFact ('event-load'),
        xhrProp ('loaded', '\\d+'),
        xhrProp ('total', '\\d+')
    )));
}

msg_log ("  +---+---+ XHR 'onloadstart', 'onprogress' and 'onload' events using a big response content...");
foreach ($httpStatusCodes as $httpStatus) {
    msg_log ("  +---+---+---+ " . $httpStatus . "...");
    $answer = ($answer && stdinSend (randomChannel (), '/async/timeout/600000/', $jsonData, 'number.zero', '==', '0', array ('response' => ($httpStatus . ' XHR test'), 'bigdata' => 'true'), STDOUT_EXPECT_MATCH, array (
        xhrFact ('event-loadstart'),
        xhrProp ('loaded', '0'),
        xhrProp ('total', '0'),
        xhrFact ('event-progress'),
        xhrProp ('loaded', '\\d+'),
        xhrProp ('total', '\\d+'),
        xhrFact ('event-progress'),
        xhrProp ('loaded', '\\d+'),
        xhrProp ('total', '\\d+'),
        xhrFact ('event-progress'),
        xhrProp ('loaded', '\\d+'),
        xhrProp ('total', '\\d+'),
        xhrFact ('event-load'),
        xhrProp ('loaded', '\\d+'),
        xhrProp ('total', '\\d+')
    ), 300));
}

msg_log ("  +---+---+ XHR 'ontimeout' event...");
$answer = ($answer && stdinSend (randomChannel (), '/async/timeout/10000/', $jsonData, 'number.zero', '==', '0', array ('pause' => 15), STDOUT_EXPECT_NOHELPER, array (
    xhrFact ('event-readystatechange'),
    xhrProp ('XHR.readyState', '4'),
    xhrFact ('event-timeout'),
    xhrProp ('loaded', '0'),
    xhrProp ('total', '0'),
    xhrFact ('event-loadend'),
    xhrProp ('loaded', '0'),
    xhrProp ('total', '0')
)));

msg_log ("  +---+---+ XHR 'onabort' event...");
$answer = ($answer && stdinSend (randomChannel (), '/async/abort/10000/', $jsonData, 'number.zero', '==', '0', array ('pause' => 15), STDOUT_EXPECT_NOHELPER, array (
    xhrFact ('event-readystatechange'),
    xhrProp ('XHR.readyState', '4'),
    xhrFact ('event-abort'),
    xhrProp ('loaded', '0'),
    xhrProp ('total', '0'),
    xhrFact ('event-loadend'),
    xhrProp ('loaded', '0'),
    xhrProp ('total', '0')
)));

msg_log ("  +---+---+ XHR 'onerror' event...");
$answer = ($answer && stdinSend (randomChannel (), $GLOBALS['invalidAddress'] . '/async/', $jsonData, 'number.zero', '==', '0', array (), STDOUT_EXPECT_NOHELPER, array (
    xhrFact ('event-readystatechange'),
    xhrProp ('XHR.readyState', '4'),
    xhrFact ('event-error'),
    xhrProp ('loaded', '0'),
    xhrProp ('total', '0'),
    xhrFact ('event-loadend'),
    xhrProp ('loaded', '0'),
    xhrProp ('total', '0')
)));

msg_log ("  +---+---+ HTTP status 301 handling...");

msg_log ("  +---+---+ HTTP status 302 handling...");

msg_log ("  +---+---+ HTTP status 303 handling...");

msg_log ("  +---+---+ HTTP status 307 handling...");

msg_log ("  +---+---+ HTTP status 308 handling...");

##################################################################

msg_log ("  +---+ Testing XHR implementation with body upload:");

msg_log ("  +---+---+ Synchronous mode for a successful request...");
// TODO: test HTTP codes 200, 203, 403, 404, 500 and 503

msg_log ("  +---+---+ Synchronous mode for a failed request...");

msg_log ("  +---+---+ XHR 'onreadystatechange' event for a successful request...");
// TODO: test HTTP codes 200, 203, 403, 404, 500 and 503

msg_log ("  +---+---+ XHR 'onreadystatechange' event for a failed request...");

msg_log ("  +---+---+ XHR 'onloadend' event for a successful request...");

msg_log ("  +---+---+ XHR 'onloadend' event for a failed request...");

msg_log ("  +---+---+ XHR 'onloadstart', 'onprogress' and 'onload' events...");

msg_log ("  +---+---+ XHR 'onloadstart', 'onprogress' and 'onload' using with a big response content...");

msg_log ("  +---+---+ XHR 'ontimeout' event...");

msg_log ("  +---+---+ XHR 'onabort' event...");

msg_log ("  +---+---+ XHR 'onerror' event...");

msg_log ("  +---+---+ HTTP status 301 handling...");

msg_log ("  +---+---+ HTTP status 302 handling...");

msg_log ("  +---+---+ HTTP status 303 handling...");

msg_log ("  +---+---+ HTTP status 307 handling...");

msg_log ("  +---+---+ HTTP status 308 handling...");

##################################################################

return ($answer);
