if (! v3ApiKey) {
    throw new ReferenceError ("Global variable 'v3ApiKey' must be set in configuration!");
}

let helperRegExp = /^https?:\/\/(([^\/]+\.)?youtube\.com(\.\w\w)?|youtu\.be)(:[0-9]+)?\//i;

function getSupportedUrls (returnValue) {
    returnValue ([helperRegExp]);
}

function getObjectFromUrl (returnValue, url) {
    if (helperRegExp.test (url)) {
        let answer, dir, subdir, pos, answerClassName, answerId;
        // This function requires an implementation of the URL object: https://developer.mozilla.org/en-US/docs/Web/API/URL
        require ("URL");
        let urlObj = new URL (url);
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
    // throw new TypeError ("'getPropertiesFromObject (returnValue, className, id);' is not implemented yet!");
    setTimeout (function () {
        returnValue ({
            "foo": "bar",
            "id": 2345
        });
    }, 1);
}
