function getSupportedUrls (callback) {
    return ([
        /^https?:\/\/([^\/]+\.)?youtube\.com(\.\w\w)?/i
    ]);
}

function getObjectFromUrl (callback, url) {
    throw new TypeError ("'getObjectFromUrl(callback, url);' is not implemented yet!");
}

function getPropertiesFromObject (callback, className, id) {
    throw new TypeError ("'getPropertiesFromObject(callback, className, id);' is not implemented yet!");
}
