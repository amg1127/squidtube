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

/*
 * This is a Javascript snippet that I can't explain well, despite it works wonderfully.
 * I initially planned to load a QJSEngine object per helper per concurrent thread.
 * Such design would consume redundant memory. So I developed a jail which confines all
 * helper code within an anonymous function and loaded a QJSEngine per thread instead.
 * All helpers will now share a runtime environment and will not conflict with each other, because
 * no objects can be defined by helpers in the global scope. Only global and C++-defined objects
 * will be available there.
 *
 * The inspiration came from JQuery:
 * https://learn.jquery.com/plugins/basic-plugin-creation/#protecting-the-alias-and-adding-scope
 * http://benalman.com/news/2010/11/immediately-invoked-function-expression/
 *
 * Additionally, the code provides the data transfer path from Javascript to C++ through "callback
 * functions", which are always sent as the first argument to Javascript functions defined by the helper.
 * Helper functions must send their return values by calling the callback functions provided, that is,
 * return values sent through the "return" statement will be ignored by the program. Therefore:
 *
 * * If the requested data is promptly available by the helper, functions just invoke the callback
 *   received as its first parameter.
 *
 * * If the requested data is not promptly available by the helper (for example, if the helper needs
 *   to download remote data through XMLHttpRequest() object), the functions transfer the callback
 *   received as its first parameter to the other callbacks which will receive the data. Then, such
 *   callbacks invoke the function.
 *
 * Important: helpers must not execute statements after calling the supplied callback. If they do, any
 * eventual exception generated will render the C++ environment inconsistent.
 *
 */

// @disable-check M127
(function (/* helperIndex, getPropertiesFromObjectCacheEntryPoint */) {
    // @disable-check M127
    "use strict";

    var getPropertiesFromObjectCache = (function (helperIndex, getPropertiesFromObjectCacheEntryPoint) {
        return (function (/* callback, className, id */) {
            getPropertiesFromObjectCacheEntryPoint (helperIndex, {
                "callback": arguments[0],
                "object"  : {
                    "className": arguments[1],
                    "id": arguments[2]
                }
            });
        });
    }) (arguments[0], arguments[1]);

    return ((function () {
        /* var global_variable_a = decodeURIComponent ('foo'); var global_variable_b = decodeURIComponent ('bar'); */

        // https://stackoverflow.com/q/21726948/7184009

        /* Helper code goes here - this placeholder must not be removed! */

        return (function (/* callback, context, method, args */) {
            return ({
                    /* This is the list of functions that
                        a helper must define in the code */
                    "getSupportedUrls": getSupportedUrls,
                    "getObjectFromUrl": getObjectFromUrl,
                    "getPropertiesFromObject": getPropertiesFromObject
                } [arguments[2]].apply (undefined, [ (function (callback, context, method) {
                    return (function (returnedValue) {
                        callback.call (undefined, context, method, returnedValue);
                    });
                }) (arguments[0], arguments[1], arguments[2]) ].concat(arguments[3]))
            );
        });
    }) ());
});

/*
 * More comments about the javascript snippets:
 *
 * Helpers must implement the following functions:
 *
 * * function getSupportedUrls (callback) { }
 *
 *   Returns an array of String or/and RegExp objects, which will be used by C++ program
 *   to determine what URL's the helper is able to process.
 *   If 'getSupportedUrls' returns fixed strings, they will be interpreted as wildcard patterns (QRegExp::WildcardUnix)
 *   If 'getSupportedUrls' returns regular expressions, they will be used as they are.
 *
 * * function getObjectFromUrl (callback, url) { }
 *
 *   Receives an URL that the helper declared to support. The helper must parse the URL and return an object
 *   (or a JSON object as a string) with properties 'className' and 'id' defined. The C++ program then will search
 *   the database for cached data about the object. If there is no cached data, the helper will be asked to retrieve
 *   it through the function 'getPropertiesFromObject()'.
 *
 * * function getPropertiesFromObject (callback, className, id) { }
 *
 *   Returns either an object or a JSON string with data associated to an object whose type is <className> and
 *   name is <id>.
 *
 * If necessary, the helper can retrieve information from the cache by calling the defined
 * 'getPropertiesFromObjectCache (callback, className, id);' function. As specified for the opposite direction,
 * C++ will send data through the function object provided in 'callback' parameter.
 * Warning: if there is no cached data about the object with specified 'className' and 'id' supplied,
 * C++ will invoke 'getPropertiesFromObject()'. This may result in an endless recursion!
 *
 */
