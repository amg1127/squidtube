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
$jsonData = array (
    "numbers" => array (0, 1, 2, 3, 4, 5, 6, 7)
);

##################################################################

msg_log ("  +---+ Tests with property names that contain dots...");
$answer = ($answer && stdinSend (randomChannel (), '/', array("foo" => array ("john.doe" => "bar")), 'foo.john.doe', '', '.', array(), STDOUT_EXPECT_NOMATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', array("foo" => array ("john.doe" => "bar")), 'foo.["john.doe"]', '-f', 'bar', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', array("foo.bar" => array ("one" => "two")), '["foo.bar"].["one"]', '-f', 'two', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', array("foo" => array ("bar" => array ("one" => "two"))), '["foo.bar"].["one"]', '-f', 'two', array(), STDOUT_EXPECT_NOMATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', array("foo" => array ("bar" => array ("one" => "two"))), 'foo.bar.["one"]', '-f', 'two', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', array("foo" => array ("bar" => array ("one" => "two"))), 'foo.bar.one', '-f', 'two', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', array("foo%25bar" => array ("one" => "two")), '["foo%25bar"].["one"]', '-f', 'two', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', array("foo.bar" => array ("one" => "two")), 'foo.bar.one', '', '.', array(), STDOUT_EXPECT_NOMATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', array("foo.bar" => array ("one" => "two")), '["foo.bar.one"]', '', '.', array(), STDOUT_EXPECT_NOMATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', array("foo.bar" => array ("one" => "two")), '["foo.bar"].one', '-f', 'two', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', array("foo.bar.baz" => array ("one" => "two")), '["foo.bar.baz"].one', '-f', 'two', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', array("foo.bar" => array ("one" => "two")), 'foo.bar.one', '', '.', array(), STDOUT_EXPECT_NOMATCH));

##################################################################

msg_log ("  +---+ Tests with array ranges...");
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<0:3>', '<', '4', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<0:4>', '<', '4', array(), STDOUT_EXPECT_NOMATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<0:4>', '<=', '4', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<3:-5>', '<', '4', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<3:-4>', '<', '4', array(), STDOUT_EXPECT_NOMATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<3:-4>', '<=', '4', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<-4>', '==', '4', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.[-4]', '==', '4', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<-7>', '==', '1', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.[-7]', '==', '1', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.[1:3]', '==', '2', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.[-1:3]', '==', '2', array(), STDOUT_EXPECT_NOMATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.[-1:3]', '==', '7', array(), STDOUT_EXPECT_NOMATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.[-1:-3]', '==', '6', array(), STDOUT_EXPECT_NOMATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.[3:-1]', '==', '2', array(), STDOUT_EXPECT_NOMATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.[3:-1]', '==', '7', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.[-3:-1]', '==', '6', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.[-8:-6]', '==', '1', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<0:2,-3:-1>', '!=', '3', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<0:2,-3:-1>', '!=', '4', array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<0:3,-3:-1>', '!=', '3', array(), STDOUT_EXPECT_NOMATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<0:2,-4:-1>', '!=', '4', array(), STDOUT_EXPECT_NOMATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<0:1,2:3,4:5,6:7>', '==', $jsonData["numbers"], array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<-8:-7,-6:-5,-4:-3,-2:-1>', '==', $jsonData["numbers"], array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<0:1,2,3,4,5,6:7>', '==', $jsonData["numbers"], array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<-8:-7,-6,-5,-4,-3,-2:-1>', '==', $jsonData["numbers"], array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<0,1,2,3,4,5,6,7>', '==', $jsonData["numbers"], array(), STDOUT_EXPECT_MATCH));
$answer = ($answer && stdinSend (randomChannel (), '/', $jsonData, 'numbers.<-8,-7,-6,-5,-4,-3,-2,-1>', '==', $jsonData["numbers"], array(), STDOUT_EXPECT_MATCH));

##################################################################

return ($answer);
