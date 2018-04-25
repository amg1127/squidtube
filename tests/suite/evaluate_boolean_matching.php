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
    "true" => array ('1', 'yes', 'true', '$true', 'on', 'YES', 'TRUE', '$TRUE', 'ON', 'TrUe', '$tRuE', 'oN', 'On'),
    "false" => array ('0', 'no', 'false', '$false', 'off', 'NO', 'FALSE', '$FALSE', 'OFF', 'FaLsE', '$fAlSe', 'oFf', 'OfF')
);
$opposite = array (
    "true" => "false",
    "false" => "true"
);

$answer = true;

##################################################################
msg_log ("  +---+ Testing valid operators...");

foreach ($jsonData as $evalString => $evalBoolean) {
    foreach (array ('', '! ') as $negateFlag) {
        foreach (array ('', ' -i ', ' --ignorecase ') as $caseFlag) {
            foreach ($validOperators as $type => $operators) {
                foreach ($operators as $operator) {
                    foreach ($jsonData as $criteriaString => $criteriaBoolean) {
                        foreach ($expressions[$criteriaString] as $pos => $expression) {
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
                            $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, $evalString, $negateFlag . $operator . $caseFlag, $expression));
                            if ($expect) {
                                $answer = ($answer && stdoutExpectMatch ());
                            } else {
                                $answer = ($answer && stdoutExpectNoMatch ());
                            }
                            $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, $evalString, $negateFlag . $caseFlag . $operator, $expression));
                            if ($expect) {
                                $answer = ($answer && stdoutExpectMatch ());
                            } else {
                                $answer = ($answer && stdoutExpectNoMatch ());
                            }
                            $otherExpression = $expressions[$opposite[$criteriaString]][$pos];
                            $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, $evalString, $negateFlag . $operator . $caseFlag, array ($expression, $otherExpression)));
                            if ($negateFlag != '') {
                                $answer = ($answer && stdoutExpectNoMatch ());
                            } else {
                                $answer = ($answer && stdoutExpectMatch ());
                            }
                            $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, $evalString, $negateFlag . $caseFlag . $operator, array ($expression, $otherExpression)));
                            if ($negateFlag != '') {
                                $answer = ($answer && stdoutExpectNoMatch ());
                            } else {
                                $answer = ($answer && stdoutExpectMatch ());
                            }
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
        foreach (array ('', ' -i ', ' --ignorecase ') as $caseFlag) {
            foreach ($invalidOperators as $operator) {
                foreach ($jsonData as $criteriaString => $criteriaBoolean) {
                    foreach ($expressions[$criteriaString] as $expression) {
                        $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, $evalString, $negateFlag . $operator . $caseFlag, $expression));
                        $answer = ($answer && stderrExpectInvalidOperator());
                        if ($negateFlag != '') {
                            $answer = ($answer && stdoutExpectMatch ());
                        } else {
                            $answer = ($answer && stdoutExpectNoMatch ());
                        }
                        $answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, $evalString, $negateFlag . $caseFlag . $operator, $expression));
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
}

##################################################################
return ($answer);
