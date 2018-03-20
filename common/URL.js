// Partial implementation of the URL object, according to specification found at https://developer.mozilla.org/en-US/docs/Web/API/URL

try {
    if (! URL.prototype) {
        throw new ReferenceError ("There is no native URL() implementation here...");
    }
} catch (e) {

    var URL = function (urlString, baseString) {
        "use strict";

        function path_resolution (path) {
            if (path) {
                if (path.endsWith ("/.") || path.endsWith ("/..")) {
                    path += "/";
                }
                let pathComponents = path.split("/");
                let pathLength = pathComponents.length;
                let resolvedPath = [];
                let i;
                let pathComponent;
                for (i = 0; i < pathLength; i++) {
                    pathComponent = pathComponents[i];
                    if (pathComponent == "..") {
                        resolvedPath.pop();
                    } else if (pathComponent && pathComponent != ".") {
                        resolvedPath.push (pathComponent);
                    }
                }
                return (((pathComponents[0]) ? "" : "/") + resolvedPath.join ("/") + ((pathComponents[pathLength-1]) ? "" : "/"));
            } else {
                return ("");
            }
        }

        function URLSearchParams (searchComponents) {
            let length = searchComponents.length;

            Object.defineProperties (this, {
                "get"     : { "value": (function (param) {
                    let escapedParam = escape (param) + "=";
                    let i;
                    for (i = 0; i < length; i++) {
                        if (searchComponents[i].startsWith (escapedParam)) {
                            return (unescape (searchComponents[i].substring (escapedParam.length)));
                        }
                    }
                    return (null);
                })},
                "getAll"  : { "value": (function (param) {
                    let returnValue = [];
                    let escapedParam = escape (param) + "=";
                    let i;
                    for (i = 0; i < length; i++) {
                        if (searchComponents[i].startsWith (escapedParam)) {
                            returnValue.push (unescape (searchComponents[i].substring (escapedParam.length)));
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

        let regexpAbsoluteUrl = (/^(|[^:\s]+:)((|\/\/)(|([^:@\/\s]+)(|:([^@\/\s]*))@)((([\w-_]+\.)*[\w-_]+\.?)(|:(\d+))))(|\/[^?#\s]*)(|\?[^#\s]*)(|#.*)$/).exec (urlString);
        if (regexpAbsoluteUrl) {
            Object.defineProperties (this, {
                "protocol"    : { "value": regexpAbsoluteUrl[1] },
                "username"    : { "value": regexpAbsoluteUrl[5] },
                "password"    : { "value": regexpAbsoluteUrl[7] },
                "host"        : { "value": regexpAbsoluteUrl[8] },
                "hostname"    : { "value": regexpAbsoluteUrl[9] },
                "port"        : { "value": regexpAbsoluteUrl[12] },
                "pathname"    : { "value": path_resolution (regexpAbsoluteUrl[13]) },
                "search"      : { "value": regexpAbsoluteUrl[14] },
                "hash"        : { "value": regexpAbsoluteUrl[15] },
                "origin"      : { "value": (regexpAbsoluteUrl[1] + regexpAbsoluteUrl[3] + regexpAbsoluteUrl[8]) }
            });
            Object.defineProperties (this, {
                "href"        : { "value": (regexpAbsoluteUrl[1] + regexpAbsoluteUrl[2] + this.pathname + this.search + this.hash) },
                "searchParams": { "value": (new URLSearchParams (this.search.substring(1).split("&"))) }
            });
            Object.defineProperty (this, "toString", { "value": (function () { return (this.href); }) });
        } else {
            let regexpRelativeUrl = (/^(|\.?\.?\/[^?#\s]*)(|\?[^#\s]*)(|#.*)$/).exec (urlString);
            if (baseString && regexpRelativeUrl) {
                let baseUrlObject = new URL (baseString);
                Object.defineProperties (this, {
                    "protocol"    : { "value": baseUrlObject.protocol },
                    "username"    : { "value": baseUrlObject.username },
                    "password"    : { "value": baseUrlObject.password },
                    "host"        : { "value": baseUrlObject.host },
                    "hostname"    : { "value": baseUrlObject.hostname },
                    "port"        : { "value": baseUrlObject.port },
                    "search"      : { "value": ((regexpRelativeUrl[2]) ? regexpRelativeUrl[2] : baseUrlObject.search) },
                    "hash"        : { "value": ((regexpRelativeUrl[3]) ? regexpRelativeUrl[3] : baseUrlObject.hash) },
                    "origin"      : { "value": baseUrlObject.origin }
                });
                let pathname;
                if (regexpRelativeUrl[1].startsWith ("/")) {
                    pathname = path_resolution (regexpRelativeUrl[1]);
                } else if (baseUrlObject.pathname.endsWith ("/")) {
                    pathname = path_resolution (baseUrlObject.pathname + regexpRelativeUrl[1]);
                } else {
                    pathname = path_resolution (baseUrlObject.pathname + "/../" + regexpRelativeUrl[1]);
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
}