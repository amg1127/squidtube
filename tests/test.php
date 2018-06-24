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

@$tz = date_default_timezone_get ();
if (empty ($tz)) {
    $tz = "UTC";
}
date_default_timezone_set ($tz);
error_reporting (-1);
$sapi = php_sapi_name ();

if ($sapi == "cli-server") {
    return (cli_server_sapi ());
} else if ($sapi == "cli") {
    return (cli_sapi ());
} else {
    msg_fatal ("Unsupported PHP execution environment: '" . $sapi . "'");
}

//////////////////////////////////////////////////////////////////

function cli_server_sapi () {
    /*
    * GET variables used for tests:
    *
    * 'response' => string, with format "<number> <string>". If present, it sets the HTTP response line
    *
    * 'pause' => pause the following number of seconds before answering the request
    *
    * 'redirects' => expects a number. If the number is above zero and 'response' begins with '3',
    *  it sends a redirect response to a constructed URL with 'redirects=' variable decremented by 1
    *
    * 'expect' => expects an array of "key=value" pairs. If present, the script tests
    *  if $_POST['key'] == 'value' and returns a HTTP 500 Server Error and an empty body if the test does not succeed
    *
    * 'mirror' => response body that is going to be sent
    *
    * 'bigdata' => if present, sends 256MB of spaces before the actual data
    *
    */

    $queryString = "";

    $response = "";
    if (! empty ($_GET['response'])) {
        $response = $_GET['response'];
        $queryString .= "&response=" . rawurlencode ($response);
    }

    if (! empty ($_GET['pause'])) {
        sleep ((int) $_GET['pause']);
        $queryString .= "&pause=" . rawurlencode ($_GET['pause']);
    }

    $redirects = -1;
    if (! empty ($_GET['redirects'])) {
        $redirects = (int) $_GET['redirects'];
        if ((--$redirects) >= 0) {
            $queryString .= "&redirects=" . rawurlencode ('' . $redirects);
        }
    }

    $expectSucceeded = true;
    if (! empty ($_GET['expect'])) {
        if (! is_array ($_GET['expect'])) {
            $_GET['expect'] = array ($_GET['expect']);
        }
        $expectCount = count ($_GET['expect']);
        for ($pos = 0; $pos < $expectCount; $pos++) {
            $pair = $_GET['expect'][$pos];
            $queryString .= "&expect[]=" . rawurlencode ($pair);
            $index = strpos ($pair, '=');
            if ($index === false) {
                $index = strlen ($pair);
            }
            $key = urldecode (substr ($pair, 0, $index));
            $value = urldecode (substr ($pair, $index + 1));

            if (empty($_POST[$key]) || ($_POST[$key] != $value)) {
                $response = '500 Server Error';
                $expectSucceeded = false;
            }
        }
    }

    if (! empty ($_GET['mirror'])) {
        $queryString .= "&mirror=" . rawurlencode ($_GET['mirror']);
    }

    if ($response) {
        $ch = substr ($response, 0, 1);
        if ($ch != '3' || $redirects >= 0) {
            header ($_SERVER["SERVER_PROTOCOL"] . ' ' . $response);
        }
        if ($ch == '3' && $redirects >= 0) {
            header ("Location: " . $_SERVER["SCRIPT_NAME"] . "?" . substr ($queryString, 1), true);
        }
    }

    if (! empty ($_GET['bigdata'])) {
        # Under a 32-bit environment, sending 96 MB of data leads to
        # allocation of more than 4GB of memory, resulting in a 'std::bad_alloc' exception.
        $mb = ((PHP_INT_MAX > 1e12) ? 96 : 8);
        for ($i = 0; $i < $mb; $i++) {
            echo (str_repeat (str_repeat (' ', 1023) . "\n", 1024));
            usleep (10 * 1000000 / $mb);
        }
    }

    if ($expectSucceeded) {
        if (empty ($_GET['mirror'])) {
            echo ("{}\n");
        } else {
            echo ($_GET['mirror'] . "\n");
        }
    }

    return (true);
}

function cli_sapi () {
    // Default exit code
    $GLOBALS['exitCode'] = 1;
    // An invalid address for simulation of network problems
    $GLOBALS['v4Socket'] = null;
    $GLOBALS['v6Socket'] = null;
    $GLOBALS['invalidAddress'] = null;
    // Server address
    $GLOBALS['serverAddress'] = null;
    // Server process descriptor
    $GLOBALS['serverProcess'] = null;
    // Process and file descriptors for communication with the program
    $GLOBALS['projectProcess'] = null;
    $GLOBALS['projectSTDIN'] = null;
    $GLOBALS['projectSTDOUT'] = null;
    $GLOBALS['projectSTDERR'] = null;
    // A file descriptor for a file that will store the data that will be sent through STDIN
    $GLOBALS['STDINclone'] = null;
    // A file descriptor for a file that will store the data that will be received through STDOUT
    $GLOBALS['STDOUTclone'] = null;
    // It must be zero at the end of each test
    $GLOBALS['diffInputOutput'] = 0;

    // This is used by stdoutExpect() function
    define ('STDOUT_EXPECT_MATCH', 'MATCH');
    define ('STDOUT_EXPECT_NOMATCH', 'NOMATCH');
    define ('STDOUT_EXPECT_NOHELPER', 'NOHELPER');
    define ('STDOUT_EXPECT_NOHELPER_FALSE', 'NOHELPER0');
    define ('STDOUT_EXPECT_NOHELPER_TRUE', 'NOHELPER1');
    define ('STDOUT_EXPECT_ERROR', 'BH');
    define ('EXPECT_DEFAULT_TIMEOUT', 30);

    // Some frequently-expected regular expressions...
    define ('STDERR_EXPECT_INVALID_COMPARISON_OPERATOR', '(INFO|DEBUG):\\s*\\[\\w+#\\d+\\]\\s*Unable\\s+to\\s+apply\\s+selected\\s+comparison\\s+operator\\s+');

    // The project name is expected to be supplied by "qmake"...
    if (empty ($GLOBALS['argv'][1])) {
        msg_fatal ("Unable to retrieve the project name!");
    } else {
        $projectName = $GLOBALS['argv'][1];
    }

    // I need two sockets bound to a fixed port in order to simulate network errors
    for ($socketTry = 1; (! $GLOBALS['invalidAddress']) && $socketTry <= 5; $socketTry++) {
        $serverPort = rand (1024, 65535);
        foreach (array ('v4Socket' => AF_INET, 'v6Socket' => AF_INET6) as $key => $family) {
            msg_log ("[" . $socketTry . "] Trying to bind " . $key . "/TCP port " . $serverPort . "...");
            $GLOBALS[$key] = socket_create ($family, SOCK_STREAM, SOL_TCP);
            if ($GLOBALS[$key]) {
                if (! socket_bind ($GLOBALS[$key], "localhost", $serverPort)) {
                    $errNo = socket_last_error ($GLOBALS[$key]);
                    msg_warning ("'socket_bind (" . $key . ", " . $serverPort . ")' failed: " . $errNo . ": " . socket_strerror ($errNo));
                    $GLOBALS[$key] = null;
                }
            } else {
                $errNo = socket_last_error ();
                msg_warning ("'socket_create (" . $key . ")' failed: " . $errNo . ": " . socket_strerror ($errNo));
            }
            if (! $GLOBALS[$key]) {
                break;
            }
        }
        if ($GLOBALS['v4Socket'] && $GLOBALS['v6Socket']) {
            $GLOBALS['invalidAddress'] = "http://localhost:" . $serverPort;
            break;
        }
    }
    if (! $GLOBALS['invalidAddress']) {
        msg_fatal ("Unable to create local sockets for network error simulation!");
    }

    // Try to launch a local web server using PHP's builtin one
    if ((int) ini_get ('allow_url_fopen')) {
        for ($launchTry = 1; (! $GLOBALS['serverProcess']) && $launchTry <= 5; $launchTry++) {
            $serverPort = rand (1024, 65535);
            msg_log ("[" . $launchTry . "] Trying to launch a local web server at TCP port " . $serverPort . "...");
            $GLOBALS['serverAddress'] = "localhost:" . $serverPort;
            $serverProcessTentative = proc_open ("php -S " . escapeshellarg ($GLOBALS['serverAddress']) . " " . escapeshellarg (__FILE__), array (
                0 => array ("pipe", "r"),
                1 => STDOUT,
                2 => STDERR
            ), $pipes, __DIR__, null, array ("bypass_shell" => true));
            if ($serverProcessTentative) {
                fclose ($pipes[0]);
                msg_log ("Server seems to be running now.");
                $GLOBALS['serverAddress'] = "http://" . $GLOBALS['serverAddress'];
                for ($connectTry = 1; (! $GLOBALS['serverProcess']) && $connectTry <= 10; $connectTry++) {
                    msg_log ("[" . $connectTry . "] Trying to establish a HTTP connection...");
                    usleep (500000);
                    $random_answer = __FILE__ . "_" . rand ();
                    $data = file_get_contents ($GLOBALS['serverAddress'] . "/ping_test/?mirror=" . rawurlencode ($random_answer));
                    if (trim ($data) == $random_answer) {
                        $GLOBALS['serverProcess'] = $serverProcessTentative;
                    }
                }
                if (! $GLOBALS['serverProcess']) {
                    shutdownProcess ($serverProcessTentative, "Server did not send the expected answer.");
                }
            }
        }
    } else {
        msg_warning ("'allow_url_fopen' is disabled!");
    }

    if (! $GLOBALS['serverProcess']) {
        socket_close ($GLOBALS['v4Socket']);
        socket_close ($GLOBALS['v6Socket']);
        msg_fatal ("Unable to launch a local web server!");
    }

    $projectLogFile = __DIR__ . "/" . basename (__FILE__, '.php') . ".log";
    removeFile ($projectLogFile);
    if (file_put_contents ($projectLogFile, '') !== 0) {
        shutdownProcess ($GLOBALS['serverProcess']);
        socket_close ($GLOBALS['v4Socket']);
        socket_close ($GLOBALS['v6Socket']);
        msg_fatal ("Unable to create log file '" . $projectLogFile . "'!");
    }

    $databaseFile = __DIR__ . "/" . basename (__FILE__, '.php') . ".sqlite";
    foreach (array ('', '-wal', '-shm') as $suffix) {
        removeFile ($databaseFile . $suffix);
    }
    $helperName = 'testhelper';
    $GLOBALS['projectProcess'] = proc_open (
        escapeshellarg (__DIR__ . "/../" . $projectName) . " " .
        "--main.loglevel=DEBUG " .
        "--config " . escapeshellarg (__DIR__ . "/test_config.conf") . " " .
        "--db.name " . escapeshellarg ($databaseFile) . " " .
        "--" . $helperName . ".notsosimpleglobalvariable " . escapeshellarg ("simple value actually") . " " .
        "--" . $helperName . ".commandlineglobalvariable " . escapeshellarg ("command-line value")
    , array (
        0 => array ("pipe", "rb"),
        1 => array ("pipe", "wt"),
        2 => array ("file", $projectLogFile, "wt")
    ), $pipes, __DIR__, null, array ("bypass_shell" => true));
    if (! $GLOBALS['projectProcess']) {
        shutdownProcess ($GLOBALS['serverProcess']);
        unlink ($projectLogFile);
        socket_close ($GLOBALS['v4Socket']);
        socket_close ($GLOBALS['v6Socket']);
        msg_fatal ("Unable to launch the project executable!");
    }
    $GLOBALS['projectSTDIN'] = $pipes[0];
    stream_set_write_buffer ($GLOBALS['projectSTDIN'], 0);
    $GLOBALS['projectSTDOUT'] = $pipes[1];
    $GLOBALS['projectSTDERR'] = fopen ($projectLogFile, "rt");
    if (! $GLOBALS['projectSTDERR']) {
        fclose ($GLOBALS['projectSTDIN']);
        if (waitGracefulFinish () === false) {
            shutdownProcess ($GLOBALS['projectProcess']);
        }
        shutdownProcess ($GLOBALS['serverProcess']);
        unlink ($projectLogFile);
        socket_close ($GLOBALS['v4Socket']);
        socket_close ($GLOBALS['v6Socket']);
        msg_fatal ("Unable to open log file '" . $projectLogFile . "' for reading!");
    }
    foreach (array ($GLOBALS['projectSTDOUT'], $GLOBALS['projectSTDERR']) as $fd) {
        stream_set_blocking ($fd, false);
        stream_set_read_buffer ($fd, 0);
    }

    $projectInFile = __DIR__ . "/" . basename (__FILE__, '.php') . ".stdin";
    removeFile ($projectInFile);
    $GLOBALS['STDINclone'] = fopen ($projectInFile, "wt");
    if (! $GLOBALS['STDINclone']) {
        msg_warning ("Unable to open file '" . basename ($projectInFile) . "' for writing!");
    }
    $projectOutFile = __DIR__ . "/" . basename (__FILE__, '.php') . ".stdout";
    removeFile ($projectOutFile);
    $GLOBALS['STDOUTclone'] = fopen ($projectOutFile, "wt");
    if (! $GLOBALS['STDOUTclone']) {
        msg_warning ("Unable to open file '" . basename ($projectOutFile) . "' for writing!");
    }

    register_shutdown_function (function () {
        closeFileDescriptor ($GLOBALS['projectSTDIN']);
        closeFileDescriptor ($GLOBALS['STDINclone']);
        closeFileDescriptor ($GLOBALS['STDOUTclone']);
        if (stderrExpect ('INFO:\\s*StdinReader\\s+thread\\s+finished\\.')) {
            stderrExpect ('DEBUG:\\s*Performing\\s+final\\s+cleaning\\s+and\\s+finishing\\s+program\\.\\.\\.');
        }
        $finishStatus = waitGracefulFinish ();
        if ($finishStatus !== 0) {
            $GLOBALS['exitCode'] = 1;
            if ($finishStatus === false) {
                msg_warning ("Project executable is still running!");
                shutdownProcess ($GLOBALS['projectProcess']);
            } else if ($finishStatus !== 0) {
                msg_warning ("Project executable ended with return code #" . $finishStatus . ".");
            }
        }
        shutdownProcess ($GLOBALS['serverProcess'], "Tests finished.");
        socket_close ($GLOBALS['v4Socket']);
        socket_close ($GLOBALS['v6Socket']);
        msg_log ("Test program exited with status code #" . $GLOBALS['exitCode'] . ".");
        exit ($GLOBALS['exitCode']);
    });

    // Wait program startup
    if (stderrExpect ('INFO:\\s*StdinReader\\s+thread\\s+started\\.')) {
        msg_log ("We are all set for the tests. Let's go!");

        $testFiles = array ();
        $testDirPath = __DIR__ . "/suite";
        $testDir = opendir ($testDirPath);
        if ($testDir) {
            while (($testFile = readdir ($testDir)) !== false) {
                if (preg_match ('/^[a-zA-Z0-9_-]+\\.php$/i', $testFile)) {
                    array_push ($testFiles, $testDirPath . "/" . $testFile);
                }
            }
            closedir ($testDir);
        }
        shuffle ($testFiles);
        $pendingTests = count ($testFiles);
        foreach ($testFiles as $testFile) {
            $procStatus = getProcessExitCode ();
            if ($procStatus !== false) {
                msg_warning ("Project executable finished unexpectedly with code #" . $procStatus . "! Did it crash?");
                break;
            }
            $testName = basename ($testFile, '.php');
            msg_log ("  + Running test '" . $testName . "'...");
            $includeAnswer = includeWithScopeProtection ($testFile);
            if ($GLOBALS['diffInputOutput']) {
                msg_warning ("Test '" . $testName . "' imbalanced input/output relationship: #" . $GLOBALS['diffInputOutput']);
            }
            if (! $includeAnswer) {
                msg_warning ("Test '" . $testName . "' did not run successfully!");
                break;
            }
            $pendingTests--;
        }
        if ((! $pendingTests) && count ($testFiles)) {
            $GLOBALS['exitCode'] = 0;
        }
    }
}

function msg_log ($msg) {
    echo ("[" . date('c') . "] " . $msg . "\n");
}

function msg_fatal ($msg) {
    msg_log (" **** " . $msg . " ****");
    exit (1);
}

function msg_warning ($msg) {
    msg_log (" ---- " . $msg . " ----");
}

function removeFile ($path) {
    clearstatcache ();
    if (file_exists ($path) || is_link ($path)) {
        unlink ($path);
    }
}

function shutdownProcess ($proc, $msg = '') {
    if (! empty ($msg)) {
        msg_log ($msg);
    }
    while (1) {
        $procStatus = proc_get_status ($proc);
        if ($procStatus) {
            if (! $procStatus['running']) {
                break;
            }
        }
        msg_log ("Terminating the child process...");
        proc_terminate ($proc);
        sleep (1);
    }
}

function getProcessExitCode () {
    if (! empty ($GLOBALS['projectProcess'])) {
        $procStatus = proc_get_status ($GLOBALS['projectProcess']);
        if ($procStatus) {
            if (! $procStatus['running']) {
                return ($procStatus['exitcode']);
            }
        }
    }
    return (false);
}

function waitGracefulFinish () {
    for ($wait = 0; $wait < 10; $wait++) {
        $procStatus = getProcessExitCode ();
        if ($procStatus !== false) {
            return ($procStatus);
        }
        usleep (500000);
    }
    return (false);
}

function swapValues (&$a, &$b) {
    $temp = $a;
    $a = $b;
    $b = $temp;
}

function matchingTest ($channel, $urlPath, $jsonData, $testProperty, $testFlags, $testCriteria, $options = array (), $expectedResult = null, $expectedMessages = null, $timeout = null) {
    // https://en.wikipedia.org/wiki/Heap%27s_algorithm
    $heap_A = preg_split ('/\\s/', $testFlags, -1, PREG_SPLIT_NO_EMPTY);
    $heap_n = count ($heap_A);
    $heap_c = array_fill (0, $heap_n, 0);
    $invertedResult = $expectedResult;
    if ($expectedResult === STDOUT_EXPECT_MATCH) {
        $invertedResult = STDOUT_EXPECT_NOMATCH;
    } else if ($expectedResult === STDOUT_EXPECT_NOMATCH) {
        $invertedResult = STDOUT_EXPECT_MATCH;
    } else if ($expectedResult === STDOUT_EXPECT_NOHELPER) {
        $expectedResult = STDOUT_EXPECT_NOHELPER_FALSE;
        $invertedResult = STDOUT_EXPECT_NOHELPER_TRUE;
    }
    if (stdinSend ($channel, $urlPath, $jsonData, $testProperty, $testFlags, $testCriteria, $options, $expectedResult, $expectedMessages, $timeout) === false) {
        return (false);
    }
    if (stdinSend ($channel, $urlPath, $jsonData, $testProperty, '! ' . $testFlags, $testCriteria, $options, $invertedResult, $expectedMessages, $timeout) === false) {
        return (false);
    }
    $heap_i = 0;
    while ($heap_i < $heap_n) {
        if ($heap_c[$heap_i] < $heap_i) {
            if ($heap_i & 1) {
                swapValues ($heap_A[$heap_c[$heap_i]], $heap_A[$heap_i]);
            } else {
                swapValues ($heap_A[0], $heap_A[$heap_i]);
            }
            $testFlags = implode (' ', $heap_A);
            if (stdinSend ($channel, $urlPath, $jsonData, $testProperty, $testFlags, $testCriteria, $options, $expectedResult, $expectedMessages, $timeout) === false) {
                return (false);
            }
            if (stdinSend ($channel, $urlPath, $jsonData, $testProperty, '! ' . $testFlags, $testCriteria, $options, $invertedResult, $expectedMessages, $timeout) === false) {
                return (false);
            }
            $heap_c[$heap_i] += 1;
            $heap_i = 0;
        } else {
            $heap_c[$heap_i] = 0;
            $heap_i += 1;
        }
    }
    return (true);
}

function stdinSend ($channel, $urlPath, $jsonData, $testProperty, $testFlags, $testCriteria, $options = array (), $expectedResult = null, $expectedMessages = null, $timeout = null) {
    $mirror = rawurlencode (json_encode ($jsonData));
    $expect = array ();
    if (! empty ($options['expect'])) {
        $expect = array_map (function ($k, $v) {
            return (rawurlencode($k) . "=" . rawurlencode ($v));
        }, array_keys ($options['expect']), $options['expect']);
    }
    if (isset ($options['expect'])) {
        unset ($options['expect']);
    }
    $optionsData = "";
    foreach ($options as $k => $v) {
        $optionsData .= rawurlencode ($k) . "=" . rawurlencode ($v) . "&";
    }
    $server = $GLOBALS['serverAddress'] . '/';
    foreach (array ('invalidAddress', 'serverAddress') as $variable) {
        if (! strncasecmp ($GLOBALS[$variable] . '/', $urlPath, strlen ($GLOBALS[$variable]) + 1)) {
            $server = '';
            break;
        }
    }
    $line = ((int) $channel) . " " . $server . $urlPath . "?" . implode ("", array_map (function ($item) {
        return (rawurlencode ("expect[]") . "=" . rawurlencode ($item) . "&");
    }, $expect)) . $optionsData . "mirror=" . $mirror . " " . rawurlencode ($testProperty) . " " .
    implode (" ", array_map ('rawurlencode', preg_split ('/\\s/', $testFlags, -1, PREG_SPLIT_NO_EMPTY))) . " -- ";
    if (is_array ($testCriteria)) {
        $line .= implode (" ", array_map ('rawurlencode', $testCriteria));
    } else {
        $line .= rawurlencode ($testCriteria);
    }
    if (! stdinSendRaw (trim ($line))) {
        return (false);
    }
    if ($expectedResult !== null) {
        $answer = stdoutExpect ($expectedResult, $timeout);
        if ($answer === false) {
            return (false);
        } else {
            if ($expectedMessages !== null) {
                if (! is_array ($expectedMessages)) {
                    $expectedMessages = array ($expectedMessages);
                }
                foreach ($expectedMessages as $expectedMessage) {
                    if (! stderrExpect ($expectedMessage, $timeout)) {
                        return (false);
                    }
                }
            }
            if (stderrExpectAnswer ($timeout)) {
                return ($answer);
            }
        }
        return (false);
    } else {
        return (true);
    }
}

function stdinSendRaw ($line) {
    if (writeDataToFileDescriptor ($line . "\n", $GLOBALS['projectSTDIN'])) {
        writeDataToFileDescriptor ($line . "\n", $GLOBALS['STDINclone']);
        fflush ($GLOBALS['projectSTDIN']);
        $GLOBALS['diffInputOutput']++;
        return (true);
    } else {
        msg_warning ("Unable to send a request line to the program! Aborting...");
        closeFileDescriptor ($GLOBALS['projectSTDIN']);
        writeDataToFileDescriptor ($line, $GLOBALS['STDINclone']);
        closeFileDescriptor ($GLOBALS['STDINclone']);
        $GLOBALS['exitCode'] = 1;
        return (false);
    }
}

function writeDataToFileDescriptor ($data, $outFd) {
    if ($outFd) {
        $dataLength = strlen ($data);
        $retries = 0;
        $maxtries = 100;
        for ($written = 0; $written < $dataLength; $written += $bytes) {
            $bytes = fwrite ($outFd, substr ($data, $written));
            if ($bytes === false) {
                return (false);
            } else if ($bytes === 0) {
                $retries++;
                if ($retries < $maxtries) {
                    usleep (100000);
                } else {
                    return (false);
                }
            } else {
                $retries = 0;
            }
        }
        return (true);
    } else {
        return (false);
    }
}

function readLineFromDescriptor ($fd, $timeout = null) {
    if (empty ($timeout)) {
        $timeout = EXPECT_DEFAULT_TIMEOUT;
    }
    set_time_limit ($timeout + 30);
    $buffer = "";
    $start = $prev = time ();
    while (1) {
        while (($character = stream_get_line ($fd, 1, "\n")) !== false) {
            if ($character === "") {
                return ($buffer);
            } else {
                $buffer .= $character;
            }
        }
        usleep (10000);
        $now = time();
        if (($prev + 4) < $now) {
            $procStatus = getProcessExitCode ();
            if ($procStatus !== false) {
                msg_warning ("Project executable ended unexpectedly with code #" . $procStatus . "! Did it crash?");
                break;
            }
            if (($now - $start) >= $timeout) {
                break;
            }
            msg_warning ('Input is not ready yet. Waiting...');
            $prev = $now;
        }
        $readFds = array ($fd);
        $writeFds = null;
        $exceptFds = null;
        $selectStatus = stream_select ($readFds, $writeFds, $exceptFds, $timeout);
        if ($selectStatus === 0) {
            continue;
        }
    }
    msg_warning ('Unable to read input!');
    closeFileDescriptor ($GLOBALS['projectSTDIN']);
    closeFileDescriptor ($GLOBALS['STDINclone']);
    $GLOBALS['exitCode'] = 1;
    return (false);
}

function closeFileDescriptor (&$fd) {
    if (! empty ($fd)) {
        fclose ($fd);
        $fd = null;
    }
}

function stderrExpect ($regexp, $timeout = null) {
    if (empty ($timeout)) {
        $timeout = EXPECT_DEFAULT_TIMEOUT;
    }
    while (($line = readLineFromDescriptor ($GLOBALS['projectSTDERR'], $timeout)) !== false) {
        $line = trim (preg_replace ('/^\\s*\\[\\d\\d\\d\\d-\\d\\d-\\d\\dT\\d\\d:\\d\\d:\\d\\d(|\\.\\d+)[^\\]]*\\]\\s+(|0[xX][a-fA-F0-9]+\\s+)(\\w+\\s*:\\s)/', '\\3', $line));
        if (preg_match ('/^' . $regexp . '/', $line)) {
            return (true);
        } else if (preg_match ('/^\\w+:\\s*(|A\\s+)Javascript\\s+exception\\s+/i', $line)) {
            msg_warning ($line);
            break;
        }
    }
    msg_warning ("Pattern was not found on messages coming from STDERR: '" . $regexp . "'!");
    $GLOBALS['exitCode'] = 1;
    return (false);
}

function stdoutExpect ($what, $timeout = null) {
    if (empty ($timeout)) {
        $timeout = EXPECT_DEFAULT_TIMEOUT;
    }
    $decodedAnswer = "";
    if (($line = readLineFromDescriptor ($GLOBALS['projectSTDOUT'], $timeout)) !== false) {
        $GLOBALS['diffInputOutput']--;
        $line = trim ($line);
        writeDataToFileDescriptor ($line . "\n", $GLOBALS['STDOUTclone']);
        if (preg_match ('/^\\s*(|(-?\\d+)\\s+)(OK|ERR|BH)\\s+message=(\\S+)\\s+log=\\S+\\s*$/', $line, $matches)) {
            $decodedAnswer = urldecode ($matches[4]);
            if ($what == STDOUT_EXPECT_MATCH && $matches[3] == "OK" && preg_match ('/^\\[[^\\]]+#\\d+\\]\\s+(Cached|Retrieved)\\s+data\\s+from\\s+.*\\)\\s+matches\\s+specified\\s+criteria\\.?$/', $decodedAnswer)) {
                return ($matches[1]);
            } else if ($what == STDOUT_EXPECT_NOMATCH && $matches[3] == "ERR" && preg_match ('/^\\[[^\\]]+#\\d+\\]\\s+(Cached|Retrieved)\\s+data\\s+from\\s+.*\\)\\s+does\\s+not\\s+match\\s+specified\\s+criteria\\.?$/', $decodedAnswer)) {
                return ($matches[1]);
            } else if ($what == STDOUT_EXPECT_NOHELPER_FALSE && $matches[3] == "ERR" && preg_match ('/^Unable\\s+to\\s+find\\s+a\\s+helper\\s+that\\s+can\\s+handle\\s+the\\s+requested\\s+URL\\s*;\\s+sending\\s+a\\s+NOMATCH\\s+response\\s+/', $decodedAnswer)) {
                return ($matches[1]);
            } else if ($what == STDOUT_EXPECT_NOHELPER_TRUE && $matches[3] == "OK" && preg_match ('/^Unable\\s+to\\s+find\\s+a\\s+helper\\s+that\\s+can\\s+handle\\s+the\\s+requested\\s+URL\\s*;\\s+sending\\s+a\\s+MATCH\\s+response\\s+/', $decodedAnswer)) {
                return ($matches[1]);
            } else if ($what == STDOUT_EXPECT_ERROR && $matches[3] == "BH") {
                return ($matches[1]);
            }
        }
    }
    if ($decodedAnswer) {
        msg_warning ("STDOUT did not return an expected '" . $what . "' response: '" . $line . "' ('" . $decodedAnswer . "')");
    } else {
        msg_warning ("STDOUT did not return an expected '" . $what . "' response: '" . $line . "'");
    }
    $GLOBALS['exitCode'] = 1;
    return (false);
}

function stderrExpectAnswer ($timeout = null) {
    if (empty ($timeout)) {
        $timeout = EXPECT_DEFAULT_TIMEOUT;
    }
    return (stderrExpect('INFO:\\s*Channel\\s+#\\d*\\s+answered\\s*:\\s+', $timeout));
}

function stdoutExpectNoHelperFalse ($timeout = null) {
    if (empty ($timeout)) {
        $timeout = EXPECT_DEFAULT_TIMEOUT;
    }
    return (stdoutExpect (STDOUT_EXPECT_NOHELPER_FALSE, $timeout) !== false && stderrExpectAnswer ($timeout));
}

function stdoutExpectNoHelperTrue ($timeout = null) {
    if (empty ($timeout)) {
        $timeout = EXPECT_DEFAULT_TIMEOUT;
    }
    return (stdoutExpect (STDOUT_EXPECT_NOHELPER_TRUE, $timeout) !== false && stderrExpectAnswer ($timeout));
}

function stdoutExpectError ($timeout = null) {
    if (empty ($timeout)) {
        $timeout = EXPECT_DEFAULT_TIMEOUT;
    }
    return (stdoutExpect (STDOUT_EXPECT_ERROR, $timeout) !== false && stderrExpect ('WARNING:\\s*An\\s+internal\\s+error\\s+occurred\\s+while\\s+processing\\s+the\\s+request(|\\s+from\\s+channel\\s+#-?\\d+)\\s*:\\s+', $timeout));
}

function includeWithScopeProtection ($file) {
    return ((include ($file)));
}

function randomChannel () {
    return (rand (0, 60));
}
