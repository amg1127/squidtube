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

// API reference: https://www.flickr.com/services/api/

// Required global variables

// This helper requires a global variable named 'api_key', which stores the API key
// created for Flickr API calls: https://www.flickr.com/services/api/misc.api_keys.html
if (! api_key) {
    throw new ReferenceError ("Global variable 'api_key' must be set in configuration!");
}

// Optional global variables

// Internal variables
// Reference: https://www.flickr.com/services/api/misc.urls.html
var photoSourceURLs = /^https?:\/\/[^\.\/]+\.static\.?flickr\.com(|:[0-9]+)\/+[^\/?]+\/+([^\/_?]+)(_[^\/_?])+\.\w+(|\?.*)$/i;
var webPageURLs = /^https?:\/\/(|www\.)flickr\.com(:[0-9]+)?\/+(people|photos)\/+([^\/?]+)(|\/+(sets|[^\/?]+)(|\/+([^\/?]+)(|\/+[^?]*)))(|\?.*)$/i;

// Required common libraries
require ("GenericJsonResolver");

//////////////////////////////////////////////////////////////////

// Functions go here

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
        // @disable-check M126
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
    // @disable-check M126
    if (className == "userIdByUrl") {
        method = "flickr.urls.lookupUser";
    }
    var flickrURLtemplate = "https://api.flickr.com/services/rest/?api_key=" + encodeURIComponent(api_key) + "&format=json&nojsoncallback=1";
    var flickrURL = flickrURLtemplate + "&method=" + encodeURIComponent(method);
    var idObj = JSON.parse (id);
    Object.keys(idObj).map (function (currentValue) {
        flickrURL += "&" + currentValue + "=" + encodeURIComponent(idObj[currentValue]);
    });
    var jsonResolver = new GenericJsonResolver (function (data) {
        returnValue (flickrFixJSONFormatting (data));
    }, className, id, getPropertiesFromObjectCache);
    jsonResolver.setErrorHandler (function (step, residual) {
        // @disable-check M126
        if (step == 1) {
            // @disable-check M126
            if (residual["stat"] == "fail") {
                // @disable-check M126
                if ((className == "people" && residual["code"] == 1) ||
                    // @disable-check M126
                    (className == "photosets" && residual["code"] == 2)) {
                    console.log ("Flickr API did not find an user with (nsid='" + idObj.user_id + "'). Calling 'urls.lookupUser' in order to get the actual 'nsid'...");
                    getPropertiesFromObjectCache (function (data) {
                        if (data) {
                            idObj.user_id = data.user.id;
                            getPropertiesFromObjectCache (returnValue, className, JSON.stringify (idObj));
                        } else {
                            returnValue (null);
                        }
                    }, "userIdByUrl", JSON.stringify({ "url": ("https://www.flickr.com/people/" + idObj.user_id + "/") }));
                    return (true);
                }
            }
        }
    });
    jsonResolver.get (flickrURL)
        .test ({"stat":"ok"})
        .remove ("stat");
    // @disable-check M126
    if (className == "photos") {
        jsonResolver.add ({"photo.owner": function (data) {
            return ({
                "objectClass": "people",
                "id"         : JSON.stringify ({
                    "user_id": data.photo.owner.nsid
                })
            });
        }});
    // @disable-check M126
    } else if (className == "photosets") {
        jsonResolver.add ({"photoset.owner": function (data) {
            return ({
                "objectClass": "people",
                "id"         : JSON.stringify ({
                    "user_id": data.photoset.owner
                })
            });
        }});
    }
}

function flickrFixJSONFormatting (jsonObject) {
    var keys, keysLength, keyPos;
    if (jsonObject) {
        if (Array.isArray (jsonObject)) {
            return (jsonObject.map (flickrFixJSONFormatting));
        } else if ((typeof jsonObject) == 'object') {
            keys = Object.keys (jsonObject);
            keysLength = keys.length;
            // @disable-check M126
            if (keysLength == 1 && keys[0] == '_content') {
                return (flickrFixJSONFormatting (jsonObject['_content']));
            } else {
                for (keyPos = 0; keyPos < keysLength; keyPos++) {
                    jsonObject[keys[keyPos]] = flickrFixJSONFormatting (jsonObject[keys[keyPos]]);
                }
            }
        }
    }
    return (jsonObject);
}

//////////////////////////////////////////////////////////////////
