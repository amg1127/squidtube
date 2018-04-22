<?php

$channel = 0;
$answer = true;

msg_log ("  +---+ An empty line");
$answer = ($answer && stdinSendRaw (""));
$answer = ($answer && stdoutExpectError ());

msg_log ("  +---+ A line with only a number");
$answer = ($answer && stdinSendRaw ("1"));
$answer = ($answer && stdoutExpectError ());

msg_log ("  +---+ A long line with numbers only");
$answer = ($answer && stdinSendRaw (str_repeat("01234567", 128)));
$answer = ($answer && stdoutExpectError ());

msg_log ("  +---+ A long line with numbers and letters");
$answer = ($answer && stdinSendRaw (str_repeat("0123456789abcdef", 64)));
$answer = ($answer && stdoutExpectError ());

msg_log ("  +---+ A line with an invalid channel number");
$answer = ($answer && stdinSend (-1, '/', array(), 'property', '', 'criteria'));
$answer = ($answer && stdoutExpectError ());

msg_log ("  +---+ A line with an invalid URL");
$answer = ($answer && stdinSendRaw ("%666% The number of the beast"));
$answer = ($answer && stdoutExpectError ());

msg_log ("  +---+ Lines without a property for comparison");
$answer = ($answer && stdinSendRaw ($GLOBALS['serverAddress'] . "/?mirror=" . rawurlencode ("{ \"foo\": \"bar\" }")));
$answer = ($answer && stdoutExpectError ());
$answer = ($answer && stdinSendRaw ($GLOBALS['serverAddress'] . "/?mirror=" . rawurlencode ("{ \"foo\": \"bar\" }") . " " . rawurlencode("!")));
$answer = ($answer && stdoutExpectError ());

msg_log ("  +---+ Lines with an invalid comparison flag");
$answer = ($answer && stdinSend ($channel++, '/', array("foo" => "bar"), 'property', '-#-', 'criteria'));
$answer = ($answer && stdoutExpectError ());
$answer = ($answer && stdinSend ($channel++, '/', array("foo" => "bar"), 'property', '-', 'criteria'));
$answer = ($answer && stdoutExpectError ());

##################################################################

$numericFlags = array ("<", "-lt", "<=", "-le", "=", "==", "-eq", "<>", "!=", "-ne", ">", "-gt", ">=", "-ge");
$stringFlags = array ("-f", "--fixed", "-w", "--wildcard", "-i", "--ignorecase");

msg_log ("  +---+ Lines that mix numeric and string comparision flags");
foreach ($numericFlags as $numericFlag) {
    foreach ($stringFlags as $stringFlag) {
        $answer = ($answer && stdinSend ($channel++, '/', array("foo" => "bar"), 'property', $numericFlag . " " . $stringFlag, 'criteria'));
        $answer = ($answer && stdoutExpectError ());

        $answer = ($answer && stdinSend ($channel++, '/', array("foo" => "bar"), 'property', $numericFlag . " " . $stringFlag, 'criteria'));
        $answer = ($answer && stdoutExpectError ());

        $answer = ($answer && stdinSend ($channel++, '/', array("foo" => "bar"), 'property', "! " . $stringFlag . " " . $numericFlag, 'criteria'));
        $answer = ($answer && stdoutExpectError ());

        $answer = ($answer && stdinSend ($channel++, '/', array("foo" => "bar"), 'property', "! " . $stringFlag . " " . $numericFlag, 'criteria'));
        $answer = ($answer && stdoutExpectError ());
    
        if ($channel > 50) {
            $channel = 0;
        }
    }
}

msg_log ("  +---+ Lines with an invalid string comparision flags combination");
foreach (array ('', '! ') as $negateFlag) {
    foreach (array ('', '-i ', '--ignorecase ') as $caseFlag) {
        foreach (array ('-f ', '--fixed ') as $fixedFlag) {
            foreach (array ('-w ', '--wildcard ') as $wildcardFlag) {
                $answer = ($answer && stdinSend ($channel++, '/', array("foo" => "bar"), 'property', $negateFlag . $caseFlag . $fixedFlag . $wildcardFlag, 'criteria'));
                $answer = ($answer && stdoutExpectError ());
                $answer = ($answer && stdinSend ($channel++, '/', array("foo" => "bar"), 'property', $negateFlag . $caseFlag . $wildcardFlag . $fixedFlag, 'criteria'));
                $answer = ($answer && stdoutExpectError ());
    
                if ($channel > 50) {
                    $channel = 0;
                }
            }
        }
    }
}

msg_log ("  +---+ Line with more than one numeric operator");
foreach (array ('', '! ') as $negateFlag) {
    foreach ($numericFlags as $numericFlagA) {
        foreach ($numericFlags as $numericFlagB) {
            $answer = ($answer && stdinSend ($channel++, '/', array("foo" => "bar"), 'property', $negateFlag . $numericFlagA . " " . $numericFlagB, 'criteria'));
            $answer = ($answer && stdoutExpectError ());
    
            if ($channel > 50) {
                $channel = 0;
            }
        }
    }
}
return ($answer);
