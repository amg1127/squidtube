// API reference: https://developers.google.com/youtube/v3/docs/

// Required global variables

// This helper requires a global variable named 'v3ApiKey', which stores the API key created
// using instructions located in https://developers.google.com/youtube/registering_an_application#Create_API_Keys
if (! v3ApiKey) {
    throw new ReferenceError ("Global variable 'v3ApiKey' must be set in configuration!");
}

// Optional global variables

// This helper accepts a global variable named 'part', which will be forwarded directly to YouTube API endpoint
// If unset, it defaults to 'snippet'.
// Check YouTube v3 API documentation for details: https://developers.google.com/youtube/v3/docs/standard_parameters
if (! part) {
    var part = "snippet";
}

// Internal variables
var helperRegExp = /^https?:\/\/(([^\/]+\.)?youtube\.com(\.\w\w)?|youtu\.be)(:[0-9]+)?\//i;

// Required common libraries
require ("URL");
require ("GenericJsonResolver");

//////////////////////////////////////////////////////////////////

// Functions go here

function getSupportedUrls (returnValue) {
    returnValue ([helperRegExp]);
}

function getObjectFromUrl (returnValue, url) {
    if (helperRegExp.test (url)) {
        var answer, dir, subdir, pos, answerClassName, answerId;
        var urlObj = new URL (url);
        if (urlObj.pathname.substring (0, 1) == "/") {
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
    // "snippet" part is required!
    var youtubeURL = "https://www.googleapis.com/youtube/v3/" + path + "?" + param + "=" + encodeURIComponent(id) + "&part=" + encodeURIComponent("snippet," + part) + "&key=" + encodeURIComponent (v3ApiKey);
    var jsonResolver = new GenericJsonResolver (returnValue, className, id, getPropertiesFromObjectCache);
    jsonResolver.get (youtubeURL)
        .extract ("items")
        .extract (0);
    if (className == "videos") {
        jsonResolver.add ({
            "snippet.category": {
                "objectClass": "videoCategories",
                "id"         : "snippet.categoryId",
                "extract"    : "snippet"
            }
        });
    }
}
