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

msg_log ("  +---+---+ XHR 'onloadstart', 'onprogress' and 'onload' events while downloading large data volume...");
foreach ($httpStatusCodes as $httpStatus) {
    break;
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
// The test must sleep at this point because the the PHP server will not be
// answering new requests until the 'sleep' call from the previous request ends.
sleep (5);

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
// The test must sleep at this point because the the PHP server will not be
// answering new requests until the 'sleep' call from the previous request ends.
sleep (5);

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

$maxRedirects = 20;
$excess = 5;
$expectedMessages = array ();
foreach (array ($maxRedirects, $maxRedirects + $excess) as $pos) {
    $expectedMessages[$pos] = array_fill (0, $maxRedirects, 'DEBUG:\\s*\\[XHR#\\d+\\.\\d+\\]\\s*Performing\\s+a\\s+HTTP\\s+GET\\s+request\\s+against\\s+');
    array_unshift ($expectedMessages[$pos],
        xhrFact ('event-readystatechange'),
        xhrProp ('XHR.readyState', '1')
    );
    array_push ($expectedMessages[$pos],
        xhrProp ('XHR.readyState', '2'),
        xhrFact ('event-readystatechange'),
        xhrProp ('XHR.readyState', '3'),
        xhrFact ('event-readystatechange'),
        xhrProp ('XHR.readyState', '4'),
        xhrFact ('event-load'),
        xhrProp ('loaded', '\\d+'),
        xhrProp ('total', '\\d+'),
        xhrFact ('event-loadend'),
        xhrProp ('loaded', '\\d+'),
        xhrProp ('total', '\\d+'),
        xhrProp ('XHR.status', '200')
    );
}
array_splice ($expectedMessages[$maxRedirects + $excess], 3, 0, array ($expectedMessages[$maxRedirects + $excess][3]));

foreach (array ('301', '307', '308', '302', '303') as $redirectCode) {
    msg_log ("  +---+---+ HTTP status " . $redirectCode . " handling...");

    $answer = ($answer && stdinSend (randomChannel (), '/async/', $jsonData, 'number.zero', '==', '0', array ('response' => ($redirectCode . ' XHR Test'), 'redirects' => $maxRedirects), STDOUT_EXPECT_MATCH, $expectedMessages[$maxRedirects]));

    array_splice ($expectedMessages[$maxRedirects + $excess], -1, 1, array (xhrProp ('XHR.status', $redirectCode)));
    $answer = ($answer && stdinSend (randomChannel (), '/async/', $jsonData, 'number.zero', '==', '0', array ('response' => ($redirectCode . ' XHR Test'), 'redirects' => ($maxRedirects + $excess)), STDOUT_EXPECT_MATCH, $expectedMessages[$maxRedirects + $excess]));
}

##################################################################

msg_log ("  +---+ Testing XHR implementation with body upload:");

msg_log ("  +---+---+ Synchronous mode for a successful request...");
foreach ($httpStatusCodes as $httpStatus) {
    $answer = ($answer && stdinSend (randomChannel (), '/sync/', $jsonData, 'number.zero', '==', '0', array ('response' => ($httpStatus . ' XHR test')), STDOUT_EXPECT_MATCH, array (xhrFact ('event-loadend'), xhrProp ('XHR.status', $httpStatus))));
}

msg_log ("  +---+---+ Synchronous mode for a failed request...");

msg_log ("  +---+---+ XHR 'onreadystatechange' event for a successful request...");
// TODO: test HTTP codes 200, 203, 403, 404, 500 and 503

msg_log ("  +---+---+ XHR 'onreadystatechange' event for a failed request...");

msg_log ("  +---+---+ XHR 'onloadend' event for a successful request...");

msg_log ("  +---+---+ XHR 'onloadend' event for a failed request...");

msg_log ("  +---+---+ XHR 'onloadstart', 'onprogress' and 'onload' events...");

msg_log ("  +---+---+ XHR 'onloadstart', 'onprogress' and 'onload' events while downloading large data volume...");

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
