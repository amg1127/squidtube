<?php

$answer = true;
$jsonData = array (
    "oddNumbers" => array (1, 3, 5, 7, 9),
    "evenNumbers" => array (0, 2, 4, 6, 8),
    "specialNumbers" => array (
        "numberOfTheBeast" => 666,
        "Avogadro" => 6.022e23,
        "Slipknot" => 742617000027,
        "MinusOne" => -1
    )
);

$channel = 0;
for ($i = 0; $i < 5; $i++) {
    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'oddNumbers.[]', '==', $jsonData['oddNumbers'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'evenNumbers.[]', '==', $jsonData['oddNumbers'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'evenNumbers.[]', '==', $jsonData['evenNumbers'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'oddNumbers.[]', '==', $jsonData['evenNumbers'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());
}

$channel = 0;
for ($i = 0; $i < 5; $i++) {
    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'oddNumbers.<>', '!=', $jsonData['oddNumbers'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'evenNumbers.<>', '!=', $jsonData['oddNumbers'][$i]));
    $answer = ($answer && stdoutExpectMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'evenNumbers.<>', '!=', $jsonData['evenNumbers'][$i]));
    $answer = ($answer && stdoutExpectNoMatch ());

    $answer = ($answer && stdinSend ($channel++, '/', $jsonData, 'oddNumbers.<>', '!=', $jsonData['evenNumbers'][$i]));
    $answer = ($answer && stdoutExpectMatch ());
}

return ($answer);
