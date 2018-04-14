// A generic JSON resolver object

function GenericJsonResolver (returnValue, className, id, getPropertiesFromObjectCache) {
    // @disable-check M127
    "use strict";

    if (! (this instanceof GenericJsonResolver)) {
        return (new GenericJsonResolver (returnValue, className, id, getPropertiesFromObjectCache));
    }

    var data = null;
    var errorHandler = null;
    var successHandlers = [];
    var xhr = null;
    var step = 1;

    function isObject (obj) {
        return ((typeof obj) == "object" && (obj instanceof Object));
    }

    var isArray = Array.isArray;

    function isFunction (value) {
        return ((typeof value) == "function" || (value instanceof Function));
    }

    function isPropertyOrPosition (value) {
        return ((typeof value) == "string" || (value instanceof String) || (typeof value) == "number");
    }

    function getPropertyRecursive (object, property) {
        var strProperty, dotPos, firstPart, secondPart;
        strProperty = "" + property;
        if (isObject (object)) {
            dotPos = strProperty.indexOf(".");
            if (dotPos >= 0) {
                firstPart = strProperty.substring (0, dotPos);
                secondPart = strProperty.substring (dotPos + 1, strProperty.length);
                return (getPropertyRecursive (object[firstPart], secondPart));
            } else {
                return (object[strProperty]);
            }
        } else {
            return (null);
        }
    }

    function verifyInterval (array, index, adjustIndex) {
        var len = array.length;
        var answer = index;
        if (answer >= 0) {
            if (answer >= len) {
                answer = ((adjustIndex) ? (len - 1) : null);
            }
        } else {
            answer += len;
            if (answer < 0) {
                answer = ((adjustIndex) ? 0 : null);
            }
        }
        return (answer);
    }

    function invokeSuccessHandlers () {
        if (successHandlers.length) {
            (successHandlers.shift()) ();
        } else {
            returnValue.call (this, data);
        }
    }

    function invokeErrorHandler (step, residual) {
        successHandlers = [];
        if (errorHandler && errorHandler.call) {
            if (errorHandler.call (this, step, residual)) {
                return;
            }
        }
        if (successHandlers.length) {
            invokeSuccessHandlers ();
        } else if (! xhr) {
            if (step > 0) {
                console.warn ("Failed to execute drill down step #" + step + " within retrieved data about (className='" + className + "', id='" + id + "'). Residual is '" + JSON.stringify (residual) + "'!");
            }
            returnValue.call (this, null);
        }
    }

    function xmlLoadEndHandler () {
        var xhrStatus = {
            "responseURL"          : xhr.responseURL,
            "status"               : xhr.status,
            "statusText"           : xhr.statusText,
            "getResponseHeader"    : xhr.getResponseHeader,
            "getAllResponseHeaders": xhr.getAllResponseHeaders,
            "responseType"         : xhr.responseType
        };
        if (xhr.status >= 200 && xhr.status < 300) {
            data = xhr.response;
            if (isObject (data) || isArray (data)) {
                console.log ("Data about (className='" + className + "', id='" + id + "') was retrieved successfully.");
                xhr = null;
                invokeSuccessHandlers ();
                return;
            } else {
                console.warn ("Retrieved data about (className='" + className + "', id='" + id + "') is invalid: status='" + xhr.status + ": " + xhr.statusText + "'");
            }
        } else {
            console.warn ("Data retrieval about (className='" + className + "', id='" + id + "') failed: status='" + xhr.status + ": " + xhr.statusText + "'");
        }
        xhr = null;
        invokeErrorHandler (0, xhrStatus);
    }

    /*
     * Defines a callback function that will handle processing errors from the GenericJsonResolver object
     * The callback function will receive two arguments:
     *
     * * step => A number that indicates the processing step that fails.
     *
     * * additionalInfo => The object that is being processed:
     *
     * If GenericJsonResolver fails at the "get" step, it will return status information from the XMLHttpRequest.
     *
     * If GenericJsonResolver fails at either "extract", "test" or "add" step, it will return the JSON object that was being processed at the time of the failure.
     *
     * "remove" step is not designed to generate error.
     *
     */
    Object.defineProperty (this, "setErrorHandler", {
        "value": (function (callback) {
            errorHandler = callback;
        })
    });

    /*
     * Defines the endpoint from which the JSON object will be retrieved and retrieve it.
     *
     */
    Object.defineProperty (this, "get", {
        "value": (function (url, username, password) {
            if (xhr || successHandlers.length) {
                throw new TypeError ("'GenericJsonResolver.prototype.get()' can not be called now!");
            } else {
                step = 1;
                xhr = new XMLHttpRequest();
                xhr.open ("GET", url, true, username, password);
                xhr.timeout = 120000;
                xhr.responseType = "json";
                xhr.onloadend = xmlLoadEndHandler;
                xhr.send ();
            }
            return (this);
        })
    });

    /*
     * Extract a property from an object or an element from an Array
     *
     */
    Object.defineProperty (this, "extract", {
        "value": (function (property) {
            var _step = step++;
            if (isPropertyOrPosition (property)) {
                successHandlers.push (function () {
                    if ((typeof property) == "number") {
                        if (isArray (data)) {
                            var index = verifyInterval (data, property, false);
                            if (index !== null) {
                                if (data[index]) {
                                    data = data[index];
                                    invokeSuccessHandlers ();
                                    return;
                                }
                            }
                        } else if (isObject (data)) {
                            var propertyKeys = Object.keys (data);
                            var index = verifyInterval (propertyKeys, property, false);
                            if (index !== null) {
                                if (data[propertyKeys[index]]) {
                                    data = data[propertyKeys[index]];
                                    invokeSuccessHandlers ();
                                    return;
                                }
                            }
                        }
                    } else if (data[property]) {
                        data = data[property];
                        invokeSuccessHandlers ();
                        return;
                    }
                    invokeErrorHandler (_step, data);
                });
                return (this);
            } else {
                throw new TypeError ("Invalid call to 'GenericJsonResolver.prototype.extract()': argument must be either a string or number!");
            }
        })
    });


    /*
     * Test if all properties of the object matches the specified values
     *
     */
    Object.defineProperty (this, "test", {
        "value": (function (conditions) {
            var _step = step++;
            if (isObject (conditions)) {
                successHandlers.push (function () {
                    var keys = Object.keys (conditions);
                    var keysLength = keys.length;
                    var keyPos;
                    var testValue;
                    var testResult = true;
                    for (keyPos = 0; testResult && keyPos < keysLength; keyPos++) {
                        testValue = conditions[keys[keyPos]];
                        if (testValue) {
                            if (isFunction (testValue)) {
                                testResult = testValue.call (this, getPropertyRecursive (data, keys[keyPos]));
                            } else {
                                // @disable-check M126
                                testResult = (getPropertyRecursive (data, keys[keyPos]) == testValue);
                            }
                        } else {
                            testResult = false;
                        }
                    }
                    if (testResult) {
                        invokeSuccessHandlers ();
                    } else {
                        invokeErrorHandler (_step, data);
                    }
                });
                return (this);
            } else {
                throw new TypeError ("Invalid call to 'GenericJsonResolver.prototype.test()': argument must be an object!");
            }
        })
    });

    /*
     * Removes a property from the object
     *
     */
    Object.defineProperty (this, "remove", {
        "value": (function (property) {
            var _step = step++;
            if (isPropertyOrPosition (property)) {
                successHandlers.push (function () {
                    if (isArray (data)) {
                        data.splice (property, 1);
                    } else if (isObject (data)) {
                        if ((typeof property) == "number") {
                            var propertyKeys = Object.keys (data);
                            var index = verifyInterval (propertyKeys, property, true);
                            if (index !== null) {
                                delete data[propertyKeys[property]];
                            }
                        } else {
                            delete data[property];
                        }
                    }
                    invokeSuccessHandlers ();
                });
                return (this);
            } else {
                throw new TypeError ("Invalid call to 'GenericJsonResolver.prototype.remove()': argument must be either a string or number!");
            }
        })
    });

    /*
     * Adds properties into the object. The values of the properties are resolved by 'getPropertiesFromObjectCache();'
     *
     */
    Object.defineProperty (this, "add", {
        "value": (function (spec) {
            var _step = step++;
            if (isObject (spec)) {
                successHandlers.push (function () {
                    var keys = Object.keys (spec);
                    var keysLength = keys.length;
                    var keyPos;
                    var specData;
                    var specDataIsFunction;
                    var objectClass;
                    var id;
                    var extract;
                    var numAddedProperties = 0;
                    var foundError = false;
                    if (keysLength) {
                        for (keyPos = 0; keyPos < keysLength; keyPos++) {
                            objectClass = null;
                            id = null;
                            extract = null;
                            specData = spec[keys[keyPos]];
                            if (specData) {
                                specDataIsFunction = isFunction (specData);
                                if (specDataIsFunction) {
                                    specData = specData.call (this, data);
                                }
                                if (specData) {
                                    if (specData['objectClass'] && specData['id']) {
                                        objectClass = specData['objectClass'];
                                        if (specDataIsFunction) {
                                            id = specData['id'];
                                        } else {
                                            id = getPropertyRecursive (data, specData['id']);
                                        }
                                        if (specData['extract']) {
                                            extract = specData['extract'];
                                        }
                                    }
                                }
                            }
                            (function (property, _objectClass, _id, extractProperty) {
                                var callbackFunction = (function (returnedValue) {
                                    var propertyComponents = property.split(".");
                                    var dataWalk = data;
                                    var propertyComponentsLength = propertyComponents.length - 1;
                                    var propertyComponentsPos;
                                    var propertyComponentName;
                                    for (propertyComponentsPos = 0; propertyComponentsPos < propertyComponentsLength; propertyComponentsPos++) {
                                        propertyComponentName = propertyComponents[propertyComponentsPos];
                                        if (! dataWalk[propertyComponentName]) {
                                            dataWalk[propertyComponentName] = {};
                                        }
                                        dataWalk = dataWalk[propertyComponentName];
                                    }
                                    if (! returnedValue) {
                                        foundError = true;
                                    }
                                    if (extractProperty) {
                                        if ((dataWalk[propertyComponents[propertyComponentsLength]] = getPropertyRecursive (returnedValue, extractProperty)) === null) {
                                            foundError = true;
                                        }
                                    } else {
                                        dataWalk[propertyComponents[propertyComponentsLength]] = returnedValue;
                                    }
                                    if ((++numAddedProperties) == keysLength) {
                                        if (foundError) {
                                            invokeErrorHandler (_step, data);
                                        } else {
                                            invokeSuccessHandlers ();
                                        }
                                    }
                                });
                                if (_objectClass && _id) {
                                    getPropertiesFromObjectCache (callbackFunction, _objectClass, _id);
                                } else {
                                    callbackFunction (null);
                                }
                            }) (keys[keyPos], objectClass, id, extract);
                        }
                    } else {
                        invokeSuccessHandlers ();
                    }
                });
                return (this);
            } else {
                throw new TypeError ("Invalid call to 'GenericJsonResolver.prototype.add()': argument must be an object!");
            }
        })
    });
}
