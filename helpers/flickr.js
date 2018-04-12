// API reference: https://www.flickr.com/services/api/

// This helper requires a global variable named 'api_key', which stores the API key
// created for Flickr API calls: https://www.flickr.com/services/api/misc.api_keys.html
if (! api_key) {
    throw new ReferenceError ("Global variable 'api_key' must be set in configuration!");
}

// These variable are used internally
// Reference: https://www.flickr.com/services/api/misc.urls.html
var photoSourceURLs = /^https?:\/\/[^\.\/]+\.static\.?flickr\.com(|:[0-9]+)\/+[^\/?]+\/+([^\/_?]+)(_[^\/_?])+\.\w+(|\?.*)$/i;
var webPageURLs = /^https?:\/\/(|www\.)flickr\.com(:[0-9]+)?\/+(people|photos)\/+([^\/?]+)(|\/+(sets|[^\/?]+)(|\/+([^\/?]+)(|\/+[^?]*)))(|\?.*)$/i;

function getSupportedUrls (returnValue) {
    returnValue ([photoSourceURLs, webPageURLs]);
}

function getObjectFromUrl (returnValue, url) {
    var captures, userId, photoId, photosetId;
    if ((captures = photoSourceURLs.exec (url)) != null) {
        returnValue ({
            "className": "photos",
            "id": JSON.stringify ({
                "photo_id": captures[2]
            })
        });
    } else if ((captures = webPageURLs.exec (url)) != null) {
        userId = captures[4];
        photoId = captures[6];
        if (photoId == "sets" || photoId == "albums") {
            photoId = null;
            photosetId = captures[8];
        } else {
            photosetId = null;
        }
        if (photoId) {
            returnValue ({
                "className": "photos",
                "id": JSON.stringify ({
                    "photo_id": photoId
                })
            });
        } else if (photosetId) {
            returnValue ({
                "className": "photosets",
                "id": JSON.stringify ({
                    "photoset_id": photosetId,
                    "user_id": userId
                })
            });
        } else {
            returnValue ({
                "className": "people",
                "id": JSON.stringify ({
                    "user_id": userId
                })
            });
        }
    } else {
        returnValue (null);
    }
}

function getPropertiesFromObject (returnValue, className, id) {
    var method = "flickr." + className + ".getInfo";
    var flickrURLtemplate = "https://api.flickr.com/services/rest/?api_key=" + encodeURIComponent(api_key) + "&format=json&nojsoncallback=1";
    var flickrURL = flickrURLtemplate + "&method=" + encodeURIComponent(method);
    var idObj = JSON.parse(id);
    var keys = Object.keys (idObj);
    var keysLength = keys.length;
    var keyPos;
    for (keyPos = 0; keyPos < keysLength; keyPos++) {
        flickrURL += "&" + keys[keyPos] + "=" + encodeURIComponent(idObj[keys[keyPos]]);
    }
    var xhr = new XMLHttpRequest ();
    var async = true;
    xhr.open ("GET", flickrURL, async);
    xhr.timeout = 120000;
    xhr.responseType = "json";
    xhr.onloadend = function () {
        var jsonResponse;
        if (xhr.status >= 200 && xhr.status < 300) {
            try {
                jsonResponse = xhr.response;
            } catch (e) {
                console.warn ("Unable to parse Flickr JSON response: " + e.toString());
                jsonResponse = {
                    "stat": "fail",
                    "code": -1,
                    "message": ("Javascript exception while retrieving Flickr answer: " + e.toString())
                };
            }
            if (jsonResponse.stat == "ok") {
                delete jsonResponse.stat;
                jsonResponse = jsonResponse[Object.keys(jsonResponse)[0]];
                console.log ("Data about className='" + className + "' and ID='" + id + "' was retrieved successfully.");
                if (className == "photos" && jsonResponse.owner && jsonResponse.owner.nsid) {
                    getPropertiesFromObjectCache (function (data) {
                        jsonResponse["owner"] = data;
                        returnValue (flickrFixJSONFormatting (jsonResponse));
                    }, "people", JSON.stringify ({
                        "user_id": jsonResponse.owner.nsid
                    }));
                } else if (className == "photosets" && jsonResponse.owner) {
                    getPropertiesFromObjectCache (function (data) {
                        jsonResponse["owner"] = data;
                        returnValue (flickrFixJSONFormatting (jsonResponse));
                    }, "people", JSON.stringify ({
                        "user_id": jsonResponse.owner
                    }));
                } else {
                    returnValue (flickrFixJSONFormatting (jsonResponse));
                }
                return;
            } else if (idObj.user_id) {
                if ((className == "people" && jsonResponse.code == 1) ||
                    (className == "photoset" && jsonResponse.code == 2)) {
                    console.log ("Flickr API did not find an user with (nsid='" + idObj.user_id + "'). Calling 'urls.lookupUser' in order to get the actual 'nsid'...");
                    flickrURL = flickrURLtemplate + "&method=" + encodeURIComponent("flickr.urls.lookupUser") + "&url=" + encodeURIComponent("https://www.flickr.com/people/" + idObj.user_id + "/");
                    xhr.open ("GET", flickrURL, async);
                    xhr.onloadend = function () {
                        var otherJsonResponse;
                        if (xhr.status >= 200 && xhr.status < 300) {
                            try {
                                otherJsonResponse = xhr.response;
                            } catch (e) {
                                console.warn ("Unable to parse Flickr JSON response for 'flickr.urls.lookupUser(\"" + idObj.user_id + "\")': " + e.toString());
                                otherJsonResponse = {
                                    "stat": "fail",
                                    "code": -1,
                                    "message": ("Javascript exception while retrieving Flickr answer for 'flickr.urls.lookupUser(\"" + idObj.user_id + "\")': " + e.toString())
                                };
                            }
                            if (otherJsonResponse.stat == "ok") {
                                idObj.user_id = otherJsonResponse.user.id;
                                getPropertiesFromObjectCache (returnValue, className, JSON.stringify(idObj));
                                return;
                            }
                        } else {
                            console.error ("Data about className='" + className + "' and ID='" + id + "' failed! status='" + xhr.status + ": " + xhr.statusText + "'");
                        }
                        returnValue (null);
                    };
                    if (async) {
                        xhr.send();
                    } else {
                        try {
                            xhr.send ();
                            xhr.onloadend ();
                        } catch (e) {
                            console.error ("XMLHttpRequest exception while invoking 'flickr.urls.lookupUser(\"" + idObj.user_id + "\")': " + e.toString());
                            returnValue (null);
                        }
                    }
                    return;
                }
            }
        } else {
            console.error ("Data about className='" + className + "' and ID='" + id + "' failed! status='" + xhr.status + ": " + xhr.statusText + "'");
        }
        returnValue (null);
    };
    if (async) {
        xhr.send ();
    } else {
        try {
            xhr.send ();
            xhr.onloadend ();
        } catch (e) {
            console.error ("XMLHttpRequest exception while retrieving data about className='" + className + "' and ID='" + id + "': " + e.toString());
            returnValue (null);
        }
    }
}

function flickrFixJSONFormatting (jsonObject) {
    var keys, keysLength, keyPos;
    if (Array.isArray (jsonObject)) {
        return (jsonObject.map (flickrFixJSONFormatting));
    } else if ((typeof jsonObject) == 'object') {
        keys = Object.keys (jsonObject);
        keysLength = keys.length;
        keyPos;
        if (keysLength == 1 && keys[0] == '_content') {
            return (flickrFixJSONFormatting (jsonObject['_content']));
        } else {
            for (keyPos = 0; keyPos < keysLength; keyPos++) {
                jsonObject[keys[keyPos]] = flickrFixJSONFormatting (jsonObject[keys[keyPos]]);
            }
        }
    }
    return (jsonObject);
}
