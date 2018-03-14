#include "appruntime.h"

// Default application settings go below

#ifdef QT_NO_DEBUG
QString AppRuntime::loglevel("INFO");
#else
QString AppRuntime::loglevel("DEBUG");
#endif
QString AppRuntime::helpers("");
QString AppRuntime::registryTTL("2592000");
QString AppRuntime::dbDriver("QSQLITE");
QString AppRuntime::dbHost("");
QString AppRuntime::dbPort("0");
QString AppRuntime::dbUser("");
QString AppRuntime::dbPassword("");
QString AppRuntime::dbOptions("");
QString AppRuntime::dbName("");
QString AppRuntime::dbStartupQuery("");

// Default application settings go above

QList<AppHelper> AppRuntime::helperObjects;

QStringList AppRuntime::dbStartupQueries;
QMutex AppRuntime::dbSettingsMutex;
int AppRuntime::dbInstance = 0;

QDateTime AppRuntime::currentDateTime () {
    static QMutex m (QMutex::Recursive);
    QMutexLocker m_lck (&m);
    QDateTime dateTime (QDateTime::currentDateTime());
    return (dateTime);
}

const QString AppHelper::AppHelperExtension(".js");

/*
 * This is a Javascript snippet that I can't explain well, despite it works wonderfully.
 * I initially planned to load a QQmlEngine object per helper per concurrent thread.
 * Such design would consume redundant memory. So I developed a jail which confines all
 * helper code within an anonymous function and loaded a QQmlEngine per thread instead.
 * All helpers will now share a runtime environment and will not conflict with each other, because
 * no objects will be defined in the global scope.
 *
 * The inspiration came from JQuery:
 * https://learn.jquery.com/plugins/basic-plugin-creation/#protecting-the-alias-and-adding-scope
 * http://benalman.com/news/2010/11/immediately-invoked-function-expression/
 *
 * Additionally, the code provides alternatives for data transfers from Javascript to C++.
 *
 * * If the requested data is promptly available by the helper, the function must send the answer
 *   by either (but not both) using the "return" statement or calling the function object received
 *   as its first parameter.
 *
 * * If the requested data is not promptly available by the helper (for example, if the helper needs
 *   to download remote data through XMLHttpRequest() object), the function must transfer the function
 *   object received as its first parameter to the callback which will retrieve the data. Then, such
 *   callback must call the function.
 */

const QString AppHelper::AppHelperCodeHeader (
    "(function () {\n"
        "\"use strict\";\n"
        "/* var global_variable_a = unescape ('foo'); */\n"
        "/* var global_variable_b = unescape ('bar'); */\n"
);

const QString AppHelper::AppHelperCodeFooter (
        "return (function (/* callback, context, method, args */) {\n"
            "try {\n"
                "return ({\n"
                    "\"exception\": false,\n"
                    "\"callback\": arguments[0],\n"
                    "\"context\": arguments[1],\n"
                    "\"method\": arguments[2],\n"
                    "\"returnValue\": {\n"
                        "\"getSupportedUrls\": getSupportedUrls,\n"
                        "\"getObjectFromUrl\": getObjectFromUrl,\n"
                        "\"getPropertiesFromObject\": getPropertiesFromObject\n"
                    "} [arguments[2]].apply (undefined, [ (function (callback, context, method) {\n"
                        "return (function (returnValue) {\n"
                            "callback.call (undefined, context, method, returnValue);\n"
                        "});\n"
                    "}) (arguments[0], arguments[1], arguments[2]) ].concat(arguments[3]))\n"
                "});\n"
            "} catch (e) {\n"
                "return ({\n"
                    "\"exception\": true,\n"
                    "\"context\": arguments[1],\n"
                    "\"method\": arguments[2],\n"
                    "\"returnValue\": e\n"
                "});\n"
            "}\n"
        "});\n"
    "}) ();\n"
);

/*
 * More comments about the javascript snippets:
 *
 * Helpers must implement the following functions:
 *
 * * function getSupportedUrls (callback) { }
 *
 *   Returns an array of String or/and RegExp objects, which will be used by C++ program
 *   to determine what URL's the helper is able to process.
 *   If 'getSupportedUrls' returns fixed strings, wildcard characters will be matched (QRegExp::WildcardUnix)
 *   If 'getSupportedUrls' returns regular expressions, they will be used as are.
 *
 * * function getObjectFromUrl (callback, url) { }
 *
 *   Receives an URL that the helper declared to support. The helper must parse the URL and return an object
 *   with properties 'className' and 'id'. The C++ program then will search the database for cache data about
 *   the object. If there is no cached data, the helper will be asked to retrieve it through the function
 *   'getPropertiesFromObject()'.
 *
 * * function getPropertiesFromObject (callback, className, id) { }
 *
 *   Returns either an object or a JSON string with data associated to an object whose type is <className> and
 *   name is <id>.
 */
