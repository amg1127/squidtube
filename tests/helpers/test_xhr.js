if (simpleglobalvariable != "simple value") {
    throw new ReferenceError ("'simpleglobalvalue' does not store the expected value!");
}

var counter = 0;

function getSupportedUrls (returnValue) {
    returnValue (["*"]);
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
    var xhr = new XMLHttpRequest ();

    var printProgress = function (eventName) {
        return (function (eventData) {
            console.log ("XHR-EVENT-" + eventName.toUpperCase());
            console.log ("|    className='" + className + "'");
            console.log ("|    id='" + id + "'");
            console.log ("|    lengthComputable=" + eventData.lengthComputable);
            console.log ("|    loaded=" + eventData.loaded);
            console.log ("|    total=" + eventData.total);
            console.log ("|    XHR.status=" + xhr.status);
            console.log ("|    XHR.statusText='" + xhr.statusText + "'");
        });
    };

    xhr.upload.onloadstart = printProgress ("upload-loadstart");
    xhr.upload.onprogress = printProgress ("upload-progress");
    xhr.upload.onabort = printProgress ("upload-abort");
    xhr.upload.onerror = printProgress ("upload-error");
    xhr.upload.onload = printProgress ("upload-load");
    xhr.upload.ontimeout = printProgress ("upload-timeout");
    xhr.upload.onloadend = printProgress ("upload-loadend");

    xhr.onloadstart = printProgress ("loadstart");
    xhr.onprogress = printProgress ("progress");
    xhr.onabort = printProgress ("abort");
    xhr.onerror = printProgress ("error");
    xhr.onload = printProgress ("load");
    xhr.ontimeout = printProgress ("timeout");
    xhr.onloadend = (function (eventData) {
        (printProgress ("loadend")) (eventData);
        returnValue (xhr.response);
    });

    xhr.onreadystatechange = function () {
        console.log ("XHR-EVENT-READYSTATECHANGE");
        console.log ("|    className='" + className + "'");
        console.log ("|    id='" + id + "'");
        console.log ("|    readyState=" + xhr.readyState);
    }

    var url = id;

    var timeoutConfig = (/^[^\?#]+\/timeout\/(\d+)(|\D.*)$/i).exec (url);
    if (timeoutConfig) {
        xhr.timeout = timeoutConfig[1];
    }

    var abortConfig = (/^[^\?#]+\/abort\/(\d+)(|\D.*)$/i).exec (url);
    if (abortConfig) {
        setTimeout (function () {
            xhr.abort ();
        }, abortConfig[1]);
    }

    if (! (timeoutConfig || abortConfig)) {
        xhr.timeout = 120000;
    }

    var asyncConfig = ((/^[^\?#]+\/sync\//i).exec (url) === null);

    var postData = (/^[^\?#]+\?(([^#&=]+=[^#&=]*&)*([^#&=]+=[^#&=]*)?)(|#.*)$/).exec (url);
    if (postData) {
        postData = postData[1].split("&").filter(function (currentValue) {
            // Filter "expect" GET parameters
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
