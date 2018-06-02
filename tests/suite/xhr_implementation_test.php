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

foreach (array ('without body' => false, 'with body' => true) as $testType => $bodyUploadFlag) {
    msg_log ("  +---+ Testing XHR implementation " . $testType . " upload:");
    if ($bodyUploadFlag) {
        $expectArray = array ('animal' => 'buffalo', 'plant' => 'fern', 'fungus' => 'mushroom');
    } else {
        $expectArray = array ();
    }

    msg_log ("  +---+---+ Synchronous mode for a successful request...");
    foreach ($httpStatusCodes as $httpStatus) {
        $answer = ($answer && stdinSend (randomChannel (), '/sync/success/', $jsonData, 'number.zero', '==', '0', array ('response' => ($httpStatus . ' XHR test'), 'expect' => $expectArray), STDOUT_EXPECT_MATCH, array (xhrFact ('event-loadend'), xhrProp ('XHR.status', $httpStatus))));
    }

    msg_log ("  +---+---+ Synchronous mode for a failed request...");
    $answer = ($answer && stdinSend (randomChannel (), $GLOBALS['invalidAddress'] . '/sync/fail/', $jsonData, 'number.zero', '==', '0', array ('expect' => $expectArray), STDOUT_EXPECT_NOHELPER, xhrFact ('exception')));

    msg_log ("  +---+---+ XHR 'onreadystatechange' event for a successful request...");
    foreach ($httpStatusCodes as $httpStatus) {
        $answer = ($answer && stdinSend (randomChannel (), '/async/onreadystatechange/success/', $jsonData, 'number.zero', '==', '0', array ('response' => ($httpStatus . ' XHR test'), 'expect' => $expectArray), STDOUT_EXPECT_MATCH, array (
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
    $answer = ($answer && stdinSend (randomChannel (), $GLOBALS['invalidAddress'] . '/async/onreadystatechange/fail/', $jsonData, 'number.zero', '==', '0', array ('expect' => $expectArray), STDOUT_EXPECT_NOHELPER, array (
        xhrFact ('event-readystatechange'),
        xhrProp ('XHR.readyState', '1'),
        xhrFact ('event-readystatechange'),
        xhrProp ('XHR.readyState', '4')
    )));

    msg_log ("  +---+---+ XHR 'onloadend' event for a successful request...");
    foreach ($httpStatusCodes as $httpStatus) {
        $answer = ($answer && stdinSend (randomChannel (), '/async/onloadend/success/', $jsonData, 'number.zero', '==', '0', array ('response' => ($httpStatus . ' XHR test'), 'expect' => $expectArray), STDOUT_EXPECT_MATCH, array_merge (
            (
                ($bodyUploadFlag) ? array (
                    xhrFact ('event-upload-loadend'),
                    xhrProp ('loaded', '\\d+'),
                    xhrProp ('total', '\\d+'),
                ) : array ()
            ), array (
                xhrFact ('event-loadend'),
                xhrProp ('loaded', '\\d+'),
                xhrProp ('total', '\\d+'),
                xhrProp ('XHR.status', $httpStatus)
            )
        )));
    }

    msg_log ("  +---+---+ XHR 'onloadend' event for a failed request...");
    $answer = ($answer && stdinSend (randomChannel (), $GLOBALS['invalidAddress'] . '/async/onloadend/fail/', $jsonData, 'number.zero', '==', '0', array ('expect' => $expectArray), STDOUT_EXPECT_NOHELPER, array_merge (
        (
            ($bodyUploadFlag) ? array (
                xhrFact ('event-upload-loadend'),
                xhrProp ('loaded', '\\d+'),
                xhrProp ('total', '\\d+'),
            ) : array ()
        ), array (
            xhrFact ('event-loadend'),
            xhrProp ('loaded', '\\d+'),
            xhrProp ('total', '\\d+'),
            xhrProp ('XHR.status', '0')
        )
    )));

    msg_log ("  +---+---+ XHR 'onloadstart', 'onprogress' and 'onload' events...");
    foreach ($httpStatusCodes as $httpStatus) {
        $answer = ($answer && stdinSend (randomChannel (), '/async/onloadstart/onprogress/onload/', $jsonData, 'number.zero', '==', '0', array ('response' => ($httpStatus . ' XHR test'), 'expect' => $expectArray), STDOUT_EXPECT_MATCH, array_merge (
            array (
                xhrFact ('event-loadstart'),
                xhrProp ('loaded', '0'),
                xhrProp ('total', '0')
            ), (
                ($bodyUploadFlag) ? array (
                    xhrFact ('event-upload-loadstart'),
                    xhrProp ('loaded', '0'),
                    xhrProp ('total', '\\d+'),
                    xhrFact ('event-upload-progress'),
                    xhrProp ('loaded', '\\d+'),
                    xhrProp ('total', '\\d+'),
                    xhrFact ('event-upload-load'),
                    xhrProp ('loaded', '\\d+'),
                    xhrProp ('total', '\\d+')
                ) : array ()
            ), array (
                xhrFact ('event-progress'),
                xhrProp ('loaded', '\\d+'),
                xhrProp ('total', '\\d+'),
                xhrFact ('event-load'),
                xhrProp ('loaded', '\\d+'),
                xhrProp ('total', '\\d+')
            )
        )));
    }

    msg_log ("  +---+---+ XHR 'onloadstart', 'onprogress' and 'onload' events while downloading large data volume...");
    $channel = randomChannel ();
    foreach ($httpStatusCodes as $httpStatus) {
        msg_log ("  +---+---+---+ " . $httpStatus . "...");
        $answer = ($answer && stdinSend ($channel, '/async/timeout/600000/onloadstart/onprogress/onload/', $jsonData, 'number.zero', '==', '0', array ('response' => ($httpStatus . ' XHR test'), 'bigdata' => 'true', 'expect' => $expectArray), STDOUT_EXPECT_MATCH, array_merge (
            array (
                xhrFact ('event-loadstart'),
                xhrProp ('loaded', '0'),
                xhrProp ('total', '0')
            ), (
                ($bodyUploadFlag) ? array (
                    xhrFact ('event-upload-loadstart'),
                    xhrProp ('loaded', '0'),
                    xhrProp ('total', '\\d+'),
                    xhrFact ('event-upload-progress'),
                    xhrProp ('loaded', '\\d+'),
                    xhrProp ('total', '\\d+'),
                    xhrFact ('event-upload-load'),
                    xhrProp ('loaded', '\\d+'),
                    xhrProp ('total', '\\d+')
                ) : array ()
            ), array (
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
            )
        ), 300));
    }

    msg_log ("  +---+---+ XHR 'ontimeout' event...");
    $answer = ($answer && stdinSend (randomChannel (), '/async/timeout/10000/', $jsonData, 'number.zero', '==', '0', array ('pause' => 15, 'expect' => $expectArray), STDOUT_EXPECT_NOHELPER, array_merge (
        (
            ($bodyUploadFlag) ? array (
                xhrFact ('event-upload-loadstart'),
                xhrProp ('loaded', '0'),
                xhrProp ('total', '\\d+'),
                xhrFact ('event-upload-progress'),
                xhrProp ('loaded', '\\d+'),
                xhrProp ('total', '\\d+'),
                xhrFact ('event-upload-load'),
                xhrProp ('loaded', '\\d+'),
                xhrProp ('total', '\\d+'),
                xhrFact ('event-upload-loadend'),
                xhrProp ('loaded', '\\d+'),
                xhrProp ('total', '\\d+')
            ) : array ()
        ), array (
            xhrFact ('event-readystatechange'),
            xhrProp ('XHR.readyState', '4'),
            xhrFact ('event-timeout'),
            xhrProp ('loaded', '0'),
            xhrProp ('total', '0'),
            xhrFact ('event-loadend'),
            xhrProp ('loaded', '0'),
            xhrProp ('total', '0')
        )
    )));
    // The test must sleep at this point because the the PHP server will not be
    // answering new requests until the 'sleep' call from the previous request ends.
    if ($answer) {
        sleep (5);
    }

    msg_log ("  +---+---+ XHR 'onabort' event...");
    $answer = ($answer && stdinSend (randomChannel (), '/async/abort/10000/', $jsonData, 'number.zero', '==', '0', array ('pause' => 15, 'expect' => $expectArray), STDOUT_EXPECT_NOHELPER, array_merge (
        (
            ($bodyUploadFlag) ? array (
                xhrFact ('event-upload-loadstart'),
                xhrProp ('loaded', '0'),
                xhrProp ('total', '\\d+'),
                xhrFact ('event-upload-progress'),
                xhrProp ('loaded', '\\d+'),
                xhrProp ('total', '\\d+'),
                xhrFact ('event-upload-load'),
                xhrProp ('loaded', '\\d+'),
                xhrProp ('total', '\\d+'),
                xhrFact ('event-upload-loadend'),
                xhrProp ('loaded', '\\d+'),
                xhrProp ('total', '\\d+')
            ) : array ()
        ), array (
            xhrFact ('event-readystatechange'),
            xhrProp ('XHR.readyState', '4'),
            xhrFact ('event-abort'),
            xhrProp ('loaded', '0'),
            xhrProp ('total', '0'),
            xhrFact ('event-loadend'),
            xhrProp ('loaded', '0'),
            xhrProp ('total', '0')
        )
    )));
    // The test must sleep at this point because the the PHP server will not be
    // answering new requests until the 'sleep' call from the previous request ends.
    if ($answer) {
        sleep (5);
    }

    msg_log ("  +---+---+ XHR 'onerror' event...");
    $answer = ($answer && stdinSend (randomChannel (), $GLOBALS['invalidAddress'] . '/async/onerror/', $jsonData, 'number.zero', '==', '0', array ('expect' => $expectArray), STDOUT_EXPECT_NOHELPER, array_merge (
        array (
            xhrFact ('event-readystatechange'),
            xhrProp ('XHR.readyState', '4')
        ), (
            ($bodyUploadFlag) ? array (
                xhrFact ('event-upload-error'),
                xhrProp ('loaded', '0'),
                xhrProp ('total', '0'),
                xhrFact ('event-upload-loadend'),
                xhrProp ('loaded', '0'),
                xhrProp ('total', '0')
            ) : array ()
        ), array (
            xhrFact ('event-error'),
            xhrProp ('loaded', '0'),
            xhrProp ('total', '0'),
            xhrFact ('event-loadend'),
            xhrProp ('loaded', '0'),
            xhrProp ('total', '0')
        )
    )));

    $maxRedirects = 20;
    $excess = 5;
    $redirectExcess = $maxRedirects + $excess;
    $expectedMessages = array ();
    foreach (array (1, 2, $maxRedirects, $redirectExcess) as $pos) {
        $expectedMessages[$pos] = array_fill (0, $maxRedirects, 'DEBUG:\\s*\\[XHR#\\d+\\.\\d+\\]\\s*Performing\\s+a\\s+HTTP\\s+' . (($bodyUploadFlag) ? 'POST' : 'GET') . '\\s+request\\s+against\\s+');
        array_unshift ($expectedMessages[$pos],
            xhrFact ('event-readystatechange'),
            xhrProp ('XHR.readyState', '1')
        );
        array_push ($expectedMessages[$pos],
            xhrFact ('event-readystatechange'),
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
    array_splice ($expectedMessages[1], 3, $maxRedirects - 1, array ());
    array_splice ($expectedMessages[2], 3, $maxRedirects - 1, array (str_replace ('POST', 'GET', $expectedMessages[2][3])));
    array_splice ($expectedMessages[2], -1, 1, array (xhrProp ('XHR.status', '500')));
    array_splice ($expectedMessages[$redirectExcess], 2, 0, array ($expectedMessages[$redirectExcess][2]));
    if ($bodyUploadFlag) {
        foreach (array_keys ($expectedMessages) as $pos) {
            array_splice ($expectedMessages[$pos], 2, 0, array (
                xhrFact ('event-upload-loadstart'),
                xhrProp ('loaded', '0'),
                xhrProp ('total', '\\d+')
            ));
            array_splice ($expectedMessages[$pos], 6, 0, array (
                xhrFact ('event-upload-progress'),
                xhrProp ('loaded', '\\d+'),
                xhrProp ('total', '\\d+'),
                xhrFact ('event-upload-load'),
                xhrProp ('loaded', '\\d+'),
                xhrProp ('total', '\\d+'),
                xhrFact ('event-upload-loadend'),
                xhrProp ('loaded', '\\d+'),
                xhrProp ('total', '\\d+')
            ));
        }
    }

    // Note from https://en.wikipedia.org/wiki/HTTP_301 :
    // "If the 301 status code is received in response to a request of any type other than GET or HEAD,
    // the client must ask the user before redirecting."
    foreach (array ('300' => false, '301' => false, '307' => false, '308' => false, '302' => true, '303' => true) as $redirectCode => $redirectMethodChange) {
        msg_log ("  +---+---+ HTTP status " . $redirectCode . " handling...");

        if ($bodyUploadFlag && $redirectMethodChange) {

            $answer = ($answer && stdinSend (randomChannel (), '/async/redirectMethodChange/', $jsonData, 'number.zero', '==', '0', array ('response' => ($redirectCode . ' XHR Test'), 'redirects' => $maxRedirects, 'expect' => $expectArray), STDOUT_EXPECT_NOHELPER, $expectedMessages[2]));

        } else if ($redirectCode == '300' || ($bodyUploadFlag && $redirectCode == '301')) {

            array_splice ($expectedMessages[1], -1, 1, array (xhrProp ('XHR.status', $redirectCode)));
            $answer = ($answer && stdinSend (randomChannel (), '/async/noRedirect/', $jsonData, 'number.zero', '==', '0', array ('response' => ($redirectCode . ' XHR Test'), 'redirects' => $maxRedirects, 'expect' => $expectArray), STDOUT_EXPECT_MATCH, $expectedMessages[1]));

        } else {

            $answer = ($answer && stdinSend (randomChannel (), '/async/maxRedirects/', $jsonData, 'number.zero', '==', '0', array ('response' => ($redirectCode . ' XHR Test'), 'redirects' => $maxRedirects, 'expect' => $expectArray), STDOUT_EXPECT_MATCH, $expectedMessages[$maxRedirects]));

            array_splice ($expectedMessages[$redirectExcess], -1, 1, array (xhrProp ('XHR.status', $redirectCode)));
            $answer = ($answer && stdinSend (randomChannel (), '/async/excessRedirects/', $jsonData, 'number.zero', '==', '0', array ('response' => ($redirectCode . ' XHR Test'), 'redirects' => ($redirectExcess), 'expect' => $expectArray), STDOUT_EXPECT_MATCH, $expectedMessages[$redirectExcess]));

        }
    }
}

##################################################################

return ($answer);
