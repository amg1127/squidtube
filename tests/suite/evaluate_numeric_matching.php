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

$mixCaseFlags = function ($param) {
    $answer = array ();
    foreach ($param as $i) {
        array_push ($answer, $i, '-i ' . $i, $i . ' -i', '--ignorecase ' . $i, $i . ' --ignorecase');
    }
    return ($answer);
};

##################################################################
msg_log ("  +---+ Testing equality...");

$ops = $mixCaseFlags (array ('=', '==', '-eq'));
$opsLength = count ($ops);
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectMatch ());
}

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne']));
$answer = ($answer && stdoutExpectMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne']));
$answer = ($answer && stdoutExpectNoMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo']));
$answer = ($answer && stdoutExpectMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo']));
$answer = ($answer && stdoutExpectNoMatch ());


##################################################################
msg_log ("  +---+ Testing inequality...");

$ops = $mixCaseFlags (array ('<>', '!=', '-ne'));
$opsLength = count ($ops);
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());
}

##################################################################
msg_log ("  +---+ Testing less-than relationship...");

$ops = $mixCaseFlags (array ('<', '-lt'));
$opsLength = count ($ops);
$st1 = $jsonData['numberSets']['setOne'][0];
$nd2 = $jsonData['numberSets']['setTwo'][0];
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $st1 = $st1 * 2));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $st1 = $st1 * 2));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $nd2 = $nd2 * 2));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $nd2 = $nd2 * 2));
    $answer = ($answer && stdoutExpectMatch ());
}

$st1 = $jsonData['numberSets']['setOne'][$numbersCount-1];
$nd2 = $jsonData['numberSets']['setTwo'][$numbersCount-1];
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', $ops[$step++ % $opsLength], $st1 = $st1 * 2));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', "! " . $ops[$step++ % $opsLength], $st1 = $st1 * 2));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', $ops[$step++ % $opsLength], $nd2 = $nd2 * 2));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', "! " . $ops[$step++ % $opsLength], $nd2 = $nd2 * 2));
    $answer = ($answer && stdoutExpectNoMatch ());
}

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$numbersCount / 2]));
$answer = ($answer && stdoutExpectMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$numbersCount / 2]));
$answer = ($answer && stdoutExpectNoMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$numbersCount / 2]));
$answer = ($answer && stdoutExpectMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$numbersCount / 2]));
$answer = ($answer && stdoutExpectNoMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne']));
$answer = ($answer && stdoutExpectNoMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne']));
$answer = ($answer && stdoutExpectMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo']));
$answer = ($answer && stdoutExpectNoMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo']));
$answer = ($answer && stdoutExpectMatch ());


##################################################################
msg_log ("  +---+ Testing greater-than relationship...");

$ops = $mixCaseFlags (array ('>', '-gt'));
$opsLength = count ($ops);
$st1 = $jsonData['numberSets']['setOne'][$numbersCount-1];
$nd2 = $jsonData['numberSets']['setTwo'][$numbersCount-1];
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $st1 = $st1 * 2));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $st1 = $st1 * 2));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $nd2 = $nd2 * 2));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $nd2 = $nd2 * 2));
    $answer = ($answer && stdoutExpectMatch ());
}

$st1 = $jsonData['numberSets']['setOne'][0];
$nd2 = $jsonData['numberSets']['setTwo'][0];
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', $ops[$step++ % $opsLength], $st1 = $st1 * 2));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', "! " . $ops[$step++ % $opsLength], $st1 = $st1 * 2));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', $ops[$step++ % $opsLength], $nd2 = $nd2 * 2));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', "! " . $ops[$step++ % $opsLength], $nd2 = $nd2 * 2));
    $answer = ($answer && stdoutExpectNoMatch ());
}

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$numbersCount / 2]));
$answer = ($answer && stdoutExpectMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$numbersCount / 2]));
$answer = ($answer && stdoutExpectNoMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$numbersCount / 2]));
$answer = ($answer && stdoutExpectMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$numbersCount / 2]));
$answer = ($answer && stdoutExpectNoMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne']));
$answer = ($answer && stdoutExpectNoMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne']));
$answer = ($answer && stdoutExpectMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo']));
$answer = ($answer && stdoutExpectNoMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo']));
$answer = ($answer && stdoutExpectMatch ());


##################################################################
msg_log ("  +---+ Testing less-than-or-equals relationship...");

$ops = $mixCaseFlags (array ('<=', '-le'));
$opsLength = count ($ops);
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectMatch ());
    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    if (($i+1) < $numbersCount) {
        $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], 0.5 * ($jsonData['numberSets']['setOne'][$i] + $jsonData['numberSets']['setOne'][$i+1])));
        $answer = ($answer && stdoutExpectMatch ());
        $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], 0.5 * ($jsonData['numberSets']['setTwo'][$i] + $jsonData['numberSets']['setTwo'][$i+1])));
        $answer = ($answer && stdoutExpectMatch ());
    }

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());
    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    if (($i+1) < $numbersCount) {
        $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], 0.5 * ($jsonData['numberSets']['setOne'][$i] + $jsonData['numberSets']['setOne'][$i+1])));
        $answer = ($answer && stdoutExpectNoMatch ());
        $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], 0.5 * ($jsonData['numberSets']['setTwo'][$i] + $jsonData['numberSets']['setTwo'][$i+1])));
        $answer = ($answer && stdoutExpectNoMatch ());
    }
}

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne']));
$answer = ($answer && stdoutExpectMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne']));
$answer = ($answer && stdoutExpectNoMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo']));
$answer = ($answer && stdoutExpectMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo']));
$answer = ($answer && stdoutExpectNoMatch ());


##################################################################
msg_log ("  +---+ Testing greater-than-or-equals relationship...");

$ops = $mixCaseFlags (array ('>=', '-ge'));
$opsLength = count ($ops);
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectMatch ());
    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    if (($i+1) < $numbersCount) {
        $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], 0.5 * ($jsonData['numberSets']['setOne'][$i] + $jsonData['numberSets']['setOne'][$i+1])));
        $answer = ($answer && stdoutExpectMatch ());
        $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], 0.5 * ($jsonData['numberSets']['setTwo'][$i] + $jsonData['numberSets']['setTwo'][$i+1])));
        $answer = ($answer && stdoutExpectMatch ());
    }

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());
    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    if (($i+1) < $numbersCount) {
        $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], 0.5 * ($jsonData['numberSets']['setOne'][$i] + $jsonData['numberSets']['setOne'][$i+1])));
        $answer = ($answer && stdoutExpectNoMatch ());
        $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], 0.5 * ($jsonData['numberSets']['setTwo'][$i] + $jsonData['numberSets']['setTwo'][$i+1])));
        $answer = ($answer && stdoutExpectNoMatch ());
    }
}

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne']));
$answer = ($answer && stdoutExpectMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne']));
$answer = ($answer && stdoutExpectNoMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo']));
$answer = ($answer && stdoutExpectMatch ());

$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.<>', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo']));
$answer = ($answer && stdoutExpectNoMatch ());


##################################################################
msg_log ("  +---+ Testing string comparison...");

$ops = array ('', '-w', '-f', '--wildcard', '--fixed'); $opsLength = count ($ops);
for ($i = 0; $answer && $i < $numbersCount; $i++) {
    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stderrExpectInvalidOperator() && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stderrExpectInvalidOperator() && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setOne.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setOne'][$i]));
    $answer = ($answer && stderrExpectInvalidOperator() && stdoutExpectMatch ());

    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numberSets.setTwo.[]', "! " . $ops[$step++ % $opsLength], $jsonData['numberSets']['setTwo'][$i]));
    $answer = ($answer && stderrExpectInvalidOperator() && stdoutExpectMatch ());
}

##################################################################
return ($answer);
