<?php

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
    // Process and file descriptors for communication with the program
    $GLOBALS['projectProc'] = null;
    $GLOBALS['projectSTDIN'] = null;
    $GLOBALS['projectSTDOUT'] = null;
    $GLOBALS['projectSTDERR'] = null;

    // This is used by stdoutExpect() function
    define ('STDOUT_EXPECT_MATCH', 'MATCH');
    define ('STDOUT_EXPECT_NOMATCH', 'NOMATCH');
    define ('STDOUT_EXPECT_NOHELPER', 'NOHELPER');

    // The project name is expected to be supplied by "qmake"...
    if (empty ($GLOBALS['argv'][1])) {
        msg_fatal ("Unable to retrieve the project name!");
    } else {
        $projectName = $GLOBALS['argv'][1];
    }

    // Try to launch a local web server using PHP's builtin one
    $serverProcess = null;

    if ((int) ini_get ('allow_url_fopen')) {
        for ($launchTry = 1; (! $serverProcess) && $launchTry <= 5; $launchTry++) {
            $serverPort = rand (1024, 65535);
            msg_log ("[" . $launchTry . "] Trying to launch a local web server at TCP port " . $serverPort . "...");
            $GLOBALS['serverAddress'] = "localhost:" . $serverPort;
            $serverProcessTentative = proc_open ("php -S " . escapeshellarg ($GLOBALS['serverAddress']) . " " . escapeshellarg (__FILE__), array (
                0 => array ("pipe", "r"),
                1 => STDOUT,
                2 => STDERR
            ), $pipes, __DIR__);
            if ($serverProcessTentative) {
                fclose ($pipes[0]);
                msg_log ("Server seems to be running now.");
                $GLOBALS['serverAddress'] = "http://" . $GLOBALS['serverAddress'];
                for ($connectTry = 1; (! $serverProcess) && $connectTry <= 10; $connectTry++) {
                    msg_log ("[" . $connectTry . "] Trying to establish a HTTP connection...");
                    usleep (500000);
                    $random_answer = __FILE__ . "_" . rand ();
                    $data = file_get_contents ($GLOBALS['serverAddress'] . "/ping_test/?mirror=" . rawurlencode ($random_answer));
                    if (trim ($data) == $random_answer) {
                        $serverProcess = $serverProcessTentative;
                    }
                }
                if (! $serverProcess) {
                    shutdownProcess ($serverProcessTentative, "Server did not send the expected answer.");
                }
            }
        }
    } else {
        msg_warning ("'allow_url_fopen' is disabled!");
    }

    if (! $serverProcess) {
        msg_fatal ("Unable to launch a local web server!");
    }

    $projectLogFile = __DIR__ . "/" . basename (__FILE__, '.php') . ".log";
    if (file_put_contents ($projectLogFile, '') !== 0) {
        shutdownProcess ($serverProcess);
        msg_fatal ("Unable to create log file '" . $projectLogFile . "'!");
    }

    $GLOBALS['projectProc'] = proc_open (escapeshellarg(__DIR__ . "/../" . $projectName) . " --main.loglevel=DEBUG --config " . escapeshellarg (__DIR__ . "/test_config.conf"), array (
        0 => array ("pipe", "r"),
        1 => array ("pipe", "w"),
        2 => array ("file", $projectLogFile, "w")
    ), $pipes, __DIR__);
    if (! $GLOBALS['projectProc']) {
        shutdownProcess ($serverProcess);
        unlink ($projectLogFile);
        msg_fatal ("Unable to launch the project executable!");
    }
    $GLOBALS['projectSTDIN'] = $pipes[0];
    $GLOBALS['projectSTDOUT'] = $pipes[1];
    $GLOBALS['projectSTDERR'] = fopen ($projectLogFile, "rt");
    if (! $GLOBALS['projectSTDERR']) {
        fclose ($GLOBALS['projectSTDIN']);
        if (waitGracefulFinish () === false) {
            shutdownProcess ($GLOBALS['projectProc']);
        }
        unlink ($projectLogFile);
        shutdownProcess ($serverProcess);
        msg_fatal ("Unable to open log file '" . $projectLogFile . "' for reading!");
    }

    $unblocked = true;
    foreach (array ($GLOBALS['projectSTDOUT'], $GLOBALS['projectSTDERR']) as $fd) {
        if (! stream_set_blocking ($fd, false)) {
            $unblocked = false;
        }
    }
    if (! $unblocked) {
        msg_warning ("Unable to set nonblocking mode for file descriptors!");
        fclose ($GLOBALS['projectSTDIN']);
        fclose ($GLOBALS['projectSTDERR']);
        if (waitGracefulFinish () === false) {
            shutdownProcess ($GLOBALS['projectProc']);
        }
        shutdownProcess ($serverProcess);
        unlink ($projectLogFile);
        msg_fatal ("Tests can not run without setting communication file descriptors to nonblocking mode!");
    }

    // Wait program startup
    if (stderrExpect ('INFO:\\s*StdinReader\\s+thread\\s+started\\.')) {
        msg_log ("We are all set for the tests. Let's go!");

        $testFiles = array ();
        $testDirPath = __DIR__ . "/suite";
        $testDir = opendir ($testDirPath);
        if ($testDir) {
            while (($testFile = readdir ($testDir)) !== false) {
                if (preg_match ('/[a-zA-Z0-9_-]+\\.php/i', $testFile)) {
                    array_push ($testFiles, $testDirPath . "/" . $testFile);
                }
            }
            closedir ($testDir);
        }
        sort ($testFiles);
        foreach ($testFiles as $testFile) {
            $procStatus = getProcessExitCode ();
            if ($procStatus !== false) {
                msg_warning ("Project executable ended unexpectedly with code #" . $procStatus . "! Did it crash?");
                $GLOBALS['exitCode'] = 1;
                break;
            }
            $testName = basename ($testFile, '.php');
            msg_log ("  + Running test '" . $testName . "'...");
            if (includeWithScopeProtection ($testFile)) {
                $GLOBALS['exitCode'] = 0;
            } else {
                msg_warning ("Test '" . $testName . "' did not run successfully!");
                $GLOBALS['exitCode'] = 1;
                break;
            }
        }
    }

    fclose ($GLOBALS['projectSTDIN']);
    if (stderrExpect ('INFO:\\s*StdinReader\\s+thread\\s+finished\\.')) {
        stderrExpect ('DEBUG:\\s*Performing\\s+final\\s+cleaning\\s+and\\s+finishing\\s+program\\.\\.\\.');
    }
    $finishStatus = waitGracefulFinish ();
    if ($finishStatus !== 0) {
        $GLOBALS['exitCode'] = 1;
        if ($finishStatus === false) {
            msg_warning ("Project executable is still running!");
            shutdownProcess ($GLOBALS['projectProc']);
        } else if ($finishStatus !== 0) {
            msg_warning ("Project executable ended with return code #" . $finishStatus . ".");
        }
    }
    shutdownProcess ($serverProcess, "Tests finished.");
    msg_log ("Test program exited with status code #" . $GLOBALS['exitCode'] . ".");
    exit ($GLOBALS['exitCode']);
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
    $procStatus = proc_get_status ($GLOBALS['projectProc']);
    if ($procStatus) {
        if (! $procStatus['running']) {
            return ($procStatus['exitcode']);
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
    implode (" ", array_map ('rawurlencode', preg_split ('/\\s/', $testFlags, -1, PREG_SPLIT_NO_EMPTY))) . " ";
    if (is_array ($testCriteria)) {
        $line .= implode (" ", array_map ('rawurlencode', $testCriteria));
    } else {
        $line .= rawurlencode ($testCriteria);
    }
    $line = trim ($line) . "\n";
    $lineLength = strlen ($line);
    for ($written = 0; $written < $lineLength; $written += $bytes) {
        $bytes = fwrite ($GLOBALS['projectSTDIN'], substr ($line, $written));
        if ($bytes === false) {
            break;
        }
    }
    if ($bytes !== false) {
        return (true);
    } else {
        $GLOBALS['exitCode'] = 1;
        msg_warning ("Unable to send a request line to the program!");
        return (false);
    }
}

function readLineFromDescriptor ($fd) {
    $line = stream_get_line ($fd, 32768, "\n");
    if ($line === false) {
        $start = time ();
        do {
            usleep (250000);
            if (($line = stream_get_line ($fd, 32768, "\n")) !== false) {
                break;
            }
            $procStatus = getProcessExitCode ();
            if ($procStatus !== false) {
                msg_warning ("Project executable ended unexpectedly with code #" . $procStatus . "! Did it crash?");
                $GLOBALS['exitCode'] = 1;
                break;
            }
            msg_warning ('Input is not ready yet. Waiting...');
        } while ((time() - $start) < 300);
        if ($line === false) {
            msg_warning ('Unable to read input!');
        }
    }
    return ($line);
}

function stderrExpect ($regexp) {
    while (($line = readLineFromDescriptor ($GLOBALS['projectSTDERR'])) !== false) {
        $line = trim (preg_replace ('/^\\s*\\[\\d\\d\\d\\d-\\d\\d-\\d\\dT\\d\\d:\\d\\d:\\d\\d(|\\.\\d+)(Z|UTC|[+-]\\d+(|:\\d+))\\]\\s+(|0[xX][a-fA-F0-9]+\\s+)(\\w+\\s*:\\s)/', '\\5', $line));
        if (preg_match ('/^' . $regexp . '/', $line)) {
            return (true);
        }
    }
    $GLOBALS['exitCode'] = 1;
    msg_warning ("Pattern was not found on messages coming from STDERR: '" . $regexp . "'!");
    return (false);
}

function stdoutExpect ($what) {
    if (($line = readLineFromDescriptor ($GLOBALS['projectSTDOUT'])) !== false) {
        $line = trim ($line);
        if (preg_match ('/^\\s*(|(\\d+)\\s+)(OK|ERR|BH)\\s+message=(\\S+)\\s+log=\\S+\\s*$/', $line, $matches)) {
            $matches[4] = urldecode ($matches[4]);
            if ($what == STDOUT_EXPECT_MATCH && $matches[3] == "OK" && preg_match ('/^(Cached|Retrieved)\\s+data\\s+from\\s+.*\\)\\s+matches\\s+specified\\s+criteria\\.?$/', $matches[4])) {
                return ($matches[1]);
            } else if ($what == STDOUT_EXPECT_NOMATCH && $match[3] == "ERR" && preg_match ('/^(Cached|Retrieved)\\s+data\\s+from\\s+.*\\)\\s+does\\s+not\\s+match\\s+specified\\s+criteria\\.?$/', $matches[4])) {
                return ($matches[1]);
            } else if ($what == STDOUT_EXPECT_NOHELPER && $match[3] == "ERR" && preg_match ('/^Unable\\s+to\\s+find\\s+a\\s+helper\\s+/', $matches[4])) {
                return ($matches[1]);
            }
        }
    }
    $GLOBALS['exitCode'] = 1;
    msg_warning ("STDOUT did not return an expected '" . $what . "' response: '" . $line . "'");
    return (false);
}

function stderrExpectAnswer () {
    return (stderrExpect('INFO:\\s*Channel\\s+#\\d*\\s+answered\\s*:\\s+'));
}

function stdoutExpectMatch () {
    return (stdoutExpect (STDOUT_EXPECT_MATCH) && stderrExpectAnswer ());
}

function stdoutExpectNoMatch () {
    return (stdoutExpect (STDOUT_EXPECT_NOMATCH) && stderrExpectAnswer ());
}

function stdoutExpectNoHelper () {
    return (stdoutExpect (STDOUT_EXPECT_NOHELPER) && stderrExpectAnswer ());
}

function includeWithScopeProtection ($file) {
    $includeResult = ((include ($file)));
    return ($includeResult);
}
