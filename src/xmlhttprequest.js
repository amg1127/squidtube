// I have to reimplement XMLHttpRequest because of QTBUG-67337 ...
// https://xhr.spec.whatwg.org/
// http://tobyho.com/2010/11/22/javascript-constructors-and/

(function (sendCallback) {

    function DOMException (message, name) {
        Object.defineProperties (this, {
            "name": { "value": name },
            "message": { "value": message },
            "code": { "get": (function () {
                // https://heycam.github.io/webidl/#idl-DOMException-error-names
                let dictionary = {
                    "IndexSizeError": 1,
                    "DOMStringSizeError": 2,
                    "HierarchyRequestError": 3,
                    "WrongDocumentError": 4,
                    "InvalidCharacterError": 5,
                    "NoDataAllowedError": 6,
                    "NoModificationAllowedError": 7,
                    "NotFoundError": 8,
                    "NotSupportedError": 9,
                    "InUseAttributeError": 10,
                    "InvalidStateError": 11,
                    "SyntaxError": 12,
                    "InvalidModificationError": 13,
                    "NamespaceError": 14,
                    "InvalidAccessError": 15,
                    "ValidationError": 16,
                    "TypeMismatchError": 17,
                    "SecurityError": 18,
                    "NetworkError": 19,
                    "AbortError": 20,
                    "URLMismatchError": 21,
                    "QuotaExceededError": 22,
                    "TimeoutError": 23,
                    "InvalidNodeTypeError": 24,
                    "DataCloneError": 25
                }
                if (dictionary[name]) {
                    return (dictionary[name]);
                } else {
                    return (0);
                }
            })}
    }
    DOMException.prototype = new Error ("DOMException");
    DOMException.prototype.constructor = DOMException;

    let writableValue = { "writable": true };

    function XMLHttpRequestEventTarget () {
        Object.defineProperties (this, {
            "onloadstart": writableValue,
            "onprogress" : writableValue,
            "onabort"    : writableValue,
            "onerror"    : writableValue,
            "onload"     : writableValue,
            "ontimeout"  : writableValue,
            "onloadend"  : writableValue
        });
    }

    function XMLHttpRequestUpload () {
    }
    XMLHttpRequestUpload.prototype = new XMLHttpRequestEventTarget ();
    XMLHttpRequestUpload.prototype.constructor = XMLHttpRequestUpload;

    function XMLHttpRequest () {
        if (! (this instanceof XMLHttpRequest)) {
            return (new XMLHttpRequest());
        }

        let status_UNSENT = 0;
        let status_OPENED = 1;
        let status_HEADERS_RECEIVED = 2;
        let status_LOADING = 3;
        let status_DONE = 4;

        let XMLHttpRequestPrivate = {
            "aborted"       : false,
            "state"         : status_UNSENT,
            "responseType"  : "",
            "async"         : true,
            "responseObject": {
                "type": "null",
                "value": null
            }
        };

        Object.defineProperties (this, {
            // event handlers
            "onreadystatechange": writableValue,

            // states
            "UNSENT"           : { "value": status_UNSENT },
            "OPENED"           : { "value": status_OPENED },
            "HEADERS_RECEIVED" : { "value": status_HEADERS_RECEIVED },
            "LOADING"          : { "value": status_LOADING },
            "DONE"             : { "value": status_DONE },

            // request
            "open"             : { "value": (function (method, url, async, username, password) {
            })},
            "setRequestReader" : { "value": (function (name, value) {
            })},
            "timeout"          : { "value":     0, "writable": true },
            "withCredentials"  : { "value": false, "writable": true },
            "upload"           : { "value": (new XMLHttpRequestUpload ()) },
            "send"             : { "value": (function (body) {
                // sendCallback (object, requestBody, setPrivateData)
                sendCallback (this, body, function (name) {
                    return (XMLHttpRequestPrivate[name]);
                }, function (name, value) {
                        XMLHttpRequestPrivate[name] = value;
                });
            })},
            "abort"            : { "value": (function () {
                XMLHttpRequestPrivate["aborted"] = true;
            })},

            // response
            "responseURL": { "get": (function () {
                return (XMLHttpRequestPrivate["responseURL"]);
            })},
            "status": { "get": (function () {
                return (XMLHttpRequestPrivate["status"]);
            })},
            "statusText": { "get": (function () {
                return (XMLHttpRequestPrivate["statusText"]);
            })},
            "getResponseHeader": { "value": (function (name) {
                let lcName = name.toLowerCase();
                if (XMLHttpRequestPrivate["responseHeaders"]) {
                    if (XMLHttpRequestPrivate["responseHeaders"][lcName]) {
                        return (XMLHttpRequestPrivate["responseHeaders"][lcName]);
                    }
                }
                return (null);
            })},
            "getAllResponseHeaders": { "value": (function () {
                let output = "";
                if (XMLHttpRequestPrivate["responseHeaders"]) {
                    let headers = Object.keys (XMLHttpRequestPrivate["responseHeaders"]);
                    headers.sort ();
                    let headersLength = headers.length;
                    let headersPos = 0;
                    while (headersPos < headersLength) {
                        output += headers[headersPos] + "\x3A\x20" + XMLHttpRequestPrivate["responseHeaders"][headers[headersPos]] + "\x0D\x0A";
                        headersPos++;
                    }
                }
                return (output);
            })},
            "overrideMimeType": { "value": (function (mime) {
                if (XMLHttpRequestPrivate["state"] == status_LOADING || XMLHttpRequestPrivate["state"] == status_DONE) {
                    throw new DOMException ("XMLHttpRequest state is " + XMLHttpRequestPrivate["state"], "InvalidStateError");
                } else {
                    XMLHttpRequestPrivate["overrideMimeType"] = "application/octet-stream";
                    let mimeCaptures = (/^([\w-]+)\/([\w-]+)(|\s*;\s*charset\s*=\s*([\w-]+)\s*)$/).exec (mime);
                    if (mimeCaptures) {
                        XMLHttpRequestPrivate["overrideMimeType"] = mimeCaptures[1] + "/" + mimeCaptures[2];
                        if (mimeCaptures[3] && mimeCaptures[4]) {
                            XMLHttpRequestPrivate["overrideCharset"] = mimeCaptures[4];
                        }
                    }
                }
            })},
            "responseType": { "get": (function () {
                return (XMLHttpRequestPrivate["responseType"]);
            }), "set": (function (value) {
                if (value == "document") {
                    // Terminate these steps
                } else if (XMLHttpRequestPrivate["state"] == status_LOADING || XMLHttpRequestPrivate["state"] == status_DONE) {
                    throw new DOMException ("XMLHttpRequest state is " + XMLHttpRequestPrivate["state"], "InvalidStateError");
                } else {
                    let allowedValues = [
                        "",
                        "arraybuffer",
                        "blob",
                        "json",
                        "text"
                    ];
                    if (allowedValues.indexOf (value) >= 0) {
                        XMLHttpRequestPrivate["responseType"] = value;
                    } else {
                        throw new TypeError ("Invalid value '" + value + "' for the property 'responseType'!");
                    }
                }
            })},
            "response": { "get": (function () {
                if (XMLHttpRequestPrivate["responseType"] == "" || XMLHttpRequestPrivate["responseType"] == "text") {
                    if (XMLHttpRequestPrivate["state"] != status_LOADING && XMLHttpRequestPrivate["state"] != status_DONE) {
                        return ("");
                    } else {
                        return (XMLHttpRequestPrivate["textResponse"]);
                    }
                } else {
                    if (XMLHttpRequestPrivate["state"] != status_DONE) {
                        return (null);
                    } else if (XMLHttpRequestPrivate["responseObject"]["type"] == "failure") {
                        return (null);
                    } else {
                        if (XMLHttpRequestPrivate["responseType"] == "arraybuffer") {
                            // Return the arraybuffer response.

                        } else if (XMLHttpRequestPrivate["responseType"] == "blob") {
                            // Return the blob response.

                        } else if (XMLHttpRequestPrivate["responseType"] == "document") {
                            // Return the document response.
                        } else if (XMLHttpRequestPrivate["responseType"] == "json") {
                            // Return the JSON response.

                        } else {
                            throw new TypeError ("Invalid value '" + XMLHttpRequestPrivate["responseType"] + "' for the property 'responseType'!");
                        }
                    }
                }
            })},
            "responseText": { "get": (function () {
                if (XMLHttpRequestPrivate["responseType"] != "" && XMLHttpRequestPrivate["responseType"] != "text") {
                    throw new DOMException ("XMLHttpRequest responseType is " + XMLHttpRequestPrivate["responseType"], "InvalidStateError");
                } else if (XMLHttpRequestPrivate["state"] != status_LOADING && XMLHttpRequestPrivate["state"] != status_DONE) {
                    return ("");
                } else {
                    return (XMLHttpRequestPrivate["textResponse"]);
                }
            })},
            "responseXML": { "get": (function () {
                if (XMLHttpRequestPrivate["responseType"] != "" && XMLHttpRequestPrivate["responseType"] != "document") {
                    throw new DOMException ("XMLHttpRequest responseType is " + XMLHttpRequestPrivate["responseType"], "InvalidStateError");
                } else if (XMLHttpRequestPrivate["state"] != status_DONE) {
                    return (null);
                } else if (XMLHttpRequestPrivate["responseObject"]["type"] !== "null") {
                    return (XMLHttpRequestPrivate["responseObject"]["value"]);
                } else {
                    throw new DOMException ("This API can not return a document response!", "NotFoundError");
                }
            })}
        });
    };
    XMLHttpRequest.prototype = new XMLHttpRequestEventTarget ();
    XMLHttpRequest.prototype.constructor = XMLHttpRequest;

    return (XMLHttpRequest);
});
