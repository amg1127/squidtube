if (! v3ApiKey) {
    throw new ReferenceError ("Global variable 'v3ApiKey' must be set in configuration!");
}

var helperRegExp = /^https?:\/\/(([^\/]+\.)?youtube\.com(\.\w\w)?|youtu\.be)(:[0-9]+)?\//i;

function getSupportedUrls (returnValue) {
    returnValue ([helperRegExp]);
}

function getObjectFromUrl (returnValue, url) {
    var answer, dir, subdir, pos, answerClassName, answerId;
    // This function requires an implementation of the URL object: https://developer.mozilla.org/en-US/docs/Web/API/URL
    var urlObj = new URL (url);
    if (urlObj.path.startsWith ("/")) {
        pos = urlObj.path.indexOf ("/", 1);
        if (pos >= 0) {
            dir = urlObj.path.substring (1, pos);
            pos = urlObj.path.indexOf ("/", pos + 1);
            if (pos < 0) {
                pos = urlObj.path.length;
            }
            subdir = urlObj.path.substring (dir.length + 2, pos);
            if (dir == "user" || dir == "channel") {
                answerClassName = dir;
                answerId = subdir;
            } else if (dir == "v" || (dir == "embed" && subdir != "videoseries")) {
                answerClassName = "video";
                answerId = subdir;
            } else if (dir == "embed" && subdir == "videoseries") {
                answerClassName = "playlist";
                answerId = urlObj.searchParams.getAll("list").slice(-1)[0];
            } else if (dir == "watch") {
                answerClassName = "video";
                answerId = urlObj.searchParams.getAll("v").slice(-1)[0];
            } else if (dir == "get_video_info") {
                answerClassName = "video";
                answerId = urlObj.searchParams.getAll("video_id").slice(-1)[0];
            }
        } else if (urlObj.hostname == "youtu.be") {
            answerClassName = "video";
            answerId = urlObj.path.substring (1);
        }
        if (answerClassName && answerId) {
            answer = {
                "className": answerClassName,
                "id": answerId
            };
        }
    }
    returnValue (answer);
}

function getPropertiesFromObject (returnValue, className, id) {
    throw new TypeError ("'getPropertiesFromObject (returnValue, className, id);' is not implemented yet!");
}
