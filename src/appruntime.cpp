#include "appruntime.h"

//////////////////////////////////////////////////////////////////
// Default application settings go below

#ifdef QT_NO_DEBUG
QString AppRuntime::loglevel("INFO");
#else
QString AppRuntime::loglevel("DEBUG");
#endif
QString AppRuntime::helpers("");
QString AppRuntime::positiveTTL("2592000");
QString AppRuntime::negativeTTL("600");
QString AppRuntime::dbDriver("QSQLITE");
QString AppRuntime::dbHost("");
QString AppRuntime::dbPort("0");
QString AppRuntime::dbUser("");
QString AppRuntime::dbPassword("");
QString AppRuntime::dbOptions("");
QString AppRuntime::dbName(":memory:");
QString AppRuntime::dbStartupQuery("");
QString AppRuntime::dbTblPrefix("tblHelper_");

// Default application settings go above
//////////////////////////////////////////////////////////////////

QMutex AppRuntime::helperMemoryCacheMutex;
QList<AppHelperObjectCache*> AppRuntime::helperMemoryCache;
QHash<QString,QString> AppRuntime::helperSourcesByName;
QStringList AppRuntime::helperNames;
QMutex AppRuntime::commonSourcesMutex;
QHash<QString,QString> AppRuntime::commonSources;

int AppRuntime::dbInstance = 1;
QStringList AppRuntime::dbStartupQueries;
QMutex AppRuntime::dbSettingsMutex;

qint64 AppRuntime::positiveTTLint = 0;
qint64 AppRuntime::negativeTTLint = 0;

QDateTime AppRuntime::currentDateTime () {
    static QMutex m (QMutex::Recursive);
    QMutexLocker m_lck (&m);
    QDateTime dateTime (QDateTime::currentDateTime());
    return (dateTime);
}

//////////////////////////////////////////////////////////////////

// Constants used by the memory cache system
// The maximum time in miliseconds for the memory cache mutex to become available
const int AppConstants::AppHelperMutexTimeout = 30000;
// How many objects at most the cache is allowed to store
const int AppConstants::AppHelperCacheMaxSize = 8192;
// How many elements will be dropped from the cache if it has reached the maximum size
const int AppConstants::AppHelperCacheVacuumSize = 2048;

// How long the program will wait after detecting that another thread or process started
// to fetch data from an object whose information the program wanted to get.
// If this timer expires the program will check again if such thread of process has already
// finished its job.
// Unit is milliseconds.
const int AppConstants::AppHelperTimerTimeout = 1000;

// How long the program will wait for such thread or process. If this timer expires,
// The program will by itself fetch data from the object
// Unit is seconds.
const int AppConstants::AppHelperMaxWait = 300;

// Path component where common libraries will be searched for
const QString AppConstants::AppCommonSubDir("common");

// Path component where helpers will be searched for
const QString AppConstants::AppHelperSubDir("helpers");

// File extension of common libraries and helpers
const QString AppConstants::AppHelperExtension(".js");

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
 *
 */

const QString AppConstants::AppHelperCodeHeader (
    "(function () {\n"
        "\"use strict\";\n"
        "/* let global_variable_a = unescape ('foo'); */\n"
        "/* let global_variable_b = unescape ('bar'); */\n"
        "\n"
);

const QString AppConstants::AppHelperCodeFooter (
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
 *
 */

//////////////////////////////////////////////////////////////////

// Memory cache destructors

AppHelperClass::~AppHelperClass () {
    for (QHash<QString,AppHelperObject*>::iterator helperId = this->ids.begin(); helperId != this->ids.end(); helperId++) {
        delete (*helperId);
    }
    this->ids.clear ();
}

AppHelperObjectCache::~AppHelperObjectCache () {
    for (QHash<QString,AppHelperClass*>::iterator helperClass = this->classNames.begin(); helperClass != this->classNames.end(); helperClass++) {
        delete (*helperClass);
    }
    this->classNames.clear ();
}

//////////////////////////////////////////////////////////////////

// Method that forces a deep copy of the object
AppSquidRequest AppSquidRequest::deepCopy () const {
    AppSquidRequest returnValue;
    returnValue.requestUrl = QUrl::fromEncoded (this->requestUrl.toEncoded(QUrl::FullyEncoded), QUrl::StrictMode);
    returnValue.requestProperties << this->requestProperties;
    returnValue.requestMathOperator = this->requestMathOperator;
    returnValue.requestCaseSensivity = this->requestCaseSensivity;
    returnValue.requestPatternSyntax = this->requestPatternSyntax;
    returnValue.requestCriteria << this->requestCriteria;
    returnValue.requestHelperName = QString("%1").arg (this->requestHelperName);
    returnValue.requestHelperId = this->requestHelperId;
    returnValue.objectClassName = QString("%1").arg (this->objectClassName);
    returnValue.objectId = QString("%1").arg (this->objectId);
    return (returnValue);
}
