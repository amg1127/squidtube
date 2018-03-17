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

QHash<QString,QString> AppRuntime::helperSources;
QHash<QString,QString> AppRuntime::commonSources;
QMutex AppRuntime::sourcesMutex;

int AppRuntime::dbInstance = 0;
QStringList AppRuntime::dbStartupQueries;
QMutex AppRuntime::dbSettingsMutex;

QDateTime AppRuntime::currentDateTime () {
    static QMutex m (QMutex::Recursive);
    QMutexLocker m_lck (&m);
    QDateTime dateTime (QDateTime::currentDateTime());
    return (dateTime);
}

const QString AppHelper::AppHelperSubDir("helpers");
const QString AppHelper::AppCommonSubDir("common");
const QString AppHelper::AppHelperExtension(".js");

/*
 * This is a Javascript snippet that I can't explain well, despite it works wonderfully.
 * I initially planned to load a QQmlEngine object per helper per concurrent thread.
 * Such design would consume redundant memory. So I developed a jail which confines all
 * helper code within an anonymous function and loaded a QQmlEngine per thread instead.
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
 */

const QString AppHelper::AppHelperCodeHeader (
    "(function () {\n"
        "\"use strict\";\n"
        "/* let global_variable_a = unescape ('foo'); */\n"
        "/* let global_variable_b = unescape ('bar'); */\n"
        "\n"
);

const QString AppHelper::AppHelperCodeFooter (
        "\n"
        "return (function (/* callback, context, method, args */) {\n"
            "return ({\n"
                    "/* This is the list of functions that\n"
                    "    a helper must define in the code */\n"
                    "\"getSupportedUrls\": getSupportedUrls,\n"
                    "\"getObjectFromUrl\": getObjectFromUrl,\n"
                    "\"getPropertiesFromObject\": getPropertiesFromObject\n"
                "} [arguments[2]].apply (undefined, [ (function (callback, context, method) {\n"
                    "return (function (returnedValue) {\n"
                        "callback.call (undefined, context, method, returnedValue);\n"
                    "});\n"
                "}) (arguments[0], arguments[1], arguments[2]) ].concat(arguments[3]))\n"
            ");\n"
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
 */
