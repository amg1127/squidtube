<?php

$jsonData = array (
    "stringSets" => array (
        "setUpper"  => array ('HOH-TEL', 'OSS-CAH', 'LEE-MAH', 'YANG-KEY'),
        "setLower"  => array ('dell-tah', 'in-dee-ah', 'vik-tah', 'eck-oh', 'row-me-oh')
    )
);
$stringSetNames = array ("setUpper", "setLower");
$invertCallbacks = array ("setUpper" => "strtolower", "setLower" => "strtoupper");
$step = 0;
$answer = true;

##################################################################
msg_log ("  +---+ Testing equality...");

foreach (array ('=', '==', '-eq') as $op) {
    foreach ($stringSetNames as $stringSetName) {

        for ($i = 0, $length = count ($jsonData['stringSets'][$stringSetName]); $answer && $i < $length; $i++) {
            $testCriteria = $jsonData['stringSets'][$stringSetName][$i];
            $testCriteriaInvertedCase = call_user_func ($invertCallbacks[$stringSetName], $testCriteria);
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $op, $testCriteria, array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $op, $testCriteriaInvertedCase, array(), STDOUT_EXPECT_NOMATCH));
            foreach (array ('-i', '--ignorecase') as $ignoreCaseFlag) {
                $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, $testCriteria, array(), STDOUT_EXPECT_MATCH));
                if (! $answer) { break; }
            }
        }

        $testCriteria = $jsonData['stringSets'][$stringSetName];
        $testCriteriaInvertedCase = array_map ($invertCallbacks[$stringSetName], $testCriteria);
        $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $op, $testCriteria, array(), STDOUT_EXPECT_MATCH));
        $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $op, $testCriteriaInvertedCase, array(), STDOUT_EXPECT_NOMATCH));
        foreach (array ('-i', '--ignorecase') as $ignoreCaseFlag) {
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, $testCriteriaInvertedCase, array(), STDOUT_EXPECT_MATCH));
            if (! $answer) { break; }
        }

        if (! $answer) { break; }
    }
    if (! $answer) { break; }
}

##################################################################
msg_log ("  +---+ Testing inequality...");

foreach (array ('<>', '!=', '-ne') as $op) {
    foreach ($stringSetNames as $stringSetName) {

        for ($i = 0, $length = count ($jsonData['stringSets'][$stringSetName]); $answer && $i < $length; $i++) {
            $testCriteria = $jsonData['stringSets'][$stringSetName][$i];
            $testCriteriaInvertedCase = call_user_func ($invertCallbacks[$stringSetName], $testCriteria);
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $op, $testCriteria, array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $op, $testCriteriaInvertedCase, array(), STDOUT_EXPECT_MATCH));
            foreach (array ('-i', '--ignorecase') as $ignoreCaseFlag) {
                $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, $testCriteria, array(), STDOUT_EXPECT_NOMATCH));
                if (! $answer) { break; }
            }
        }

        if (! $answer) { break; }
    }
    if (! $answer) { break; }
}

##################################################################
msg_log ("  +---+ Testing less-than relationship...");

foreach (array ('<', '-lt') as $op) {

    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, 'ZZZZ', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, 'aaaa', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, 'NNNN', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.[]', $op, 'NNNN', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, 'nnnn', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.[]', $op, 'nnnn', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, min ($jsonData['stringSets']['setUpper']), array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, max ($jsonData['stringSets']['setUpper']), array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.[]', $op, min ($jsonData['stringSets']['setUpper']), array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.[]', $op, max ($jsonData['stringSets']['setUpper']), array(), STDOUT_EXPECT_MATCH));
    if (! $answer) { break; }

    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, 'ZZZZ', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, 'aaaa', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, 'NNNN', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.[]', $op, 'NNNN', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, 'nnnn', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.[]', $op, 'nnnn', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, min ($jsonData['stringSets']['setLower']), array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, max ($jsonData['stringSets']['setLower']), array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.[]', $op, min ($jsonData['stringSets']['setLower']), array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.[]', $op, max ($jsonData['stringSets']['setLower']), array(), STDOUT_EXPECT_MATCH));
    if (! $answer) { break; }

    foreach ($stringSetNames as $stringSetName) {
        foreach (array ('-i', '--ignorecase') as $ignoreCaseFlag) {
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, 'ZZZZ', array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, 'aaaa', array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, 'NNNN', array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, 'NNNN', array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, 'nnnn', array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, 'nnnn', array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, min ($jsonData['stringSets'][$stringSetName]), array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, max ($jsonData['stringSets'][$stringSetName]), array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, min ($jsonData['stringSets'][$stringSetName]), array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, max ($jsonData['stringSets'][$stringSetName]), array(), STDOUT_EXPECT_MATCH));
            if (! $answer) { break; }
        }
        if (! $answer) { break; }
    }
    if (! $answer) { break; }

}

##################################################################
msg_log ("  +---+ Testing greater-than relationship...");

foreach (array ('>', '-gt') as $op) {

    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, 'ZZZZ', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, 'aaaa', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, 'NNNN', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.[]', $op, 'NNNN', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, 'nnnn', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.[]', $op, 'nnnn', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, min ($jsonData['stringSets']['setUpper']), array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, max ($jsonData['stringSets']['setUpper']), array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.[]', $op, min ($jsonData['stringSets']['setUpper']), array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.[]', $op, max ($jsonData['stringSets']['setUpper']), array(), STDOUT_EXPECT_NOMATCH));
    if (! $answer) { break; }

    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, 'ZZZZ', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, 'aaaa', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, 'NNNN', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.[]', $op, 'NNNN', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, 'nnnn', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.[]', $op, 'nnnn', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, min ($jsonData['stringSets']['setLower']), array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, max ($jsonData['stringSets']['setLower']), array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.[]', $op, min ($jsonData['stringSets']['setLower']), array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.[]', $op, max ($jsonData['stringSets']['setLower']), array(), STDOUT_EXPECT_NOMATCH));
    if (! $answer) { break; }

    foreach ($stringSetNames as $stringSetName) {
        foreach (array ('-i', '--ignorecase') as $ignoreCaseFlag) {
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, 'ZZZZ', array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, 'aaaa', array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, 'NNNN', array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, 'NNNN', array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, 'nnnn', array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, 'nnnn', array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, min ($jsonData['stringSets'][$stringSetName]), array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, max ($jsonData['stringSets'][$stringSetName]), array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, min ($jsonData['stringSets'][$stringSetName]), array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, max ($jsonData['stringSets'][$stringSetName]), array(), STDOUT_EXPECT_NOMATCH));
            if (! $answer) { break; }
        }
        if (! $answer) { break; }
    }
    if (! $answer) { break; }

}

##################################################################
msg_log ("  +---+ Testing less-than-or-equals relationship...");

foreach (array ('<=', '-le') as $op) {

    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, 'ZZZZ', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, 'aaaa', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, 'NNNN', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.[]', $op, 'NNNN', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, 'nnnn', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.[]', $op, 'nnnn', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, min ($jsonData['stringSets']['setUpper']), array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, max ($jsonData['stringSets']['setUpper']), array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.[]', $op, min ($jsonData['stringSets']['setUpper']), array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.[]', $op, max ($jsonData['stringSets']['setUpper']), array(), STDOUT_EXPECT_MATCH));
    if (! $answer) { break; }

    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, 'ZZZZ', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, 'aaaa', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, 'NNNN', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.[]', $op, 'NNNN', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, 'nnnn', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.[]', $op, 'nnnn', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, min ($jsonData['stringSets']['setLower']), array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, max ($jsonData['stringSets']['setLower']), array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.[]', $op, min ($jsonData['stringSets']['setLower']), array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.[]', $op, max ($jsonData['stringSets']['setLower']), array(), STDOUT_EXPECT_MATCH));
    if (! $answer) { break; }

    foreach ($stringSetNames as $stringSetName) {
        foreach (array ('-i', '--ignorecase') as $ignoreCaseFlag) {
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, 'ZZZZ', array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, 'aaaa', array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, 'NNNN', array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, 'NNNN', array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, 'nnnn', array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, 'nnnn', array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, min ($jsonData['stringSets'][$stringSetName]), array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, max ($jsonData['stringSets'][$stringSetName]), array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, min ($jsonData['stringSets'][$stringSetName]), array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, max ($jsonData['stringSets'][$stringSetName]), array(), STDOUT_EXPECT_MATCH));
            if (! $answer) { break; }
        }
        if (! $answer) { break; }
    }
    if (! $answer) { break; }

}

##################################################################
msg_log ("  +---+ Testing greater-than-or-equals relationship...");

foreach (array ('>=', '-ge') as $op) {

    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, 'ZZZZ', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, 'aaaa', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, 'NNNN', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.[]', $op, 'NNNN', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, 'nnnn', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.[]', $op, 'nnnn', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, min ($jsonData['stringSets']['setUpper']), array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.<>', $op, max ($jsonData['stringSets']['setUpper']), array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.[]', $op, min ($jsonData['stringSets']['setUpper']), array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setUpper.[]', $op, max ($jsonData['stringSets']['setUpper']), array(), STDOUT_EXPECT_MATCH));
    if (! $answer) { break; }

    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, 'ZZZZ', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, 'aaaa', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, 'NNNN', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.[]', $op, 'NNNN', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, 'nnnn', array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.[]', $op, 'nnnn', array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, min ($jsonData['stringSets']['setLower']), array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.<>', $op, max ($jsonData['stringSets']['setLower']), array(), STDOUT_EXPECT_NOMATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.[]', $op, min ($jsonData['stringSets']['setLower']), array(), STDOUT_EXPECT_MATCH));
    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.setLower.[]', $op, max ($jsonData['stringSets']['setLower']), array(), STDOUT_EXPECT_MATCH));
    if (! $answer) { break; }

    foreach ($stringSetNames as $stringSetName) {
        foreach (array ('-i', '--ignorecase') as $ignoreCaseFlag) {
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, 'ZZZZ', array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, 'aaaa', array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, 'NNNN', array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, 'NNNN', array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, 'nnnn', array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, 'nnnn', array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, min ($jsonData['stringSets'][$stringSetName]), array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.<>', $ignoreCaseFlag . ' ' . $op, max ($jsonData['stringSets'][$stringSetName]), array(), STDOUT_EXPECT_NOMATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, min ($jsonData['stringSets'][$stringSetName]), array(), STDOUT_EXPECT_MATCH));
            $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, max ($jsonData['stringSets'][$stringSetName]), array(), STDOUT_EXPECT_MATCH));
            if (! $answer) { break; }
        }
        if (! $answer) { break; }
    }
    if (! $answer) { break; }

}

##################################################################
msg_log ("  +---+ Testing string comparison...");

$ops = array (
    ''           => 'regex',
    '-f'         => 'fixed',
    '--fixed'    => 'fixed',
    '-w'         => 'wildcard',
    '--wildcard' => 'wildcard'
);

$expectedResults = array (
    array (
        /* 'row-me-oh' => 'row-me-o' */
        array (
            '/^(.*).$/',
            '${1}'
        ), array (
            'regex'    => STDOUT_EXPECT_MATCH,
            'fixed'    => STDOUT_EXPECT_NOMATCH,
            'wildcard' => STDOUT_EXPECT_NOMATCH
        )
    ), array (
        /* 'row-me-oh' => 'row-*e-oh' */
        array (
            '/^(....).(.*)$/',
            '${1}*${2}'
        ), array (
            'regex'    => STDOUT_EXPECT_NOMATCH,
            'fixed'    => STDOUT_EXPECT_NOMATCH,
            'wildcard' => STDOUT_EXPECT_MATCH
        )
    ), array (
        /* 'row-me-oh' => 'ro*' */
        array (
            '/^(..).*$/',
            '${1}*'
        ), array (
            'regex'    => STDOUT_EXPECT_MATCH,
            'fixed'    => STDOUT_EXPECT_NOMATCH,
            'wildcard' => STDOUT_EXPECT_MATCH
        )
    ), array (
        /* 'row-me-oh' => 'row-?e-oh' */
        array (
            '/^(....).(.*)$/',
            '${1}?${2}'
        ), array (
            'regex'    => STDOUT_EXPECT_NOMATCH,
            'fixed'    => STDOUT_EXPECT_NOMATCH,
            'wildcard' => STDOUT_EXPECT_MATCH
        )
    ), array (
        /* 'row-me-oh' => 'ro?' */
        array (
            '/^(..).*$/',
            '${1}?'
        ), array (
            'regex'    => STDOUT_EXPECT_MATCH,
            'fixed'    => STDOUT_EXPECT_NOMATCH,
            'wildcard' => STDOUT_EXPECT_NOMATCH
        )
    ), array (
        /* 'row-me-oh' => '^\\s*row-me-oh\\s*$' */
        array (
            '/^.*$/',
            '^\\s*${0}\\s*\\$'
        ), array (
            'regex'    => STDOUT_EXPECT_MATCH,
            'fixed'    => STDOUT_EXPECT_NOMATCH,
            'wildcard' => STDOUT_EXPECT_NOMATCH
        )
    ), array (
        /* 'row-me-oh' => 'row-me-oh' */
        array (
            '/^.*$/',
            '${0}'
        ), array (
            'regex'    => STDOUT_EXPECT_MATCH,
            'fixed'    => STDOUT_EXPECT_MATCH,
            'wildcard' => STDOUT_EXPECT_MATCH
        )
    ), array (
        /* 'row-me-oh' => 'row-me-ohh' */
        array (
            '/^.*(.)$/',
            '${0}${1}'
        ), array (
            'regex'    => STDOUT_EXPECT_NOMATCH,
            'fixed'    => STDOUT_EXPECT_NOMATCH,
            'wildcard' => STDOUT_EXPECT_NOMATCH
        )
    )
);

foreach ($ops as $op => $opType) {
    foreach ($stringSetNames as $stringSetName) {
        foreach ($expectedResults as $expectedResult) {
            foreach ($jsonData['stringSets'][$stringSetName] as $testWord) {
                $testWordModified = preg_replace ($expectedResult[0][0], $expectedResult[0][1], $testWord);
                $testWordModifiedAndInvertedCase = call_user_func ($invertCallbacks[$stringSetName], $testWordModified);
                $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $op, $testWordModified, array(), $expectedResult[1][$opType]));
                $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $op, $testWordModifiedAndInvertedCase, array(), STDOUT_EXPECT_NOMATCH));
                foreach (array ('-i', '--ignorecase') as $ignoreCaseFlag) {
                    $answer = ($answer && matchingTest (randomChannel (), '/', $jsonData, 'stringSets.' . $stringSetName . '.[]', $ignoreCaseFlag . ' ' . $op, $testWordModifiedAndInvertedCase, array(), $expectedResult[1][$opType]));
                }
                if (! $answer) { break; }
            }
            if (! $answer) { break; }
        }
        if (! $answer) { break; }
    }
    if (! $answer) { break; }
}

##################################################################

return ($answer);
