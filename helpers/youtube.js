//////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////

// API reference: https://developers.google.com/youtube/v3/docs/

// Required global variables

// This helper requires a global variable named 'v3ApiKey', which stores the API key created
// using instructions located at https://developers.google.com/youtube/registering_an_application#Create_API_Keys
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
        // @disable-check M126
        if (urlObj.pathname.substring (0, 1) == "/") {
            pos = urlObj.pathname.indexOf ("/", 1);
            if (pos >= 0) {
                dir = urlObj.pathname.substring (1, pos);
                pos = urlObj.pathname.indexOf ("/", pos + 1);
                if (pos < 0) {
                    pos = urlObj.pathname.length;
                }
                subdir = urlObj.pathname.substring (dir.length + 2, pos);
                // @disable-check M126
                if (dir == "user" || dir == "channel") {
                    answerClassName = dir + "s";
                    answerId = subdir;
                // @disable-check M126
                } else if (dir == "v" || (dir == "embed" && subdir != "videoseries")) {
                    answerClassName = "videos";
                    answerId = subdir;
                // @disable-check M126
                } else if (dir == "embed" && subdir == "videoseries") {
                    answerClassName = "playlists";
                    answerId = urlObj.searchParams.getAll("list").slice(-1)[0];
                }
            } else {
                dir = urlObj.pathname.substring (1);
            }
            // @disable-check M126
            if (dir == "watch") {
                answerClassName = "videos";
                answerId = urlObj.searchParams.getAll("v").slice(-1)[0];
            // @disable-check M126
            } else if (dir == "get_video_info") {
                answerClassName = "videos";
                answerId = urlObj.searchParams.getAll("video_id").slice(-1)[0];
            // @disable-check M126
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
        returnValue (null);
    }
}

function getPropertiesFromObject (returnValue, className, id) {
    var path = className;
    var param = "id";
    // @disable-check M126
    if (className == "users") {
        path = "channels";
        param = "forUsername";
    }
    // "snippet" part is required!
    // For "videoCategories", only "snippet" is accepted as "part" argument
    var _part = "snippet," + part;
    // @disable-check M126
    if (className == "videoCategories") {
        _part = "snippet";
    }
    var youtubeURL = "https://www.googleapis.com/youtube/v3/" + path + "?" + param + "=" + encodeURIComponent(id) + "&part=" + encodeURIComponent(_part) + "&key=" + encodeURIComponent (v3ApiKey);
    var jsonResolver = new GenericJsonResolver (returnValue, className, id, getPropertiesFromObjectCache);
    jsonResolver.get (youtubeURL)
        .extract ("items")
        .extract (0);
    // @disable-check M126
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

//////////////////////////////////////////////////////////////////
