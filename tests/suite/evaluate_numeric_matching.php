<?php

$jsonData = array (
    "numberSets" => array (
        "setOne" => array ( -9, -7, -5, -3, -1, 1, 3, 5, 7, 9, sqrt (7919),      666, 742617000027),
        "setTwo" => array (-10, -8, -6, -4, -2, 0, 2, 4, 6, 8, sqrt    (2), 6.022e23, M_E),
    )
);
$step = 0;
$numbersCount = count($jsonData['numberSets']['setOne']);
$answer = ($numbersCount && $numbersCount == count($jsonData['numberSets']['setTwo']));
sort ($jsonData['numberSets']['setOne']);
sort ($jsonData['numberSets']['setTwo']);

##################################################################
msg_log ("  +---+ Testing equality...");

$channel = 0;
$ops = array ('=', '==', '-eq'); $opsLength = count ($ops);
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectMatch ());
    
    if ($channel > 50) {
        $channel = 0;
    }
}

##################################################################
msg_log ("  +---+ Testing inequality...");

$channel = 0;
$ops = array ('<>', '!=', '-ne'); $opsLength = count ($ops);
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());
    
    if ($channel > 50) {
        $channel = 0;
    }
}

##################################################################
msg_log ("  +---+ Testing less-than relationship...");

$channel = 0;
$ops = array ('<', '-lt'); $opsLength = count ($ops);
$st1 = $jsonData['numberSets']['setOne'][0];
$nd2 = $jsonData['numberSets']['setTwo'][0];
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $st1 = $st1 * 2));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $st1 = $st1 * 2));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $nd2 = $nd2 * 2));
    $answer = ($answer && stdoutExpectNoMatch ());
    
    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $nd2 = $nd2 * 2));
    $answer = ($answer && stdoutExpectMatch ());
    
    if ($channel > 50) {
        $channel = 0;
    }
}

$st1 = $jsonData['numberSets']['setOne'][$numbersCount-1];
$nd2 = $jsonData['numberSets']['setTwo'][$numbersCount-1];
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.<>', $ops[$step++ % $opsLength], $st1 = $st1 * 2));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.<>', "! " . $ops[$step++ % $opsLength], $st1 = $st1 * 2));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.<>', $ops[$step++ % $opsLength], $nd2 = $nd2 * 2));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.<>', "! " . $ops[$step++ % $opsLength], $nd2 = $nd2 * 2));
    $answer = ($answer && stdoutExpectNoMatch ());
    
    if ($channel > 50) {
        $channel = 0;
    }
}
$answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$numbersCount / 2]));
$answer = ($answer && stdoutExpectMatch ());

$answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$numbersCount / 2]));
$answer = ($answer && stdoutExpectNoMatch ());

$answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$numbersCount / 2]));
$answer = ($answer && stdoutExpectMatch ());

$answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$numbersCount / 2]));
$answer = ($answer && stdoutExpectNoMatch ());

##################################################################
msg_log ("  +---+ Testing greater-than relationship...");

$channel = 0;
$ops = array ('>', '-gt'); $opsLength = count ($ops);
$st1 = $jsonData['numberSets']['setOne'][$numbersCount-1];
$nd2 = $jsonData['numberSets']['setTwo'][$numbersCount-1];
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $st1 = $st1 * 2));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $st1 = $st1 * 2));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $nd2 = $nd2 * 2));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $nd2 = $nd2 * 2));
    $answer = ($answer && stdoutExpectMatch ());
    
    if ($channel > 50) {
        $channel = 0;
    }
}

$st1 = $jsonData['numberSets']['setOne'][0];
$nd2 = $jsonData['numberSets']['setTwo'][0];
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.<>', $ops[$step++ % $opsLength], $st1 = $st1 * 2));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.<>', "! " . $ops[$step++ % $opsLength], $st1 = $st1 * 2));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.<>', $ops[$step++ % $opsLength], $nd2 = $nd2 * 2));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.<>', "! " . $ops[$step++ % $opsLength], $nd2 = $nd2 * 2));
    $answer = ($answer && stdoutExpectNoMatch ());
    
    if ($channel > 50) {
        $channel = 0;
    }
}
$answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$numbersCount / 2]));
$answer = ($answer && stdoutExpectMatch ());

$answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$numbersCount / 2]));
$answer = ($answer && stdoutExpectNoMatch ());

$answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$numbersCount / 2]));
$answer = ($answer && stdoutExpectMatch ());

$answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$numbersCount / 2]));
$answer = ($answer && stdoutExpectNoMatch ());

##################################################################
msg_log ("  +---+ Testing less-than-or-equals relationship...");

$channel = 0;
$ops = array ('<=', '-le'); $opsLength = count ($ops);
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectMatch ());
    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    if (($i+1) < $numbersCount) {
        $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], 0.5 * ($jsonData['numberSets']['setOne'][$i] + $jsonData['numberSets']['setOne'][$i+1])));
        $answer = ($answer && stdoutExpectMatch ());
        $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], 0.5 * ($jsonData['numberSets']['setTwo'][$i] + $jsonData['numberSets']['setTwo'][$i+1])));
        $answer = ($answer && stdoutExpectMatch ());
    }

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());
    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    if (($i+1) < $numbersCount) {
        $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], 0.5 * ($jsonData['numberSets']['setOne'][$i] + $jsonData['numberSets']['setOne'][$i+1])));
        $answer = ($answer && stdoutExpectNoMatch ());
        $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], 0.5 * ($jsonData['numberSets']['setTwo'][$i] + $jsonData['numberSets']['setTwo'][$i+1])));
        $answer = ($answer && stdoutExpectNoMatch ());
    }
    
    if ($channel > 50) {
        $channel = 0;
    }
}

##################################################################
msg_log ("  +---+ Testing greater-than-or-equals relationship...");

$channel = 0;
$ops = array ('>=', '-ge'); $opsLength = count ($ops);
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectMatch ());
    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    if (($i+1) < $numbersCount) {
        $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], 0.5 * ($jsonData['numberSets']['setOne'][$i] + $jsonData['numberSets']['setOne'][$i+1])));
        $answer = ($answer && stdoutExpectMatch ());
        $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], 0.5 * ($jsonData['numberSets']['setTwo'][$i] + $jsonData['numberSets']['setTwo'][$i+1])));
        $answer = ($answer && stdoutExpectMatch ());
    }

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());
    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    if (($i+1) < $numbersCount) {
        $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], 0.5 * ($jsonData['numberSets']['setOne'][$i] + $jsonData['numberSets']['setOne'][$i+1])));
        $answer = ($answer && stdoutExpectNoMatch ());
        $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], 0.5 * ($jsonData['numberSets']['setTwo'][$i] + $jsonData['numberSets']['setTwo'][$i+1])));
        $answer = ($answer && stdoutExpectNoMatch ());
    }
    
    if ($channel > 50) {
        $channel = 0;
    }
}

##################################################################
msg_log ("  +---+ Testing string comparison...");

$channel = 0;
$ops = array ('', '-w', '-f', '-i'); $opsLength = count ($ops);
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stderrExpect ('(INFO|DEBUG):\\s*\\[\\w+#\\d+\\]\\s*Unable\\s+to\\s+apply\\s+selected\\s+comparison\\s+operator\\s+'));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stderrExpect ('(INFO|DEBUG):\\s*\\[\\w+#\\d+\\]\\s*Unable\\s+to\\s+apply\\s+selected\\s+comparison\\s+operator\\s+'));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stderrExpect ('(INFO|DEBUG):\\s*\\[\\w+#\\d+\\]\\s*Unable\\s+to\\s+apply\\s+selected\\s+comparison\\s+operator\\s+'));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stderrExpect ('(INFO|DEBUG):\\s*\\[\\w+#\\d+\\]\\s*Unable\\s+to\\s+apply\\s+selected\\s+comparison\\s+operator\\s+'));
    $answer = ($answer && stdoutExpectMatch ());
    
    if ($channel > 50) {
        $channel = 0;
    }
}

##################################################################
return ($answer);
