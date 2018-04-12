// API reference: https://developers.google.com/youtube/v3/docs/

// This helper requires an implementation of the URL object: https://developer.mozilla.org/en-US/docs/Web/API/URL
require ("URL");

// This helper requires a global variable named 'v3ApiKey', which stores the API key created
// using instructions located in https://developers.google.com/youtube/registering_an_application#Create_API_Keys
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
                    answerClassName = dir + "s";
                    answerId = subdir;
                } else if (dir == "v" || (dir == "embed" && subdir != "videoseries")) {
                    answerClassName = "videos";
                    answerId = subdir;
                } else if (dir == "embed" && subdir == "videoseries") {
                    answerClassName = "playlists";
                    answerId = urlObj.searchParams.getAll("list").slice(-1)[0];
                }
            } else {
                dir = urlObj.pathname.substring (1);
            }
            if (dir == "watch") {
                answerClassName = "videos";
                answerId = urlObj.searchParams.getAll("v").slice(-1)[0];
            } else if (dir == "get_video_info") {
                answerClassName = "videos";
                answerId = urlObj.searchParams.getAll("video_id").slice(-1)[0];
            } else if (urlObj.hostname == "youtu.be") {
                answerClassName = "videos";
                answerId = dir;
            }
            if (answerClassName && answerId) {
                answer = {
                    "className": answerClassName,
                    "id": answerId
                };
            }
        }
        returnValue (answer);
    } else {
        throw new URIError ("Invalid URL: '" + url + "'!");
    }
}

function getPropertiesFromObject (returnValue, className, id) {
    var path = className;
    var param = "id";
    if (className == "users") {
        path = "channels";
        param = "forUsername";
    }
    var xhr = new XMLHttpRequest ();
    var youtubeURL = "https://www.googleapis.com/youtube/v3/" + path + "?" + param + "=" + encodeURIComponent(id) + "&part=" + encodeURIComponent(part) + "&key=" + encodeURIComponent (v3ApiKey);
    var async = true;
    xhr.open ("GET", youtubeURL, async);
    xhr.timeout = 120000;
    xhr.responseType = "json";
    xhr.onloadend = function () {
        var answer = null;
        if (xhr.status >= 200 && xhr.status < 300) {
            answer = xhr.response;
            if (answer.items.length) {
                console.log ("Data about (className='" + className + "', id='" + id + "') was retrieved successfully.");
                answer = answer.items[0];
                if (answer["snippet"] && answer["snippet"]["categoryId"]) {
                    getPropertiesFromObjectCache (function (data) {
                        if (data["snippet"]) {
                            answer["snippet"]["category"] = data["snippet"];
                        }
                        returnValue (answer);
                    }, "videoCategories", answer["snippet"]["categoryId"]);
                    return;
                }
            } else {
                console.warn ("Retrieved data about (className='" + className + "', id='" + id + "') is invalid: '" + JSON.stringify(answer) + "'");
                answer = null;
            }
        } else {
            console.error ("Data retrieval about (className='" + className + "', id='" + id + "') failed! status='" + xhr.status + ": " + xhr.statusText + "'");
        }
        returnValue (answer);
    };
    if (async) {
        xhr.send ();
    } else {
        try {
            xhr.send ();
            xhr.onloadend ();
        } catch (e) {
            console.error ("XMLHttpRequest exception while retrieving data about (className='" + className + "', id='" + id + "'): " + e.toString());
            returnValue (null);
        }
    }
}
