if (simpleglobalvariable != "simple value") {
    throw new ReferenceError ("'simpleglobalvalue' does not store the expected value!");
}

if (notsosimpleglobalvariable != "simple value actually") {
    throw new ReferenceError ("'notsosimpleglobalvariable' does not store the expected value!");
}

if (commandlineglobalvariable != "command-line value") {
    throw new ReferenceError ("'commandlineglobalvariable' does not store the expected value!");
}

var counter = 0;
var xhr = null;

function getSupportedUrls (returnValue) {
    returnValue ([/^http:\/\/localhost(|:\d+)\//]);
}

function getObjectFromUrl (returnValue, url) {
    setTimeout (function () {
        returnValue ({
            "className": ("URLtest_" + Date.now() + "_" + (counter++)),
            "id"       : url
        });
    }, 1);
}

function getPropertiesFromObject (returnValue, className, id) {
    var url, timeoutConfig, abortConfig, asyncConfig, postData;

    if (! (xhr && (counter % 32))) {
        xhr = new XMLHttpRequest ();
    }

    xhr.upload.onloadstart = printProgress (xhr, className, id, "upload-loadstart");
    xhr.upload.onprogress = printProgress (xhr, className, id, "upload-progress");
    xhr.upload.onabort = printProgress (xhr, className, id, "upload-abort");
    xhr.upload.onerror = printProgress (xhr, className, id, "upload-error");
    xhr.upload.onload = printProgress (xhr, className, id, "upload-load");
    xhr.upload.ontimeout = printProgress (xhr, className, id, "upload-timeout");
    xhr.upload.onloadend = printProgress (xhr, className, id, "upload-loadend");

    xhr.onloadstart = printProgress (xhr, className, id, "loadstart");
    xhr.onprogress = printProgress (xhr, className, id, "progress");
    xhr.onabort = printProgress (xhr, className, id, "abort");
    xhr.onerror = printProgress (xhr, className, id, "error");
    xhr.onload = printProgress (xhr, className, id, "load");
    xhr.ontimeout = printProgress (xhr, className, id, "timeout");

    xhr.onreadystatechange = (function (xhrObj, _className, _id) {
        return (function () {
            console.log ("XHR-EVENT-READYSTATECHANGE");
            console.log ("|    className='" + _className + "'");
            console.log ("|    id='" + _id + "'");
            console.log ("|    XHR.readyState=" + xhrObj.readyState);
        });
    }) (xhr, className, id);

    xhr.onloadend = (function (xhrObj, _className, _id) {
        return (function (eventData) {
            (printProgress (xhrObj, _className, _id, "loadend")) (eventData);
            returnValue (xhrObj.response);
        });
    }) (xhr, className, id);

    url = id;

    timeoutConfig = (/^[^\?#]+\/timeout\/(\d+)(|\D.*)$/i).exec (url);
    if (timeoutConfig) {
        xhr.timeout = timeoutConfig[1];
    }

    abortConfig = (/^[^\?#]+\/abort\/(\d+)(|\D.*)$/i).exec (url);
    if (abortConfig) {
        setTimeout ((function (xhrObj) {
            return (function () {
                xhrObj.abort ();
            });
        }) (xhr), abortConfig[1]);
    }

    if (! (timeoutConfig || abortConfig)) {
        xhr.timeout = 120000;
    }

    asyncConfig = ((/^[^\?#]+\/sync\//i).exec (url) === null);

    postData = (/^[^\?#]+\?(([^#&=]+=[^#&=]*&)*([^#&=]+=[^#&=]*)?)(|#.*)$/).exec (url);
    if (postData) {
        postData = postData[1].split("&").filter(function (currentValue) {
            // Filter "expect" GET parameters
            // @disable-check M126
            return (currentValue.substring(0, 7) == "expect=" || currentValue.substring(0, 9) == "expect[]=" || (currentValue.substring(0, 6) == "expect" && currentValue.substring(6, 13).toUpperCase() == "%5B%5D="));
        }).map(function (currentValue) {
            // Extract and decode their values
            return (decodeURIComponent (currentValue.split("=").slice(1).join("=")));
        }).join("&");
    }

    if (postData) {
        xhr.open ("POST", url, asyncConfig);
        xhr.setRequestHeader ("Content-Type", "application/x-www-form-urlencoded");
    } else {
        xhr.open ("GET", url, asyncConfig);
    }
    if (asyncConfig) {
        xhr.send (postData);
    } else {
        try {
            xhr.send (postData);
            xhr.onloadend ({
                "lengthComputable": false,
                "loaded": 0,
                "total": 0
            });
        } catch (e) {
            console.log ("XHR-EXCEPTION");
            console.log ("|    className='" + className + "'");
            console.log ("|    id='" + id + "'");
            console.log ("|    exception=" + e);
            returnValue (null);
        }
    }
}

function printProgress (xhrObj, className, id, eventName) {
    return (function (eventData) {
        console.log ("XHR-EVENT-" + eventName.toUpperCase());
        console.log ("|    className='" + className + "'");
        console.log ("|    id='" + id + "'");
        console.log ("|    lengthComputable=" + eventData.lengthComputable);
        console.log ("|    loaded=" + eventData.loaded);
        console.log ("|    total=" + eventData.total);
        console.log ("|    XHR.status=" + xhrObj.status);
        console.log ("|    XHR.statusText='" + xhrObj.statusText + "'");
    });
}
