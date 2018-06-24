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

// Partial implementation of the URL object, according to specification found at https://developer.mozilla.org/en-US/docs/Web/API/URL

try {

    if (! URL.prototype) {
        throw new ReferenceError ("There is no native URL() implementation here...");
    }

} catch (e) {

    var URL = (function () {
        // @disable-check M127
        "use strict";

        function stringStartsWith (str, substr, position) {
            // Added in Qt 5.8
            if (str.startsWith) {
                return (str.startsWith (substr, position));
            } else {
                var _start = 0;
                if (position) {
                    _start = position;
                }
                // @disable-check M126
                return (str.substring (_start, _start + substr.length) == substr);
            }
        }

        function stringEndsWith (str, substr, position) {
            // Added in Qt 5.8
            if (str.endsWith) {
                return (str.endsWith (substr, position));
            } else {
                var _end = str.length;
                if (position) {
                    _end = position;
                }
                // @disable-check M126
                return (str.substring (_end - substr.length, _end) == substr);
            }
        }

        function path_resolution (path) {
            if (path) {
                if (stringEndsWith (path, "/.") || stringEndsWith (path, "/..")) {
                    path += "/";
                }
                var pathComponents = path.split("/");
                var pathLength = pathComponents.length;
                var resolvedPath = [];
                var i;
                var pathComponent;
                for (i = 0; i < pathLength; i++) {
                    pathComponent = pathComponents[i];
                    // @disable-check M126
                    if (pathComponent == "..") {
                        resolvedPath.pop();
                    } else {
                        // @disable-check M126
                        if (pathComponent && pathComponent != ".") {
                            resolvedPath.push (pathComponent);
                        }
                    }
                }
                return (((pathComponents[0]) ? "" : "/") + resolvedPath.join ("/") + ((pathComponents[pathLength-1]) ? "" : "/"));
            } else {
                return ("");
            }
        }

        function URLSearchParams (searchComponents) {
            var length = searchComponents.length;

            Object.defineProperties (this, {
                "get"     : { "value": (function (param) {
                    var escapedParam = encodeURIComponent (param) + "=";
                    var i;
                    for (i = 0; i < length; i++) {
                        if (stringStartsWith (searchComponents[i], escapedParam)) {
                            return (decodeURIComponent (searchComponents[i].substring (escapedParam.length)));
                        }
                    }
                    return (null);
                })},
                "getAll"  : { "value": (function (param) {
                    var returnValue = [];
                    var escapedParam = encodeURIComponent (param) + "=";
                    var i;
                    for (i = 0; i < length; i++) {
                        if (stringStartsWith (searchComponents[i], escapedParam)) {
                            returnValue.push (decodeURIComponent (searchComponents[i].substring (escapedParam.length)));
                        }
                    }
                    return (returnValue);
                })},
                "toString": { "value": (function () {
                    if (searchComponents.length) {
                        return ("?" + searchComponents.join("&"));
                    } else {
                        return ("");
                    }
                })}
            });
            Object.defineProperty (this, "has", { "value": (function () {
                    if (searchComponents.length) {
                        return ("?" + searchComponents.join("&"));
                    } else {
                        return ("");
                    }
                })
            });
        }

        var regexpAbsoluteUrl = /^(|[^:\s]+:)((|\/\/)(|([^:@\/\s]+)(|:([^@\/\s]*))@)((([\w-_]+\.)*[\w-_]+\.?)(|:(\d+))))(|\/[^?#\s]*)(|\?[^#\s]*)(|#.*)$/;
        var regexpRelativeUrl = /^(|\.?\.?\/[^?#\s]*)(|\?[^#\s]*)(|#.*)$/;

        function URL (urlString, baseString) {

            if (! (this instanceof URL)) {
                return (new URL (urlString, baseString));
            }

            var absoluteUrlCaptures = regexpAbsoluteUrl.exec (urlString);
            if (absoluteUrlCaptures) {
                Object.defineProperties (this, {
                    "protocol"    : { "value": absoluteUrlCaptures[1] },
                    "username"    : { "value": absoluteUrlCaptures[5] },
                    "password"    : { "value": absoluteUrlCaptures[7] },
                    "host"        : { "value": absoluteUrlCaptures[8] },
                    "hostname"    : { "value": absoluteUrlCaptures[9] },
                    "port"        : { "value": absoluteUrlCaptures[12] },
                    "pathname"    : { "value": path_resolution (absoluteUrlCaptures[13]) },
                    "search"      : { "value": absoluteUrlCaptures[14] },
                    "hash"        : { "value": absoluteUrlCaptures[15] },
                    "origin"      : { "value": (absoluteUrlCaptures[1] + absoluteUrlCaptures[3] + absoluteUrlCaptures[8]) }
                });
                Object.defineProperties (this, {
                    "href"        : { "value": (absoluteUrlCaptures[1] + absoluteUrlCaptures[2] + this.pathname + this.search + this.hash) },
                    "searchParams": { "value": (new URLSearchParams (this.search.substring(1).split("&"))) }
                });
                Object.defineProperty (this, "toString", { "value": (function () { return (this.href); }) });
            } else {
                var relativeUrlCaptures = regexpRelativeUrl.exec (urlString);
                if (baseString && relativeUrlCaptures) {
                    var baseUrlObject = new URL (baseString);
                    Object.defineProperties (this, {
                        "protocol"    : { "value": baseUrlObject.protocol },
                        "username"    : { "value": baseUrlObject.username },
                        "password"    : { "value": baseUrlObject.password },
                        "host"        : { "value": baseUrlObject.host },
                        "hostname"    : { "value": baseUrlObject.hostname },
                        "port"        : { "value": baseUrlObject.port },
                        "search"      : { "value": ((relativeUrlCaptures[2]) ? relativeUrlCaptures[2] : baseUrlObject.search) },
                        "hash"        : { "value": ((relativeUrlCaptures[3]) ? relativeUrlCaptures[3] : baseUrlObject.hash) },
                        "origin"      : { "value": baseUrlObject.origin }
                    });
                    var pathname;
                    if (stringStartsWith (relativeUrlCaptures[1], "/")) {
                        pathname = path_resolution (relativeUrlCaptures[1]);
                    } else if (stringEndsWith (baseUrlObject.pathname, "/")) {
                        pathname = path_resolution (baseUrlObject.pathname + relativeUrlCaptures[1]);
                    } else {
                        pathname = path_resolution (baseUrlObject.pathname + "/../" + relativeUrlCaptures[1]);
                    }
                    Object.defineProperties (this, {
                        "pathname"    : { "value": pathname },
                        "href"        : { "value": (baseUrlObject.href.substring(0, baseUrlObject.href.length - baseUrlObject.pathname.length - baseUrlObject.search.length - baseUrlObject.hash.length) + this.pathname + this.search + this.hash) },
                        "searchParams": { "value": (new URLSearchParams (this.search.substring(1).split("&"))) },
                    });
                    Object.defineProperty (this, "toString", { "value": (function () { return (this.href); }) });
                } else {
                    throw new TypeError ("URL syntax is not valid!");
                }
            }
        }

        return (URL);
    }) ();
}
