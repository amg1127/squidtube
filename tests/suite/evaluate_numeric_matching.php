<?php
/*
 * squidtube - An external Squid ACL class helper that provides control over access to videos
 * Copyright (C) 2018  Anderson M. Gomes
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

$jsonData = array (
    "numberSets" => array (
        "setOne" => array ( -9, -7, -5, -3, -1, 1, 3, 5, 7, 9, sqrt (7919),      666, 742617000027),
        "setTwo" => array (-10, -8, -6, -4, -2, 0, 2, 4, 6, 8, sqrt    (2), 6.022e23, M_E),
    )
);
$answer = true;
$numberSets = array ('setOne', 'setTwo');
$otherSet = array (
    'setOne' => 'setTwo',
    'setTwo' => 'setOne'
);
foreach ($numberSets as $numberSet) {
    sort ($jsonData['numberSets'][$numberSet]);
    $numbersCount = count ($jsonData['numberSets'][$numberSet]);
    $answer = ($answer && $numbersCount && count($jsonData['numberSets'][$otherSet[$numberSet]]) == $numbersCount);
}

##################################################################
msg_log ("  +---+ Testing equality...");

foreach (array ('=', '==', '-eq') as $op) {
    foreach (array ('', '-i', '--ignorecase') as $caseFlag) {
        foreach ($numberSets as $numberSet) {
            foreach ($jsonData['numberSets'][$numberSet] as $index => $testNumber) {
                $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.[]', $caseFlag . ' ' . $op, $testNumber, array(), STDOUT_EXPECT_MATCH));
                $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.[]', $caseFlag . ' ' . $op, $jsonData['numberSets'][$otherSet[$numberSet]][$index], array(), STDOUT_EXPECT_NOMATCH));
                if (! $answer) { break; }
            }
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.<>', $caseFlag . ' ' . $op, $jsonData['numberSets'][$numberSet], array(), STDOUT_EXPECT_MATCH));
            if (! $answer) { break; }
        }
        if (! $answer) { break; }
    }
    if (! $answer) { break; }
}

##################################################################
msg_log ("  +---+ Testing inequality...");

foreach (array ('<>', '!=', '-ne') as $op) {
    foreach (array ('', '-i', '--ignorecase') as $caseFlag) {
        foreach ($numberSets as $numberSet) {
            foreach ($jsonData['numberSets'][$numberSet] as $index => $testNumber) {
                $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.<>', $caseFlag . ' ' . $op, $testNumber, array(), STDOUT_EXPECT_NOMATCH));
                $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.<>', $caseFlag . ' ' . $op, $jsonData['numberSets'][$otherSet[$numberSet]][$index], array(), STDOUT_EXPECT_MATCH));
                if (! $answer) { break; }
            }
            if (! $answer) { break; }
        }
        if (! $answer) { break; }
    }
    if (! $answer) { break; }
}

##################################################################
msg_log ("  +---+ Testing less-than relationship...");

foreach (array ('<', '-lt') as $op) {
    foreach (array ('', '-i', '--ignorecase') as $caseFlag) {
        foreach ($numberSets as $numberSet) {
            $testMinNumber = min ($jsonData['numberSets'][$numberSet]);
            $testMaxNumber = max ($jsonData['numberSets'][$numberSet]);
            for ($i = 0; $answer && $i < 5; $i++) {
                $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.[]', $caseFlag . ' ' . $op, $testMinNumber = $testMinNumber * 2, array(), STDOUT_EXPECT_NOMATCH));
                $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.<>', $caseFlag . ' ' . $op, $testMaxNumber = $testMaxNumber * 2, array(), STDOUT_EXPECT_MATCH));
            }
            $testAvgNumber = 1.0 * array_sum ($jsonData['numberSets'][$numberSet]) / $numbersCount;
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.[]', $caseFlag . ' ' . $op, $testAvgNumber, array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.<>', $caseFlag . ' ' . $op, $jsonData['numberSets'][$numberSet], array(), STDOUT_EXPECT_NOMATCH));
            if (! $answer) { break; }
        }
        if (! $answer) { break; }
    }
    if (! $answer) { break; }
}

##################################################################
msg_log ("  +---+ Testing greater-than relationship...");

foreach (array ('>', '-gt') as $op) {
    foreach (array ('', '-i', '--ignorecase') as $caseFlag) {
        foreach ($numberSets as $numberSet) {
            $testMinNumber = min ($jsonData['numberSets'][$numberSet]);
            $testMaxNumber = max ($jsonData['numberSets'][$numberSet]);
            for ($i = 0; $answer && $i < 5; $i++) {
                $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.[]', $caseFlag . ' ' . $op, $testMaxNumber = $testMaxNumber * 2, array(), STDOUT_EXPECT_NOMATCH));
                $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.<>', $caseFlag . ' ' . $op, $testMinNumber = $testMinNumber * 2, array(), STDOUT_EXPECT_MATCH));
            }
            $testAvgNumber = 1.0 * array_sum ($jsonData['numberSets'][$numberSet]) / $numbersCount;
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.[]', $caseFlag . ' ' . $op, $testAvgNumber, array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.<>', $caseFlag . ' ' . $op, $jsonData['numberSets'][$numberSet], array(), STDOUT_EXPECT_NOMATCH));
            if (! $answer) { break; }
        }
        if (! $answer) { break; }
    }
    if (! $answer) { break; }
}

##################################################################
msg_log ("  +---+ Testing less-than-or-equals relationship...");

foreach (array ('<=', '-le') as $op) {
    foreach (array ('', '-i', '--ignorecase') as $caseFlag) {
        foreach ($numberSets as $numberSet) {
            for ($i = $numbersCount - 1; $answer && $i >= 0; $i--) {
                $testNumber = $jsonData['numberSets'][$numberSet][$i];
                $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.[]', $caseFlag . ' ' . $op, $testNumber, array(), STDOUT_EXPECT_MATCH));
                if ($i > 0) {
                    $testNumber += $jsonData['numberSets'][$numberSet][$i-1];
                    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.[]', $caseFlag . ' ' . $op, 0.5 * $testNumber, array(), STDOUT_EXPECT_MATCH));
                }
            }
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.[]', $caseFlag . ' ' . $op, $testNumber * 2, array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.<>', $caseFlag . ' ' . $op, $jsonData['numberSets'][$numberSet], array(), STDOUT_EXPECT_MATCH));
            if (! $answer) { break; }
        }
        if (! $answer) { break; }
    }
    if (! $answer) { break; }
}

##################################################################
msg_log ("  +---+ Testing greater-than-or-equals relationship...");

foreach (array ('>=', '-ge') as $op) {
    foreach (array ('', '-i', '--ignorecase') as $caseFlag) {
        foreach ($numberSets as $numberSet) {
            for ($i = 0; $answer && $i < $numbersCount; $i++) {
                $testNumber = $jsonData['numberSets'][$numberSet][$i];
                $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.[]', $caseFlag . ' ' . $op, $testNumber, array(), STDOUT_EXPECT_MATCH));
                if (($i+1) < $numbersCount) {
                    $testNumber += $jsonData['numberSets'][$numberSet][$i+1];
                    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.[]', $caseFlag . ' ' . $op, 0.5 * $testNumber, array(), STDOUT_EXPECT_MATCH));
                }
            }
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.[]', $caseFlag . ' ' . $op, $testNumber * 2, array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.<>', $caseFlag . ' ' . $op, $jsonData['numberSets'][$numberSet], array(), STDOUT_EXPECT_MATCH));
            if (! $answer) { break; }
        }
        if (! $answer) { break; }
    }
    if (! $answer) { break; }
}

##################################################################
msg_log ("  +---+ Testing string comparison...");

foreach (array ('', '-w', '-f', '--wildcard', '--fixed') as $op) {
    foreach (array ('', '-i', '--ignorecase') as $caseFlag) {
        foreach ($numberSets as $numberSet) {
            foreach ($jsonData['numberSets'][$numberSet] as $testNumber) {
                $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.[]', $caseFlag . ' ' . $op, $testNumber, array(), STDOUT_EXPECT_NOMATCH, STDERR_EXPECT_INVALID_COMPARISON_OPERATOR));
                if (! $answer) { break; }
            }
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'numberSets.' . $numberSet . '.[]', $caseFlag . ' ' . $op, $jsonData['numberSets'][$numberSet], array(), STDOUT_EXPECT_NOMATCH, STDERR_EXPECT_INVALID_COMPARISON_OPERATOR));
            if (! $answer) { break; }
        }
        if (! $answer) { break; }
    }
    if (! $answer) { break; }
}

##################################################################
return ($answer);
