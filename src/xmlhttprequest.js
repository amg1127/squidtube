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

// I had to reimplement XMLHttpRequest because of QTBUG-67337 ...
// https://xhr.spec.whatwg.org/
// http://tobyho.com/2010/11/22/javascript-constructors-and/

// @disable-check M127
(function (sendCallback, abortCallback, setTimeoutCallback, getResponseBufferCallback, validStatusCodes) {

    function DOMException (message, name) {
        Object.defineProperties (this, {
            "name": { "value": name },
            "message": { "value": message },
            "code": { "get": (function () {
                // https://heycam.github.io/webidl/#idl-DOMException-error-names
                var dictionary = {
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
        });
        if (Error.captureStackTrace) {
            Error.captureStackTrace (this, DOMException);
        }
    }
    DOMException.prototype = new Error ("DOMException");
    DOMException.prototype.constructor = DOMException;

    // This is so crazy: https://stackoverflow.com/a/249937
    var contentTypeParser = /\s*,\s*([\w-]+\/[\w-]+)((\s*;\s*[\w-]+\s*=\s*([\w-]+|"([^"\\]|\\.)*"))*)\s*/g;
    var parametersParser = /\s*;\s*([\w-]+)\s*=\s*(([\w-]+)|"(([^"\\]|\\.)*)")\s*/g;
    function parseContentType (contentTypeString) {
        var returnValue = {};
        contentTypeParser.lastIndex = 0;
        var contentTypeCaptures = null;
        var parametersCaptures = null;
        var contentTypeStringWithComma = "," + contentTypeString;
        var lastMatchedIndex = 0;
        // @disable-check M19
        while (contentTypeCaptures = contentTypeParser.exec (contentTypeStringWithComma)) {
            lastMatchedIndex = contentTypeParser.lastIndex;
            returnValue["mimeType"] = contentTypeCaptures[1];
            returnValue["charset"] = null;
            returnValue["quotedCharset"] = null;
            if (contentTypeCaptures[2]) {
                parametersParser.lastIndex = 0;
                // @disable-check M19
                while (parametersCaptures = parametersParser.exec (contentTypeCaptures[2])) {
                    // @disable-check M126
                    if (parametersCaptures[1].toLowerCase() == "charset") {
                        if (parametersCaptures[4]) {
                            returnValue["quotedCharset"] = parametersCaptures[4];
                            returnValue["charset"] = returnValue["quotedCharset"].replace(/\\(.)/g, '$1');
                        } else {
                            returnValue["quotedCharset"] = returnValue["charset"] = parametersCaptures[3];
                        }
                    }
                }
            }
        }
        if (/^\s*$/.test (contentTypeStringWithComma.substring(lastMatchedIndex, contentTypeStringWithComma.length))) {
            return (returnValue);
        } else {
            return (null);
        }
    }

    var writableValue = { "writable": true };

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

        var status_UNSENT = validStatusCodes["UNSENT"];
        var status_OPENED = validStatusCodes["OPENED"];
        var status_HEADERS_RECEIVED = validStatusCodes["HEADERS_RECEIVED"];
        var status_LOADING = validStatusCodes["LOADING"];
        var status_DONE = validStatusCodes["DONE"];

        var XMLHttpRequestPrivate = {
            "state"                 : status_UNSENT,
            "requestId"             : null,
            "requestMethod"         : "",
            "requestUrl"            : "",
            "requestHeaders"        : [],
            "requestTimeout"        : 0,
            "requestUsername"       : null,
            "requestPassword"       : null,
            "responseType"          : "",
            "responseObject"        : {
                "type": "null",
                "value": null
            },
            "requestBuffer"         : null,
            "responseBuffer"        : null,
            "synchronousFlag"       : false,
            "uploadCompleteFlag"    : false,
            "timedOutFlag"          : false,
            "sendFlag"              : false,
            "abortedFlag"           : false,
            "withCredentialsFlag"   : false
        };

        var getArrayBufferResponse = function () {
            return (XMLHttpRequestPrivate["responseBuffer"].slice(0));
        };

        var updateArrayBufferResponse = function () {
            if (XMLHttpRequestPrivate["requestId"] && XMLHttpRequestPrivate["sendFlag"]) {
                XMLHttpRequestPrivate["responseBuffer"] = getResponseBufferCallback (XMLHttpRequestPrivate["requestId"]);
            }
        };

        var getFinalMimeType = function () {
            var finalMimeType = "text/xml";
            var finalCharset = null;
            var finalQuotedCharset = null;
            if (XMLHttpRequestPrivate["responseHeaders"]) {
                if (XMLHttpRequestPrivate["responseHeaders"]["content-type"]) {
                    var mimeTypeCandidates = XMLHttpRequestPrivate["responseHeaders"]["content-type"];
                    var mimeTypeCandidatesLength = mimeTypeCandidates.length;
                    var mimeTypeCandidatesPos = 0;
                    var mimeParsed;
                    while (mimeTypeCandidatesPos < mimeTypeCandidatesLength) {
                        mimeParsed = parseContentType (mimeTypeCandidates[mimeTypeCandidatesPos]);
                        if (mimeParsed) {
                            if (mimeParsed["mimeType"]) {
                                finalMimeType = mimeParsed["mimeType"];
                                finalCharset = mimeParsed["charset"];
                                finalQuotedCharset = mimeParsed["quotedCharset"];
                            }
                        }
                        mimeTypeCandidatesPos++;
                    }
                }
            }
            if (XMLHttpRequestPrivate["overrideMimeType"]) {
                finalMimeType = XMLHttpRequestPrivate["overrideMimeType"];
            }
            if (XMLHttpRequestPrivate["overrideCharset"]) {
                finalCharset = XMLHttpRequestPrivate["overrideCharset"];
                finalQuotedCharset = XMLHttpRequestPrivate["overrideQuotedCharset"];
            }
            if (finalMimeType) {
                finalMimeType = finalMimeType.toLowerCase ();
            }
            if (finalCharset) {
                finalCharset = finalCharset.toLowerCase ();
            }
            if (finalQuotedCharset) {
                finalQuotedCharset = finalQuotedCharset.toLowerCase ();
            }
            return ({
                "mimeType"     : finalMimeType,
                "charset"      : finalCharset,
                "quotedCharset": finalQuotedCharset
            });
        }

        var isXMLMimeType = function (mimeType) {
            // @disable-check M126
            if (mimeType == "text/xml" || mimeType == "application/xml") {
                return (true);
                // @disable-check M126
            } else if (mimeType.substring (mimeType.length - 4, mimeType.length) == "+xml") {
                return (true);
            } else {
                return (false);
            }
        }

        var getBlobResponse = function () {
            var finalMimeType = getFinalMimeType ();
            updateArrayBufferResponse ();
            if (XMLHttpRequestPrivate["responseBlob"]) {
                // @disable-check M126
                if (XMLHttpRequestPrivate["responseBlob"].size == XMLHttpRequestPrivate["responseBuffer"].byteLength) {
                    var currentMimeType = parseContentType (XMLHttpRequestPrivate["responseBlob"].type);
                    if (currentMimeType) {
                        // @disable-check M126
                        if (currentMimeType["mimeType"] == finalMimeType["mimeType"] && currentMimeType["charset"] == finalMimeType["charset"]) {
                            return (XMLHttpRequestPrivate["responseBlob"]);
                        }
                    }
                }
            }
            var data = new Blob ([XMLHttpRequestPrivate["responseBuffer"]], { "type": (finalMimeType["mimeType"] + ((finalMimeType["quotedCharset"]) ? (" ; charset=\"" + finalMimeType["quotedCharset"] + "\"") : "")) });
            XMLHttpRequestPrivate["responseBlob"] = data;
            XMLHttpRequestPrivate["responseObject"]["type"] = "blob";
            XMLHttpRequestPrivate["responseObject"]["value"] = data;
            return (data);
        }

        var getDocumentResponse = function () {
            throw new DOMException ("This XMLHttpRequest implementation can not return a document response!", "NotFoundError");
        }

        // https://en.wikipedia.org/wiki/Byte_order_mark#Byte_order_marks_by_encoding
        var bomMasks = {
            "utf-32": [
                [ 0x00, 0x00, 0xFE, 0xFF ],
                [ 0xFF, 0xFE, 0x00, 0x00 ]
            ], "utf-16": [
                [ 0xFE, 0xFF ],
                [ 0xFF, 0xFE ]
            ], "utf-8": [
                [ 0xEF, 0xBB, 0xBF ],
            ], "utf7-empty": [
                [ 0x2B, 0x2F, 0x76, 0x38, 0x2D ]
            ], "utf-7": [
                [ 0x2B, 0x2F, 0x76, 0x38 ],
                [ 0x2B, 0x2F, 0x76, 0x39 ],
                [ 0x2B, 0x2F, 0x76, 0x2B ],
                [ 0x2B, 0x2F, 0x76, 0x2F ]
            ], "utf-1": [
                [ 0xF7, 0x64, 0x4C ]
            ], "utf-ebcdic": [
                [ 0xDD, 0x73, 0x66, 0x73 ]
            ], "scsu": [
                [ 0x0E, 0xFE, 0xFF ]
            ], "bocu-1": [
                [ 0xFB, 0xEE, 0x28 ]
            ], "gb-18030": [
                [ 0x84, 0x31, 0x95, 0x33 ]
            ]
        };
        var bomMasksKeys = Object.keys (bomMasks);
        var bomMasksKeysLength = bomMasksKeys.length;
        var xmlHeaderParser = /^<\?xml(|.*(\s+encoding=['"]?([^'"\?\s>]+)['"]?(|\s+.*)))\?>/m;
        var getTextResponse = function () {
            updateArrayBufferResponse ();
            if (XMLHttpRequestPrivate["responseText"] && XMLHttpRequestPrivate["responseTextLength"]) {
                // @disable-check M126
                if (XMLHttpRequestPrivate["responseTextLength"] == XMLHttpRequestPrivate["responseBuffer"].byteLength) {
                    return (XMLHttpRequestPrivate["responseText"]);
                }
            }
            var data = "";
            if (XMLHttpRequestPrivate["responseBuffer"].byteLength) {
                var buffer = new Uint8Array (XMLHttpRequestPrivate["responseBuffer"]);
                var bufferLen = buffer.length;
                var finalMimeType = getFinalMimeType ();
                var charset = finalMimeType["charset"];
                if ((! XMLHttpRequestPrivate["responseType"]) && (! charset) && isXMLMimeType (finalMimeType["mimeType"])) {
                    // ... then use the rules set forth in the XML specifications to determine the encoding
                    // http://xmlwriter.net/xml_guide/xml_declaration.shtml
                    var xmlBufferStart = 0;
                    // https://en.wikipedia.org/wiki/Byte_order_mark#Byte_order_marks_by_encoding
                    var bomMasksKeysPos;
                    var bomMaskPatterns, bomMaskPatternsLength, bomMaskPatternsPos;
                    var bomMaskPattern, bomMaskPatternLength, bomMaskPatternPos;
                    for (bomMasksKeysPos = 0; bomMasksKeysPos < bomMasksKeysLength; bomMasksKeysPos++) {
                        bomMaskPatterns = bomMasks[bomMasksKeys[bomMasksKeysPos]];
                        bomMaskPatternsLength = bomMaskPatterns.length;
                        for (bomMaskPatternsPos = 0; bomMaskPatternsPos < bomMaskPatternsLength; bomMaskPatternsPos++) {
                            bomMaskPattern = bomMaskPatterns[bomMaskPatternsPos];
                            bomMaskPatternLength = bomMaskPattern.length;
                            for (bomMaskPatternPos = 0; bomMaskPatternPos < bomMaskPatternLength && bomMaskPatternPos < bufferLen; bomMaskPatternPos++) {
                                // @disable-check M126
                                if (bomMaskPattern[bomMaskPatternPos] != buffer[bomMaskPatternPos]) {
                                    break;
                                }
                            }
                            if (bomMaskPatternPos >= bomMaskPatternLength) {
                                charset = bomMasksKeys[bomMasksKeysPos];
                                xmlBufferStart = bomMaskPatternLength;
                                break;
                            }
                        }
                        if (charset) {
                            break;
                        }
                    }
                    var xmlHeader = "";
                    for (; xmlBufferStart < bufferLen && xmlBufferStart < 128; xmlBufferStart++) {
                        xmlHeader += String.fromCharCode (buffer[xmlBufferStart]);
                    }
                    var xmlHeaderCaptures = xmlHeaderParser.exec (xmlHeader);
                    if (xmlHeaderCaptures) {
                        if (xmlHeaderCaptures[3]) {
                            charset = xmlHeaderCaptures[3];
                        }
                    }
                }
                if (! charset) {
                    charset = 'utf-8';
                }
                // @disable-check M126
                if (charset.toLowerCase() == 'utf7-empty') {
                    data = "";
                } else {
                    data = textDecode (XMLHttpRequestPrivate["responseBuffer"], charset);
                }
            }
            XMLHttpRequestPrivate["responseTextLength"] = XMLHttpRequestPrivate["responseBuffer"].byteLength;
            XMLHttpRequestPrivate["responseText"] = data;
            XMLHttpRequestPrivate["responseObject"]["type"] = "text";
            XMLHttpRequestPrivate["responseObject"]["value"] = data;
            return (data);
        }

        var getJsonResponse = function () {
            updateArrayBufferResponse ();
            if (XMLHttpRequestPrivate["responseJson"] && XMLHttpRequestPrivate["responseJsonLength"]) {
                // @disable-check M126
                if (XMLHttpRequestPrivate["responseJsonLength"] == XMLHttpRequestPrivate["responseBuffer"].byteLength) {
                    return (XMLHttpRequestPrivate["responseJson"]);
                }
            }
            XMLHttpRequestPrivate["responseJsonLength"] = XMLHttpRequestPrivate["responseBuffer"].byteLength;
            var strData = textDecode (XMLHttpRequestPrivate["responseBuffer"], "utf-8");
            var jsonData;
            try {
                jsonData = JSON.parse (strData);
            } catch (e) {
                console.warn ("An exception was thrown while converting data to JSON format: " + e.message);
                jsonData = null;
            }
            XMLHttpRequestPrivate["responseJson"] = jsonData;
            XMLHttpRequestPrivate["responseObject"]["type"] = "json";
            XMLHttpRequestPrivate["responseObject"]["value"] = jsonData;
            return (jsonData);
        };

        var abortMethod = function () {
            if (XMLHttpRequestPrivate["requestId"]) {
                abortCallback (XMLHttpRequestPrivate["requestId"]);
            }
            // @disable-check M126
            if (XMLHttpRequestPrivate["state"] == status_DONE) {
                XMLHttpRequestPrivate["state"] = status_UNSENT;
                XMLHttpRequestPrivate["responseObject"]["type"] = "failure";
                XMLHttpRequestPrivate["responseObject"]["value"] = "NetworkError";
            }
        };

        var forbiddenHeaderNames = [
            'Accept-Charset',
            'Accept-Encoding',
            'Access-Control-Request-Headers',
            'Access-Control-Request-Method',
            'Connection',
            'Content-Length',
            'Cookie',
            'Cookie2',
            'Date',
            'DNT',
            'Expect',
            'Host',
            'Keep-Alive',
            'Origin',
            'Referer',
            'TE',
            'Trailer',
            'Transfer-Encoding',
            'Upgrade',
            'Via'
        ];
        var forbiddenHeaderNamesLength = forbiddenHeaderNames.length;
        var forbiddenHeaderPrefixes = [
            'Proxy-',
            'Sec-'
        ];
        var forbiddenHeaderPrefixesLength = forbiddenHeaderPrefixes.length;

        var getPrivateData1 = function (key) {
            return (XMLHttpRequestPrivate[key]);
        };
        var setPrivateData1 = function (key, value) {
            XMLHttpRequestPrivate[key] = value;
        };
        var getPrivateData2 = function (key, subkey) {
            return (XMLHttpRequestPrivate[key][subkey]);
        };
        var setPrivateData2 = function (key, subkey, value) {
            XMLHttpRequestPrivate[key][subkey] = value;
        };

        Object.defineProperties (this, {
            // event handlers
            "onreadystatechange": writableValue,

            // states
            "UNSENT"               : { "value": status_UNSENT },
            "OPENED"               : { "value": status_OPENED },
            "HEADERS_RECEIVED"     : { "value": status_HEADERS_RECEIVED },
            "LOADING"              : { "value": status_LOADING },
            "DONE"                 : { "value": status_DONE },
            "readyState"           : { "get": (function () {
                return (XMLHttpRequestPrivate["state"]);
            })},

            // request
            "open"                 : { "value": (function (method, url, async, username, password) {
                if (! (/^\w+$/.test (method))) {
                    throw new DOMException ("Invalid XMLHttpRequest method '" + method + "'!", "SyntaxError");
                }
                var methodUpper = method.toUpperCase();
                // @disable-check M126
                if (methodUpper == 'CONNECT' || methodUpper == 'TRACE' || methodUpper == 'TRACK') {
                    throw new DOMException ("Forbidden XMLHttpRequest method '" + method + "'!", "SecurityError");
                }
                // @disable-check M126
                if (methodUpper == 'DELETE' || methodUpper == 'GET' || methodUpper == 'HEAD' ||
                    // @disable-check M126
                    methodUpper == 'OPTIONS' || methodUpper == 'POST' || methodUpper == 'PUT') {
                    method = methodUpper;
                }
                if (arguments.length < 3) {
                    async = true;
                    username = null;
                    password = null;
                }
                abortMethod ();
                XMLHttpRequestPrivate["sendFlag"] = false;
                XMLHttpRequestPrivate["requestMethod"] = method;
                XMLHttpRequestPrivate["requestUrl"] = url;
                if (! async) {
                    XMLHttpRequestPrivate["synchronousFlag"] = true;
                } else {
                    XMLHttpRequestPrivate["synchronousFlag"] = false;
                }
                XMLHttpRequestPrivate["requestHeaders"] = [];
                XMLHttpRequestPrivate["requestUsername"] = username;
                XMLHttpRequestPrivate["requestPassword"] = password;
                XMLHttpRequestPrivate["responseObject"]["type"] = "failure";
                XMLHttpRequestPrivate["responseObject"]["value"] = "NetworkError";
                XMLHttpRequestPrivate["requestBuffer"] = null;
                XMLHttpRequestPrivate["responseBuffer"] = null;
                XMLHttpRequestPrivate["responseBlob"] = null;
                XMLHttpRequestPrivate["responseText"] = null;
                XMLHttpRequestPrivate["responseJson"] = null;
                // @disable-check M126
                if (XMLHttpRequestPrivate["state"] != status_OPENED) {
                    XMLHttpRequestPrivate["state"] = status_OPENED;
                    if (this.onreadystatechange) {
                        this.onreadystatechange ();
                    }
                }
            })},
            "setRequestHeader"     : { "value": (function (name, value) {
                // @disable-check M126
                if (XMLHttpRequestPrivate["state"] != status_OPENED) {
                    throw new DOMException ("XMLHttpRequest state is " + XMLHttpRequestPrivate["state"], "InvalidStateError");
                }
                if (XMLHttpRequestPrivate["sendFlag"]) {
                    throw new DOMException ("XMLHttpRequest sendFlag is set", "InvalidStateError");
                }
                var valueNormalized = value.trim ();
                if (! ((/^[\w-]+$/.test (name)) && (/^[^\x00\x0a\x0d]*$/.test (valueNormalized)))) {
                    throw new DOMException ("Either header name or value is invalid", "SyntaxError");
                }
                var nameLower = name.toLowerCase ();
                var headerPos;
                for (headerPos = 0; headerPos < forbiddenHeaderNamesLength; headerPos++) {
                    // @disable-check M126
                    if (forbiddenHeaderNames[headerPos].toLowerCase() == nameLower) {
                        return;
                    }
                }
                for (headerPos = 0; headerPos < forbiddenHeaderPrefixesLength; headerPos++) {
                    // @disable-check M126
                    if (forbiddenHeaderPrefixes[headerPos].toLowerCase() == nameLower.substring(0, forbiddenHeaderPrefixes[headerPos].length)) {
                        return;
                    }
                }
                if (XMLHttpRequestPrivate["requestHeaders"][nameLower]) {
                    XMLHttpRequestPrivate["requestHeaders"][nameLower].push (valueNormalized);
                } else {
                    XMLHttpRequestPrivate["requestHeaders"][nameLower] = [valueNormalized];
                }
            })},
            "timeout"              : { "get": (function () {
                return (XMLHttpRequestPrivate["requestTimeout"]);
            }), "set": (function (value) {
                XMLHttpRequestPrivate["requestTimeout"] = value;
                if (XMLHttpRequestPrivate["requestTimeout"] <= 0) {
                    XMLHttpRequestPrivate["requestTimeout"] = 0;
                }
                if (XMLHttpRequestPrivate["requestId"]) {
                    setTimeoutCallback (XMLHttpRequestPrivate["requestId"], XMLHttpRequestPrivate["requestTimeout"]);
                }
            })},
            "withCredentials"      : { "get": (function () {
                return (XMLHttpRequestPrivate["withCredentialsFlag"]);
            }), "set": (function (value) {
                // @disable-check M126
                if (XMLHttpRequestPrivate["state"] != status_UNSENT && XMLHttpRequestPrivate["state"] != status_OPENED) {
                    throw new DOMException ("XMLHttpRequest state is " + XMLHttpRequestPrivate["state"], "InvalidStateError");
                }
                if (XMLHttpRequestPrivate["sendFlag"]) {
                    throw new DOMException ("XMLHttpRequest sendFlag is set", "InvalidStateError");
                }
                XMLHttpRequestPrivate["withCredentialsFlag"] = value;
            })},
            "upload"               : { "value": (new XMLHttpRequestUpload ()) },
            "send"                 : { "value": (function (body) {
                // @disable-check M126
                if (XMLHttpRequestPrivate["state"] != status_OPENED) {
                    throw new DOMException ("XMLHttpRequest state is " + XMLHttpRequestPrivate["state"], "InvalidStateError");
                }
                if (XMLHttpRequestPrivate["sendFlag"]) {
                    throw new DOMException ("XMLHttpRequest sendFlag is set", "InvalidStateError");
                }
                // @disable-check M126
                if (XMLHttpRequestPrivate["requestMethod"] == 'GET' || XMLHttpRequestPrivate["requestMethod"] == 'HEAD') {
                    body = null;
                }
                if (body) {
                    var encoding = null;
                    var mimeType = null;
                    if (! (encoding || mimeType)) {
                        try {
                            if ((body instanceof String) || (typeof body) == "string") {
                                encoding = 'UTF-8';
                                XMLHttpRequestPrivate["requestBuffer"] = textEncode (body, encoding);
                            }
                        } finally {
                        }
                    }
                    if (! (encoding || mimeType)) {
                        try {
                            if (body instanceof Blob) {
                                mimeType = 'application/octet-stream';
                            }
                        } finally {
                        }
                        if (mimeType) {
                            throw new DOMException ("This XMLHttpRequest implementation can not receive a BLOB request body", "NotFoundError");
                        }
                    }
                    if (! (encoding || mimeType)) {
                        try {
                            if (body instanceof ArrayBuffer) {
                                mimeType = 'application/octet-stream';
                                XMLHttpRequestPrivate["requestBuffer"] = body;
                            }
                        } finally {
                        }
                    }
                    if (! (encoding || mimeType)) {
                        try {
                            if (body instanceof Object) {
                                // I believe the caller intends to send JSON data...
                                encoding = 'UTF-8';
                                XMLHttpRequestPrivate["requestBuffer"] = textEncode (JSON.stringify (body), encoding);
                            }
                        } finally {
                        }
                    }
                    if (! (encoding || mimeType)) {
                        if ((typeof body) == "object") {
                            throw new DOMException ("Unmanageable request body type: '" + body.constructor.name + "'!", "SyntaxError");
                        } else {
                            throw new DOMException ("Unmanageable request body type: '" + (typeof body) + "'!", "SyntaxError");
                        }
                    }
                    if (mimeType) {
                        if (! XMLHttpRequestPrivate["requestHeaders"]["content-type"]) {
                            XMLHttpRequestPrivate["requestHeaders"]["content-type"] = [mimeType];
                        }
                    }
                    if (encoding && XMLHttpRequestPrivate["requestHeaders"]["content-type"]) {
                        var contentTypeLen = XMLHttpRequestPrivate["requestHeaders"]["content-type"].length;
                        var contentTypePos, contentTypeValue, specifiedMimeType;
                        for (contentTypePos = 0; contentTypePos < contentTypeLen; contentTypePos++) {
                            contentTypeValue = XMLHttpRequestPrivate["requestHeaders"]["content-type"][contentTypePos];
                            specifiedMimeType = parseContentType (contentTypeValue);
                            if (specifiedMimeType) {
                                // @disable-check M126
                                if (specifiedMimeType["mimeType"] && specifiedMimeType["charset"] != encoding) {
                                    XMLHttpRequestPrivate["requestHeaders"]["content-type"][contentTypePos] = contentTypeValue.replace (/\s*;\s*charset\s*=\s*([\w-]+|"([^"\\]|\\.)*")\s*/gi, '; charset="' + encoding.replace(/["\\]/g, '\\$1').replace(/\$/g, '$$') + '"');
                                }
                            }
                        }
                    }
                } else {
                    XMLHttpRequestPrivate["requestBuffer"] = null;
                }
                XMLHttpRequestPrivate["uploadCompleteFlag"] = false;
                XMLHttpRequestPrivate["timedOutFlag"] = false;
                XMLHttpRequestPrivate["abortedFlag"] = false; // The XMLHttpRequest specification did not rule this...
                XMLHttpRequestPrivate["status"] = 0;          // The XMLHttpRequest specification did not rule this...
                XMLHttpRequestPrivate["statusText"] = '';     // The XMLHttpRequest specification did not rule this...
                XMLHttpRequestPrivate["responseURL"] = '';    // The XMLHttpRequest specification did not rule this...

                if (! body) {
                    XMLHttpRequestPrivate["uploadCompleteFlag"] = true;
                }
                XMLHttpRequestPrivate["sendFlag"] = true;
                // From step 11 onwards, let the C++ stack assume the control
                sendCallback (this, getPrivateData1, setPrivateData1, getPrivateData2, setPrivateData2);
                // @disable-check M126
                if (XMLHttpRequestPrivate["synchronousFlag"] && XMLHttpRequestPrivate["responseObject"]["type"] == "failure") {
                    var exception = XMLHttpRequestPrivate["responseObject"]["value"];
                    var colonPos = exception.indexOf(":");
                    if (colonPos >= 0) {
                        throw new DOMException (exception.substring(colonPos + 1, exception.length).trim(), exception.substring(0, colonPos));
                    }
                }
            })},
            "abort"                : { "value": abortMethod },

            // response
            "responseURL"          : { "get": (function () {
                return (XMLHttpRequestPrivate["responseURL"]);
            })},
            "status"               : { "get": (function () {
                return (XMLHttpRequestPrivate["status"]);
            })},
            "statusText"           : { "get": (function () {
                return (XMLHttpRequestPrivate["statusText"]);
            })},
            "getResponseHeader"    : { "value": (function (name) {
                var lcName = name.toLowerCase();
                if (XMLHttpRequestPrivate["responseHeaders"]) {
                    if (XMLHttpRequestPrivate["responseHeaders"][lcName]) {
                        return (XMLHttpRequestPrivate["responseHeaders"][lcName].join("\x2C\x20"));
                    }
                }
                return (null);
            })},
            "getAllResponseHeaders": { "value": (function () {
                var output = "";
                if (XMLHttpRequestPrivate["responseHeaders"]) {
                    var headers = Object.keys (XMLHttpRequestPrivate["responseHeaders"]);
                    headers.sort ();
                    var headersLength = headers.length;
                    var headersPos = 0;
                    var headerItem, headerItemLength, headerItemPos;
                    while (headersPos < headersLength) {
                        headerItem = XMLHttpRequestPrivate["responseHeaders"][headers[headersPos]];
                        headerItemLength = headerItem.length;
                        headerItemPos = 0;
                        while (headerItemPos < headerItemLength) {
                            output += headers[headersPos] + "\x3A\x20" + headerItem[headerItemPos]  + "\x0D\x0A";
                            headerItemPos++;
                        }
                        headersPos++;
                    }
                }
                return (output);
            })},
            "overrideMimeType"     : { "value": (function (mime) {
                // @disable-check M126
                if (XMLHttpRequestPrivate["state"] == status_LOADING || XMLHttpRequestPrivate["state"] == status_DONE) {
                    throw new DOMException ("XMLHttpRequest state is " + XMLHttpRequestPrivate["state"], "InvalidStateError");
                } else {
                    XMLHttpRequestPrivate["overrideMimeType"] = "application/octet-stream";
                    var mimeParsed = parseContentType (mime);
                    if (mimeParsed) {
                        if (mimeParsed["mimeType"]) {
                            XMLHttpRequestPrivate["overrideMimeType"] = mimeParsed["mimeType"];
                            if (mimeParsed["charset"]) {
                                XMLHttpRequestPrivate["overrideCharset"] = mimeParsed["charset"];
                            }
                            if (mimeParsed["quotedCharset"]) {
                                XMLHttpRequestPrivate["overrideQuotedCharset"] = mimeParsed["quotedCharset"];
                            }
                        }
                    }
                }
            })},
            "responseType"         : { "get": (function () {
                return (XMLHttpRequestPrivate["responseType"]);
            }), "set": (function (value) {
                // @disable-check M126
                if (value == "document") {
                    // Terminate these steps
                // @disable-check M126
                } else if (XMLHttpRequestPrivate["state"] == status_LOADING || XMLHttpRequestPrivate["state"] == status_DONE) {
                    throw new DOMException ("XMLHttpRequest state is " + XMLHttpRequestPrivate["state"], "InvalidStateError");
                } else {
                    var allowedValues = [
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
            "response"             : { "get": (function () {
                // @disable-check M126
                if (XMLHttpRequestPrivate["responseType"] == "" || XMLHttpRequestPrivate["responseType"] == "text") {
                    // @disable-check M126
                    if (XMLHttpRequestPrivate["state"] != status_LOADING && XMLHttpRequestPrivate["state"] != status_DONE) {
                        return ("");
                    } else {
                        return (getTextResponse ());
                    }
                } else {
                    // @disable-check M126
                    if (XMLHttpRequestPrivate["state"] != status_DONE) {
                        return (null);
                    // @disable-check M126
                    } else if (XMLHttpRequestPrivate["responseObject"]["type"] == "failure") {
                        return (null);
                    } else {
                        // @disable-check M126
                        if (XMLHttpRequestPrivate["responseType"] == "arraybuffer") {
                            return (getArrayBufferResponse ());
                        // @disable-check M126
                        } else if (XMLHttpRequestPrivate["responseType"] == "blob") {
                            return (getBlobResponse ());
                        // @disable-check M126
                        } else if (XMLHttpRequestPrivate["responseType"] == "document") {
                            return (getDocumentResponse ());
                        // @disable-check M126
                        } else if (XMLHttpRequestPrivate["responseType"] == "json") {
                            return (getJsonResponse ());
                        } else {
                            throw new TypeError ("Invalid value '" + XMLHttpRequestPrivate["responseType"] + "' for the property 'responseType'!");
                        }
                    }
                }
            })},
            "responseText"         : { "get": (function () {
                // @disable-check M126
                if (XMLHttpRequestPrivate["responseType"] != "" && XMLHttpRequestPrivate["responseType"] != "text") {
                    throw new DOMException ("XMLHttpRequest responseType is " + XMLHttpRequestPrivate["responseType"], "InvalidStateError");
                // @disable-check M126
                } else if (XMLHttpRequestPrivate["state"] != status_LOADING && XMLHttpRequestPrivate["state"] != status_DONE) {
                    return ("");
                } else {
                    return (getTextResponse ());
                }
            })},
            "responseXML"          : { "get": (function () {
                // @disable-check M126
                if (XMLHttpRequestPrivate["responseType"] != "" && XMLHttpRequestPrivate["responseType"] != "document") {
                    throw new DOMException ("XMLHttpRequest responseType is " + XMLHttpRequestPrivate["responseType"], "InvalidStateError");
                // @disable-check M126
                } else if (XMLHttpRequestPrivate["state"] != status_DONE || XMLHttpRequestPrivate["responseObject"]["type"] != "failure") {
                    return (null);
                } else if (XMLHttpRequestPrivate["responseObject"]["type"] !== "null") {
                    return (XMLHttpRequestPrivate["responseObject"]["value"]);
                } else {
                    return (getDocumentResponse ());
                }
            })}
        });
    };
    XMLHttpRequest.prototype = new XMLHttpRequestEventTarget ();
    XMLHttpRequest.prototype.constructor = XMLHttpRequest;

    return (XMLHttpRequest);
});
