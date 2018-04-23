<?php

$jsonData = array (
    "true" => true,
    "false" => false
);
$answer = true;
$validOperators = array (
    "equal" => array ('', '=', '==', '-eq'),
    "notequal" => array ('!=', '<>', '-ne')
);
$invalidOperators = array ("<", "-lt", "<=", "-le", ">", "-gt", ">=", "-ge");
$expressions = array (
    "true" => array ('1', 'yes', 'true', '$true', 'on'),
    "false" => array ('0', 'no', 'false', '$false', 'off')
);

$answer = true;

##################################################################
msg_log ("  +---+ Testing valid operators...");

foreach ($jsonData as $evalString => $evalBoolean) {
    foreach (array ('', '! ') as $negateFlag) {
        foreach ($validOperators as $type => $operators) {
            foreach ($operators as $operator) {
                foreach ($jsonData as $criteriaString => $criteriaBoolean) {
                    foreach ($expressions[$criteriaString] as $expression) {
                        $expect = $evalBoolean;
                        if ($negateFlag != '') {
                            $expect = (! $expect);
                        }
                        if ($type != "equal") {
                            $expect = (! $expect);
                        }
                        if (! $criteriaBoolean) {
                            $expect = (! $expect);
                        }
                        $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, $evalString, $negateFlag . $operator, $expression));
                        if ($expect) {
                            $answer = ($answer && stdoutExpectMatch ());
                        } else {
                            $answer = ($answer && stdoutExpectNoMatch ());
                        }
                    }
                }
            }
        }
    }
}

##################################################################
msg_log ("  +---+ Testing invalid operators...");
foreach ($jsonData as $evalString => $evalBoolean) {
    foreach (array ('', '! ') as $negateFlag) {
        foreach ($invalidOperators as $operator) {
            foreach ($jsonData as $criteriaString => $criteriaBoolean) {
                foreach ($expressions[$criteriaString] as $expression) {
                    $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, $evalString, $negateFlag . $operator, $expression));
                    $answer = ($answer && stderrExpectInvalidOperator());
                    if ($negateFlag != '') {
                        $answer = ($answer && stdoutExpectMatch ());
                    } else {
                        $answer = ($answer && stdoutExpectNoMatch ());
                    }
                }
            }
        }
    }
}

##################################################################
return ($answer);
