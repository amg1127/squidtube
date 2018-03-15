function getSupportedUrls (returnValue) {
    returnValue ([
        /^https?:\/\/([^\/]+\.)?youtube\.com(\.\w\w)?/i
    ]);
}

function getObjectFromUrl (returnValue, url) {
    throw new TypeError ("'getObjectFromUrl (returnValue, url);' is not implemented yet!");
}

function getPropertiesFromObject (returnValue, className, id) {
    throw new TypeError ("'getPropertiesFromObject (returnValue, className, id);' is not implemented yet!");
}
