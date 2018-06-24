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
$answer = ($answer && stdinSend (randomChannel (), '/', array("foo" => "bar"), 'property', '-#-', 'criteria'));
$answer = ($answer && stdoutExpectError ());
$answer = ($answer && stdinSend (randomChannel (), '/', array("foo" => "bar"), 'property', '-', 'criteria'));
$answer = ($answer && stdoutExpectError ());

##################################################################

$numericFlags = array ("<", "-lt", "<=", "-le", "=", "==", "-eq", "<>", "!=", "-ne", ">", "-gt", ">=", "-ge");
$stringFlags = array ("-f", "--fixed", "-w", "--wildcard");

msg_log ("  +---+ Lines that mix numeric and string comparison flags");
foreach ($numericFlags as $numericFlag) {
    foreach ($stringFlags as $stringFlag) {
        $answer = ($answer && stdinSend (randomChannel (), '/', array("foo" => "bar"), 'property', $numericFlag . " " . $stringFlag, 'criteria'));
        $answer = ($answer && stdoutExpectError ());

        $answer = ($answer && stdinSend (randomChannel (), '/', array("foo" => "bar"), 'property', $numericFlag . " " . $stringFlag, 'criteria'));
        $answer = ($answer && stdoutExpectError ());

        $answer = ($answer && stdinSend (randomChannel (), '/', array("foo" => "bar"), 'property', "! " . $stringFlag . " " . $numericFlag, 'criteria'));
        $answer = ($answer && stdoutExpectError ());

        $answer = ($answer && stdinSend (randomChannel (), '/', array("foo" => "bar"), 'property', "! " . $stringFlag . " " . $numericFlag, 'criteria'));
        $answer = ($answer && stdoutExpectError ());
    }
}

msg_log ("  +---+ Lines with an invalid string comparison flags combination");
foreach (array ('', '! ') as $negateFlag) {
    foreach (array ('', '-i ', '--ignorecase ') as $caseFlag) {
        foreach (array ('-f ', '--fixed ') as $fixedFlag) {
            foreach (array ('-w ', '--wildcard ') as $wildcardFlag) {
                $answer = ($answer && stdinSend (randomChannel (), '/', array("foo" => "bar"), 'property', $negateFlag . $caseFlag . $fixedFlag . $wildcardFlag, 'criteria'));
                $answer = ($answer && stdoutExpectError ());
                $answer = ($answer && stdinSend (randomChannel (), '/', array("foo" => "bar"), 'property', $negateFlag . $caseFlag . $wildcardFlag . $fixedFlag, 'criteria'));
                $answer = ($answer && stdoutExpectError ());
            }
        }
    }
}

msg_log ("  +---+ Line with more than one numeric operator");
foreach (array ('', '! ') as $negateFlag) {
    foreach ($numericFlags as $numericFlagA) {
        foreach ($numericFlags as $numericFlagB) {
            $answer = ($answer && stdinSend (randomChannel (), '/', array("foo" => "bar"), 'property', $negateFlag . $numericFlagA . " " . $numericFlagB, 'criteria'));
            $answer = ($answer && stdoutExpectError ());
        }
    }
}
return ($answer);
