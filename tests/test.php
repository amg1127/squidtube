<?php
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
    *  if $_POST['key'] == 'value' and returns a HTTP 500 Server Error if the test does not succeed
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

    $redirects = 0;
    if (! empty ($_GET['redirects'])) {
        $redirects = (int) $_GET['redirects'];
        $queryString .= "&redirects=" . rawurlencode ($_GET['redirects']);
    }

    if (! empty ($_GET['expect'])) {
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
            }
        }
    }

    if (! empty ($_GET['mirror'])) {
        $queryString .= "&mirror=" . rawurlencode ($_GET['mirror']);
    }

    if ($response) {
        header (substr ($response, 4), true, (int) (substr ($response, 0, 3)));
        if (substr ($response, 0, 1) == '3' && $redirects > 0) {
            header ("Location: " . $_SERVER["SCRIPT_FILENAME"] . "?" . substr ($queryString, 1), true);
        }
    }

    if (! empty ($_GET['bigdata'])) {
        for ($i = 0; $i < 256; $i++) {
            echo (str_repeat (str_repeat (' ', 1023) . "\n", 1024));
            usleep (30 * 1000000 / 256);
        }
    }

    if (empty ($_GET['mirror'])) {
        echo ("{}\n");
    } else {
        echo ($_GET['mirror'] . "\n");
    }

    return (true);
}

function cli_sapi () {
    // Default exit code
    $GLOBALS['exitCode'] = 1;
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
    define ('STDOUT_EXPECT_ERROR', 'BH');

    // The project name is expected to be supplied by "qmake"...
    if (empty ($GLOBALS['argv'][1])) {
        msg_fatal ("Unable to retrieve the project name!");
    } else {
        $projectName = $GLOBALS['argv'][1];
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
        msg_fatal ("Unable to launch a local web server!");
    }

    $projectLogFile = __DIR__ . "/" . basename (__FILE__, '.php') . ".log";
    removeFile ($projectLogFile);
    if (file_put_contents ($projectLogFile, '') !== 0) {
        shutdownProcess ($GLOBALS['serverProcess']);
        msg_fatal ("Unable to create log file '" . $projectLogFile . "'!");
    }

    $databaseFile = __DIR__ . "/" . basename (__FILE__, '.php') . ".sqlite";
    foreach (array ('', '-wal', '-shm') as $suffix) {
        removeFile ($databaseFile . $suffix);
    }
    $GLOBALS['projectProcess'] = proc_open (escapeshellarg(__DIR__ . "/../" . $projectName) . " --main.loglevel=DEBUG --config " . escapeshellarg (__DIR__ . "/test_config.conf") . " --db.name " . escapeshellarg ($databaseFile), array (
        0 => array ("pipe", "rb"),
        1 => array ("pipe", "wt"),
        2 => array ("file", $projectLogFile, "wt")
    ), $pipes, __DIR__, null, array ("bypass_shell" => true));
    if (! $GLOBALS['projectProcess']) {
        shutdownProcess ($GLOBALS['serverProcess']);
        unlink ($projectLogFile);
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
        unlink ($projectLogFile);
        shutdownProcess ($GLOBALS['serverProcess']);
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
        sort ($testFiles);
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

function stdinSend ($channel, $urlPath, $jsonData, $testProperty, $testFlags, $testCriteria, $options = array ()) {
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
    $line = ((int) $channel) . " " . $GLOBALS['serverAddress'] . $urlPath . "?" . implode ("", array_map (function ($item) {
        return (rawurlencode ("expect[]") . "=" . rawurlencode ($item) . "&");
    }, $expect)) . $optionsData . "mirror=" . $mirror . " " . rawurlencode ($testProperty) . " " .
    implode (" ", array_map ('rawurlencode', preg_split ('/\\s/', $testFlags, -1, PREG_SPLIT_NO_EMPTY))) . " -- ";
    if (is_array ($testCriteria)) {
        $line .= implode (" ", array_map ('rawurlencode', $testCriteria));
    } else {
        $line .= rawurlencode ($testCriteria);
    }
    return (stdinSendRaw (trim ($line)));
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

function readLineFromDescriptor ($fd, $timeout = 30) {
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

function stderrExpect ($regexp, $timeout = 30) {
    while (($line = readLineFromDescriptor ($GLOBALS['projectSTDERR'], $timeout)) !== false) {
        $line = trim (preg_replace ('/^\\s*\\[\\d\\d\\d\\d-\\d\\d-\\d\\dT\\d\\d:\\d\\d:\\d\\d(|\\.\\d+)[^\\]]*\\]\\s+(|0[xX][a-fA-F0-9]+\\s+)(\\w+\\s*:\\s)/', '\\3', $line));
        if (preg_match ('/^' . $regexp . '/', $line)) {
            return (true);
        }
    }
    msg_warning ("Pattern was not found on messages coming from STDERR: '" . $regexp . "'!");
    $GLOBALS['exitCode'] = 1;
    return (false);
}

function stdoutExpect ($what, $timeout = 30) {
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
            } else if ($what == STDOUT_EXPECT_NOHELPER && $matches[3] == "ERR" && preg_match ('/^Unable\\s+to\\s+find\\s+a\\s+helper\\s+/', $decodedAnswer)) {
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

function stderrExpectAnswer ($timeout = 30) {
    return (stderrExpect('INFO:\\s*Channel\\s+#\\d*\\s+answered\\s*:\\s+', $timeout));
}

function stdoutExpectMatch ($timeout = 30) {
    return (stdoutExpect (STDOUT_EXPECT_MATCH, $timeout) !== false && stderrExpectAnswer ($timeout));
}

function stdoutExpectNoMatch ($timeout = 30) {
    return (stdoutExpect (STDOUT_EXPECT_NOMATCH, $timeout) !== false && stderrExpectAnswer ($timeout));
}

function stdoutExpectNoHelper ($timeout = 30) {
    return (stdoutExpect (STDOUT_EXPECT_NOHELPER, $timeout) !== false && stderrExpectAnswer ($timeout));
}
function stdoutExpectError ($timeout = 30) {
    return (stdoutExpect (STDOUT_EXPECT_ERROR, $timeout) !== false && stderrExpect ('WARNING:\\s*An\\s+internal\\s+error\\s+occurred\\s+while\\s+processing\\s+the\\s+request(|\\s+from\\s+channel\\s+#-?\\d+)\\s*:\\s+', $timeout));
}

function includeWithScopeProtection ($file) {
    return ((include ($file)));
}
