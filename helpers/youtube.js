// This helper requires an implementation of the URL object: https://developer.mozilla.org/en-US/docs/Web/API/URL
require ("URL");

// This helper requires a global variable named 'v3ApiKey', which stores the
if (! v3ApiKey) {
    throw new ReferenceError ("Global variable 'v3ApiKey' must be set in configuration!");
}

// This helper accepts a global variable named 'part', which will be forwarded directly to YouTube API endpoint
// If unset, it defaults to 'snippet'.
// Check YouTube v3 API documentation for details: https://developers.google.com/youtube/v3/docs/standard_parameters
if (! part) {
    var part = "snippet";
}

// This variable is used internally
var helperRegExp = /^https?:\/\/(([^\/]+\.)?youtube\.com(\.\w\w)?|youtu\.be)(:[0-9]+)?\//i;

function getSupportedUrls (returnValue) {
    returnValue ([helperRegExp]);
}

function getObjectFromUrl (returnValue, url) {
    if (helperRegExp.test (url)) {
        var answer, dir, subdir, pos, answerClassName, answerId;
        var urlObj = new URL (url);
        if (urlObj.pathname.startsWith ("/")) {
            pos = urlObj.pathname.indexOf ("/", 1);
            if (pos >= 0) {
                dir = urlObj.pathname.substring (1, pos);
                pos = urlObj.pathname.indexOf ("/", pos + 1);
                if (pos < 0) {
                    pos = urlObj.pathname.length;
                }
                subdir = urlObj.pathname.substring (dir.length + 2, pos);
                if (dir == "user" || dir == "channel") {
                    answerClassName = dir;
                    answerId = subdir;
                } else if (dir == "v" || (dir == "embed" && subdir != "videoseries")) {
                    answerClassName = "video";
                    answerId = subdir;
                } else if (dir == "embed" && subdir == "videoseries") {
                    answerClassName = "playlist";
                    answerId = urlObj.searchParams.getAll("list").slice(-1)[0];
                }
            } else {
                dir = urlObj.pathname.substring (1);
            }
            if (dir == "watch") {
                answerClassName = "video";
                answerId = urlObj.searchParams.getAll("v").slice(-1)[0];
            } else if (dir == "get_video_info") {
                answerClassName = "video";
                answerId = urlObj.searchParams.getAll("video_id").slice(-1)[0];
            } else if (urlObj.hostname == "youtu.be") {
                answerClassName = "video";
                answerId = dir;
            }
            if (answerClassName && answerId) {
                answer = {
                    "className": answerClassName,
                    "id": answerId
                };
            }
        }
        setTimeout (function () {
            returnValue (answer);
        }, 1);
    } else {
        throw new URIError ("Invalid URL: '" + url + "'!");
    }
}

function getPropertiesFromObject (returnValue, className, id) {
    var path = className + "s";
    var param = "id";
    if (className == "user") {
        path = "channels";
        param = "forUsername";
    }
    var xhr = new XMLHttpRequest ();
    var youtubeURL = "https://www.googleapis.com/youtube/v3/" + path + "?" + param + "=" + escape(id) + "&part=" + escape(part) + "&key=" + escape (v3ApiKey);
    var async = false;
    xhr.open ("GET", youtubeURL, async);
    xhr.timeout = 120000;
    xhr.responseType = "json";
    xhr.onloadend = function () {
        if (xhr.status >= 200 && xhr.status < 300) {
            console.log ("Data about className='" + className + "' and ID='" + id + "' was retrieved successfully.");
            returnValue (xhr.response.items[0]);
        } else {
            console.error ("Data about className='" + className + "' and ID='" + id + "' failed! status='" + xhr.status + ": " + xhr.statusText + "'");
            returnValue (xhr.response);
        }
    };
    if (async) {
        xhr.send ();
    } else {
        try {
            xhr.send ();
            xhr.onloadend ();
        } catch (e) {
            console.error ("XMLHttpRequest exception: " + e);
            returnValue (null);
        }
    }
}
