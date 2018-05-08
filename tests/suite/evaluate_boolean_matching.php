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
    foreach (array ('', ' -i ', ' --ignorecase ') as $caseFlag) {
        foreach ($validOperators as $type => $operators) {
            foreach ($operators as $operator) {
                foreach ($jsonData as $criteriaString => $criteriaBoolean) {
                    foreach ($expressions[$criteriaString] as $pos => $expression) {
                        $expect = $evalBoolean;
                        if ($type != "equal") {
                            $expect = (! $expect);
                        }
                        if (! $criteriaBoolean) {
                            $expect = (! $expect);
                        }
                        if ($expect) {
                            $expect = STDOUT_EXPECT_MATCH;
                        } else {
                            $expect = STDOUT_EXPECT_NOMATCH;
                        }
                        $answer = $answer && matchingTest (randomChannel (), '/', $jsonData, $evalString, $operator . $caseFlag, $expression, array (), $expect);
                        $otherExpression = $expressions[$opposite[$criteriaString]][$pos];
                        $answer = $answer && matchingTest (randomChannel (), '/', $jsonData, $evalString, $operator . $caseFlag, array ($expression, $otherExpression), array (), STDOUT_EXPECT_MATCH);
                    }
                }
            }
        }
    }

}

##################################################################
msg_log ("  +---+ Testing invalid operators...");
foreach ($jsonData as $evalString => $evalBoolean) {
    foreach (array ('', ' -i ', ' --ignorecase ') as $caseFlag) {
        foreach ($invalidOperators as $operator) {
            foreach ($jsonData as $criteriaString => $criteriaBoolean) {
                foreach ($expressions[$criteriaString] as $expression) {
                    $answer = $answer && matchingTest (randomChannel (), '/', $jsonData, $evalString, $operator . $caseFlag, $expression, array (), STDOUT_EXPECT_NOMATCH, STDERR_EXPECT_INVALID_COMPARISON_OPERATOR);
                }
            }
        }
    }
}

##################################################################
return ($answer);
