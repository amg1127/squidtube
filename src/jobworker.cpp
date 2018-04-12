#include "jobworker.h"

void JobWorker::startRetryTimer () {
    if (this->rngdInitialized) {
        this->retryTimer->start (AppConstants::AppHelperTimerTimeout + ((int) ((((double) qrand()) / ((double) RAND_MAX)) * ((double) AppConstants::AppHelperTimerTimeout))));
    } else {
        this->retryTimer->start (AppConstants::AppHelperTimerTimeout);
    }
}


void JobWorker::tryNextHelper (unsigned int requestId) {
    QMap<unsigned int,AppJobRequest*>::iterator requestIdIterator = this->runningRequests.find (requestId);
    if (requestIdIterator != this->runningRequests.end ()) {
        this->tryNextHelper (requestIdIterator);
    }
}

void JobWorker::tryNextHelper (QMap<unsigned int,AppJobRequest*>::iterator& requestIdIterator) {
    AppJobRequestFromSquid* squidRequest = downcast<AppJobRequestFromSquid*> (*requestIdIterator);
    if (squidRequest != Q_NULLPTR) {
        this->runningRequests.erase (requestIdIterator);
        squidRequest->helper.name.clear();
        squidRequest->helper.id++;
        this->incomingRequests.append (squidRequest);
        this->processIncomingRequest ();
    } else {
        qFatal("Invalid procedure call: 'AppJobRequest*' could not be downcasted to 'AppJobRequestFromSquid*'!");
    }
}

void JobWorker::squidResponseOut (QMap<unsigned int,AppJobRequest*>::iterator& requestIdIterator, AppJobRequestFromSquid* squidRequest, const QString& msg, bool isMatch) {
    if (requestIdIterator != this->runningRequests.end ()) {
        unsigned int requestId (requestIdIterator.key ());
        this->runningRequests.erase (requestIdIterator);
        emit writeAnswerLine (this->requestChannel, QString("[%1#%2] %3").arg(squidRequest->helper.name).arg(requestId).arg(msg), false, isMatch);
    } else {
        emit writeAnswerLine (this->requestChannel, msg, false, isMatch);
    }
    delete (squidRequest);
    if (this->finishRequested && this->runningRequests.isEmpty () && this->incomingRequests.isEmpty ()) {
        qInfo() << QString("No pending jobs now. Finished handler for channel #%1.").arg(this->requestChannel);
        emit finished ();
    }
}

void JobWorker::scriptResponseOut (QMap<unsigned int,AppJobRequest*>::iterator& requestIdIterator, AppJobRequestFromHelper *helperRequest, const QJSValue &returnValue) {
    unsigned int requestId (requestIdIterator.key ());
    this->runningRequests.erase (requestIdIterator);
    JavascriptBridge::warnJsError ((*(this->runtimeEnvironment)), helperRequest->data.callback.call (QJSValueList() << returnValue), QString("[%1#%2] Uncaught exception received while calling the callback received as argument from a previous call to 'getPropertiesFromObjectCache();' (className='%3', id='%4')").arg(helperRequest->helper.name).arg(requestId).arg(helperRequest->object.className).arg(helperRequest->object.id));
    delete (helperRequest);
}

void JobWorker::processSupportedUrls (int helperInstance, const QJSValue& appHelperSupportedUrls) {
    if (helperInstance > 0 && helperInstance <= this->helperInstances.count() && appHelperSupportedUrls.isArray()) {
        AppHelperInfo* appHelperInfo (this->helperInstances[helperInstance - 1]);
        int length = appHelperSupportedUrls.property("length").toInt();
        if (length > 1) {
            qDebug() << QString("[%1] Received %2 patterns of supported URL's.").arg(appHelperInfo->name).arg(length);
        } else if (length == 1) {
            qDebug() << QString("[%1] Received a pattern of supported URL's.").arg(appHelperInfo->name);
        } else {
            qWarning() << QString("[%1] Helper declares no support for any URL pattern!").arg(appHelperInfo->name);
        }
        appHelperInfo->supportedURLs.clear ();
        for (int i = 0; i < length; i++) {
            QJSValue appHelperSupportedUrl (appHelperSupportedUrls.property (i));
            QRegExp regExpSupportedUrl;
            if (appHelperSupportedUrl.isString ()) {
                regExpSupportedUrl = QRegExp (appHelperSupportedUrl.toString(), Qt::CaseInsensitive, QRegExp::WildcardUnix);
            } else {
                regExpSupportedUrl = JavascriptBridge::RegExp2QRegExp (appHelperSupportedUrl);
            }
            if (regExpSupportedUrl.isValid ()) {
                if (regExpSupportedUrl.isEmpty ()) {
                    qWarning() << QString("[%1] Discarding the empty regular expression #%2...").arg(appHelperInfo->name).arg(i);
                } else {
                    appHelperInfo->supportedURLs.append (regExpSupportedUrl);
                }
            } else {
                qWarning() << QString("[%1] Invalid item returned: '%2'").arg(appHelperInfo->name).arg(JavascriptBridge::QJS2QString(appHelperSupportedUrl));
            }
        }
    } else {
        qWarning() << QString("Invalid returned data: (count=%1, instance=%2, data='%3')").arg(this->helperInstances.count()).arg(helperInstance).arg(JavascriptBridge::QJS2QString(appHelperSupportedUrls));
    }
}

void JobWorker::processObjectFromUrl (unsigned int requestId, const QJSValue& appHelperObjectFromUrl) {
    QMap<unsigned int,AppJobRequest*>::iterator requestIdIterator = this->runningRequests.find (requestId);
    if (requestIdIterator != this->runningRequests.end()) {
        AppJobRequest* request = (*requestIdIterator);
        request->object.className.clear ();
        request->object.id.clear ();
        AppRequestType requestType (request->type());
        AppJobRequestFromHelper* helperRequest = Q_NULLPTR;
        AppJobRequestFromSquid* squidRequest = Q_NULLPTR;
        if (requestType == RequestFromHelper) {
            helperRequest = downcast<AppJobRequestFromHelper*> (request);
        } else if (requestType == RequestFromSquid) {
            squidRequest = downcast<AppJobRequestFromSquid*> (request);
        }
        if (helperRequest != Q_NULLPTR || squidRequest != Q_NULLPTR) {
            if (appHelperObjectFromUrl.isObject ()) {
                QJSValue className = appHelperObjectFromUrl.property("className");
                QJSValue id = appHelperObjectFromUrl.property("id");
                if (! (JavascriptBridge::valueIsEmpty (className) || JavascriptBridge::valueIsEmpty (id))) {
                    request->object.className = className.toString ();
                    request->object.id = id.toString ();
                }
            } else if (appHelperObjectFromUrl.isString ()) {
                QString jsonString (appHelperObjectFromUrl.toString());
                QJsonParseError jsonParseError;
                QJsonDocument jsonHelperObjectFromUrl (QJsonDocument::fromJson (jsonString.toUtf8(), &jsonParseError));
                if (jsonParseError.error != QJsonParseError::NoError) {
                    if (requestType == RequestFromHelper) {
                        qInfo() << QString("[%1#%2] Data requested by the helper could not be parsed as JSON: (rawData='%3', offset=%4, errorString='%5')").arg(request->helper.name).arg(requestId).arg(jsonString).arg(jsonParseError.offset).arg(jsonParseError.errorString());
                    } else if (requestType == RequestFromSquid) {
                        qInfo() << QString("[%1#%2] Data returned by the helper could not be parsed as JSON: (URL='%3', rawData='%4', offset=%5, errorString='%6')").arg(request->helper.name).arg(requestId).arg(squidRequest->data.url.toString()).arg(jsonString).arg(jsonParseError.offset).arg(jsonParseError.errorString());
                    } else {
                        qFatal("Unexpected code flow!");
                    }
                }
                request->object.className = jsonHelperObjectFromUrl.object().value("className").toString();
                request->object.id = jsonHelperObjectFromUrl.object().value("id").toString();
            }
            if (request->object.className.isEmpty() || request->object.id.isEmpty ()) {
                if (requestType == RequestFromHelper) {
                    this->scriptResponseOut (requestIdIterator, helperRequest, QJSValue::NullValue);
                } else if (requestType == RequestFromSquid) {
                    this->tryNextHelper (requestIdIterator);
                } else {
                    qFatal("Unexpected code flow!");
                }
            } else {
                AppHelperInfo* appHelperInfo (this->helperInstances[request->helper.id]);
                if (requestType == RequestFromHelper) {
                    qDebug() << QString("[%1#%2] Helper is requesting information concerning (className='%3', id='%4')...").arg(request->helper.name).arg(requestId).arg(request->object.className).arg(request->object.id);
                } else if (requestType == RequestFromSquid) {
                    qDebug() << QString("[%1#%2] Searching for information concerning (className='%3', id='%4')...").arg(request->helper.name).arg(requestId).arg(request->object.className).arg(request->object.id);
                } else {
                    qFatal("Unexpected code flow!");
                }
                QJsonDocument objectData;
                qint64 objectTimestamp;
                CacheStatus cacheStatus = appHelperInfo->memoryCache->read (requestId, request->object.className, request->object.id, this->currentTimestamp, objectData, objectTimestamp);
                if (cacheStatus == CacheHitPositive) {
                    qDebug() << QString("[%1#%2] Information retrieved from the cache concerning (className='%3', id='%4') is fresh.").arg(request->helper.name).arg(requestId).arg(request->object.className).arg(request->object.id);
                    if (requestType == RequestFromHelper) {
                        this->scriptResponseOut (requestIdIterator, helperRequest, JavascriptBridge::QJson2QJS ((*(this->runtimeEnvironment)), objectData));
                    } else if (requestType == RequestFromSquid) {
                        bool matchResult = this->processCriteria (
                            squidRequest->helper.name,
                            requestId,
                            squidRequest->data.properties.count(),
                            squidRequest->data.properties.constBegin(),
                            squidRequest->data.mathMatchOperator,
                            squidRequest->data.caseSensitivity,
                            squidRequest->data.patternSyntax,
                            squidRequest->data.invertMatch,
                            squidRequest->data.criteria,
                            objectData
                        );
                        this->squidResponseOut (requestIdIterator, squidRequest, QString("Cached data from the object with (className='%1', id='%2') %3 specified criteria").arg(request->object.className).arg(request->object.id).arg((matchResult) ? "matches" : "does not match"), matchResult);
                    } else {
                        qFatal("Unexpected code flow!");
                    }
                } else if (cacheStatus == CacheHitNegative) {
                    qInfo() << QString("[%1#%2] Another thread or process tried to fetch information concerning the object with (className='%3', id='%4') recently and failed to do so.").arg(request->helper.name).arg(requestId).arg(request->object.className).arg(request->object.id);
                    if (requestType == RequestFromHelper) {
                        this->scriptResponseOut (requestIdIterator, helperRequest, QJSValue::NullValue);
                    } else if (requestType == RequestFromSquid) {
                        this->tryNextHelper (requestIdIterator);
                    } else {
                        qFatal("Unexpected code flow!");
                    }
                } else if (cacheStatus == CacheOnProgress) {
                    qDebug() << QString("[%1#%2] Another thread or process is currently fetching information concerning (className='%3', id='%4'). I will try to wait for it...").arg(request->helper.name).arg(requestId).arg(request->object.className).arg(request->object.id);
                    if (requestType == RequestFromHelper) {
                        this->runningRequests.erase (requestIdIterator);
                        this->incomingRequests.prepend (request);
                        this->startRetryTimer ();
                    } else if (requestType == RequestFromSquid) {
                        request->helper.isOnProgress = true;
                        this->tryNextHelper (requestIdIterator);
                    } else {
                        qFatal("Unexpected code flow!");
                    }
                } else if (cacheStatus == CacheMiss) {
                    qDebug() << QString("[%1#%2] Information concerning (className='%3', id='%4') was not found in the cache. Invoking 'getPropertiesFromObject();'...").arg(request->helper.name).arg(requestId).arg(request->object.className).arg(request->object.id);
                    // Note: remember the reminder saved into 'objectcache.cpp'...
                    QJsonDocument empty;
                    empty.setObject (QJsonObject ());
                    // Should a failure during a database write prevent me to request object information from a helper?
                    // In this moment, i guess it should not. So, errors from ObjectCache::write() are being ignored for now.
                    appHelperInfo->memoryCache->write (requestId, request->object.className, request->object.id, empty, this->currentTimestamp);
                    QJSValue params = this->runtimeEnvironment->newArray (2);
                    params.setProperty (0, request->object.className);
                    params.setProperty (1, request->object.id);
                    if (! this->javascriptBridge->invokeMethod (appHelperInfo->entryPoint, requestId, JavascriptMethod::getPropertiesFromObject, params)) {
                        // Register a failure into the database, so the negative TTL can count.
                        appHelperInfo->memoryCache->write (requestId, request->object.className, request->object.id, QJsonDocument(), this->currentTimestamp);
                        if (requestType == RequestFromHelper) {
                            this->scriptResponseOut (requestIdIterator, helperRequest, QJSValue::NullValue);
                        } else if (requestType == RequestFromSquid) {
                            this->tryNextHelper (requestIdIterator);
                        } else {
                            qFatal("Unexpected code flow!");
                        }
                    }
                } else {
                    qFatal("Unexpected code flow!");
                }
            }
        } else {
            qFatal("Invalid procedure call: 'AppJobRequest*' could not be downcasted to 'AppJobRequestFromHelper*' nor to 'AppJobRequestFromSquid*'!");
        }
    } else {
        qWarning() << QString("Invalid returned data: (requestId=%1, data='%2')").arg(requestId).arg(JavascriptBridge::QJS2QString(appHelperObjectFromUrl));
    }
}

void JobWorker::processPropertiesFromObject (unsigned int requestId, const QJSValue& appHelperPropertiesFromObject) {
    QMap<unsigned int,AppJobRequest*>::iterator requestIdIterator = this->runningRequests.find (requestId);
    if (requestIdIterator != this->runningRequests.end()) {
        AppJobRequest* request = (*requestIdIterator);
        AppRequestType requestType (request->type());
        AppJobRequestFromHelper* helperRequest = Q_NULLPTR;
        AppJobRequestFromSquid* squidRequest = Q_NULLPTR;
        if (requestType == RequestFromHelper) {
            helperRequest = downcast<AppJobRequestFromHelper*> (request);
        } else if (requestType == RequestFromSquid) {
            squidRequest = downcast<AppJobRequestFromSquid*> (request);
        }
        if (helperRequest != Q_NULLPTR || squidRequest != Q_NULLPTR) {
            AppHelperInfo* appHelperInfo (this->helperInstances[request->helper.id]);
            QJsonDocument objectData;
            if (appHelperPropertiesFromObject.isObject ()) {
                objectData = JavascriptBridge::QJS2QJsonDocument (appHelperPropertiesFromObject);
            } else if (appHelperPropertiesFromObject.isString ()) {
                QJsonParseError jsonParseError;
                objectData = QJsonDocument::fromJson (appHelperPropertiesFromObject.toString().toUtf8());
                if (jsonParseError.error != QJsonParseError::NoError) {
                    qInfo() << QString("[%1#%2] Data returned by the helper could not be parsed as JSON: (className='%3', id='%4', offset=%5, errorString=%6'!").arg(request->helper.name).arg(requestId).arg(request->object.className).arg(request->object.id).arg(jsonParseError.offset).arg(jsonParseError.errorString());
                }
            }
            bool validData = ObjectCache::jsonDocumentHasData (objectData);
            if (! validData) {
                qWarning() << QString("[%1#%2] Data returned by the helper is not valid: (className='%3', id='%4', rawData='%5')").arg(request->helper.name).arg(requestId).arg(request->object.className).arg(request->object.id).arg(QString::fromUtf8 (objectData.toJson(QJsonDocument::Compact)));
                objectData = QJsonDocument();
            }
            if (! appHelperInfo->memoryCache->write (requestId, request->object.className, request->object.id, objectData, this->currentTimestamp)) {
                qCritical() << QString("[%1#%2] Failed to save object information for (className='%3', id='%4', rawData='%5')!").arg(request->helper.name).arg(requestId).arg(request->object.className).arg(request->object.id).arg(QString::fromUtf8 (objectData.toJson (QJsonDocument::Compact)));
            }
            if (requestType == RequestFromHelper) {
                if (validData) {
                    this->scriptResponseOut (requestIdIterator, helperRequest, JavascriptBridge::QJson2QJS ((*(this->runtimeEnvironment)), objectData));
                } else {
                    this->scriptResponseOut (requestIdIterator, helperRequest, QJSValue::NullValue);
                }
            } else if (requestType == RequestFromSquid) {
                if (validData) {
                    bool matchResult = this->processCriteria (
                        squidRequest->helper.name,
                        requestId,
                        squidRequest->data.properties.count(),
                        squidRequest->data.properties.constBegin(),
                        squidRequest->data.mathMatchOperator,
                        squidRequest->data.caseSensitivity,
                        squidRequest->data.patternSyntax,
                        squidRequest->data.invertMatch,
                        squidRequest->data.criteria,
                        objectData
                    );
                    this->squidResponseOut (requestIdIterator, squidRequest, QString("Retrieved data from the object with (className='%1', id='%2') %3 specified criteria").arg(request->object.className).arg(request->object.id).arg((matchResult) ? "matches" : "does not match"), matchResult);
                } else {
                    this->tryNextHelper (requestIdIterator);
                }
            } else {
                qFatal("Unexpected code flow!");
            }
        } else {
            qFatal("Invalid procedure call: 'AppJobRequest*' could not be downcasted to 'AppJobRequestFromHelper*' nor to 'AppJobRequestFromSquid*'!");
        }
    } else {
        qWarning() << QString("Invalid returned data: (requestId=%1, data='%2')").arg(requestId).arg(JavascriptBridge::QJS2QString(appHelperPropertiesFromObject));
    }
}

QString JobWorker::jsonType (const QJsonValue& jsonValue) {
    QJsonValue::Type jsonValueType = jsonValue.type();
    if (jsonValueType == QJsonValue::Null) {
        return ("QJsonValue::Null");
    } else if (jsonValueType == QJsonValue::Bool) {
        return ("QJsonValue::Bool");
    } else if (jsonValueType == QJsonValue::Double) {
        return ("QJsonValue::Double");
    } else if (jsonValueType == QJsonValue::String) {
        return ("QJsonValue::String");
    } else if (jsonValueType == QJsonValue::Array) {
        return ("QJsonValue::Array");
    } else if (jsonValueType == QJsonValue::Object) {
        return ("QJsonValue::Object");
    } else if (jsonValueType == QJsonValue::Undefined) {
        return ("QJsonValue::Undefined");
    } else {
        return ("(unknown)");
    }
}

bool JobWorker::processCriteria (
    const QString& requestHelperName,
    const unsigned int requestId,
    const int level,
    const QLinkedList<AppSquidPropertyMatch>::const_iterator& requestPropertiesIterator,
    const AppSquidMathMatchOperator& requestMathMatchOperator,
    const Qt::CaseSensitivity& requestCaseSensitivity,
    const QRegExp::PatternSyntax& requestPatternSyntax,
    bool requestInvertMatch,
    const QStringList& requestCriteria,
    const QJsonDocument& jsonDocumentInformation) {
    bool answer;
    if (jsonDocumentInformation.isObject ()) {
        answer = JobWorker::processCriteria (
            requestHelperName,
            requestId,
            level,
            requestPropertiesIterator,
            requestMathMatchOperator,
            requestCaseSensitivity,
            requestPatternSyntax,
            requestCriteria,
            jsonDocumentInformation.object()
        );
    } else if (jsonDocumentInformation.isArray ()) {
        answer = JobWorker::processCriteria (
            requestHelperName,
            requestId,
            level,
            requestPropertiesIterator,
            requestMathMatchOperator,
            requestCaseSensitivity,
            requestPatternSyntax,
            requestCriteria,
            jsonDocumentInformation.array()
        );
    } else {
        qInfo() << QString("[%1#%2] Unexpected JSON format while parsing '%3'.").arg(requestHelperName).arg(requestId).arg(requestPropertiesIterator->componentName);
        answer = false;
    }
    if (requestInvertMatch) {
        return (! answer);
    } else {
        return (answer);
    }
}

bool JobWorker::processCriteria (
    const QString& requestHelperName,
    const unsigned int requestId,
    const int level,
    const QLinkedList<AppSquidPropertyMatch>::const_iterator& requestPropertiesIterator,
    const AppSquidMathMatchOperator& requestMathMatchOperator,
    const Qt::CaseSensitivity& requestCaseSensitivity,
    const QRegExp::PatternSyntax& requestPatternSyntax,
    const QStringList& requestCriteria,
    const QJsonValue& jsonValueInformation) {
    if (level > 0) {
        AppSquidPropertyMatch requestPropertiesItem = (*requestPropertiesIterator);
        if (requestPropertiesItem.matchType == MatchObject) {
            if (jsonValueInformation.isObject ()) {
                return (JobWorker::processCriteria (
                    requestHelperName,
                    requestId,
                    level - 1,
                    requestPropertiesIterator + 1,
                    requestMathMatchOperator,
                    requestCaseSensitivity,
                    requestPatternSyntax,
                    requestCriteria,
                    jsonValueInformation.toObject().value(requestPropertiesItem.componentName)
                ));
            } else {
                qInfo() << QString("[%1#%2] Unexpected JSON type '%3' while parsing '%4'. A '%5' was expected.").arg(requestHelperName).arg(requestId).arg(JobWorker::jsonType(jsonValueInformation)).arg(requestPropertiesItem.componentName).arg(JobWorker::jsonType(QJsonValue::Object));
            }
        } else if (requestPropertiesItem.matchType == MatchArray) {
            if (jsonValueInformation.isArray ()) {
                QJsonArray jsonArray (jsonValueInformation.toArray ());
                int arraySize = jsonArray.count ();
                int intervalStart, intervalEnd, intervalItem;
                for (QList< QPair<int,int> >::const_iterator intervalIterator = requestPropertiesItem.matchIntervals.constBegin(); intervalIterator != requestPropertiesItem.matchIntervals.constEnd(); intervalIterator++) {
                    intervalStart = intervalIterator->first;
                    if (intervalStart < 0) {
                        intervalStart = arraySize + intervalStart;
                    }
                    if (intervalStart < 0) {
                        intervalStart = 0;
                    }
                    if (intervalStart >= arraySize) {
                        intervalStart = arraySize - 1;
                    }
                    intervalEnd = intervalIterator->second;
                    if (intervalEnd < 0) {
                        intervalEnd = arraySize + intervalEnd;
                    }
                    if (intervalEnd < 0) {
                        intervalEnd = 0;
                    }
                    if (intervalEnd >= arraySize) {
                        intervalEnd = arraySize - 1;
                    }
                    for (intervalItem = intervalStart; intervalItem <= intervalEnd; intervalItem++) {
                        if (JobWorker::processCriteria (
                            requestHelperName,
                            requestId,
                            level - 1,
                            requestPropertiesIterator + 1,
                            requestMathMatchOperator,
                            requestCaseSensitivity,
                            requestPatternSyntax,
                            requestCriteria,
                            jsonArray.at (intervalItem))) {
                            if (requestPropertiesItem.matchQuantity == MatchAny) {
                                return (true);
                            }
                        } else {
                            if (requestPropertiesItem.matchQuantity == MatchAll) {
                                return (false);
                            }
                        }
                    }
                }
                if (requestPropertiesItem.matchQuantity == MatchAll) {
                    return (true);
                } else if (requestPropertiesItem.matchQuantity == MatchAny) {
                    return (false);
                } else {
                    qFatal("Unexpected code flow!");
                }
            } else {
                qInfo() << QString("[%1#%2] Unexpected JSON type '%3' while parsing '%4'. A '%5' was expected.").arg(requestHelperName).arg(requestId).arg(JobWorker::jsonType(jsonValueInformation)).arg(requestPropertiesItem.componentName).arg(JobWorker::jsonType(QJsonValue::Array));
            }
        } else {
            qFatal("Unexpected code flow!");
        }
    } else {
        // Match the leaf as Bool, Double or String
        if (jsonValueInformation.isBool ()) {
            static QList<QStringList> booleanOptions (
                QList<QStringList> ()
                    << ( QStringList()
                        << "n"
                        << "no"
                        << "0"
                        << "false"
                        << "$false"
                        << "off"
                    ) << ( QStringList()
                        << "y"
                        << "yes"
                        << "1"
                        << "true"
                        << "$true"
                        << "on"
                    )
            );
            int booleanOptionsPos = ((jsonValueInformation.toBool ()) ? 1 : 0);
            if (requestMathMatchOperator == OperatorString ||
                requestMathMatchOperator == OperatorEquals ||
                requestMathMatchOperator == OperatorNotEquals) {
                bool matched = (requestMathMatchOperator == OperatorNotEquals);
                for (QStringList::const_iterator requestCriteriaIterator = requestCriteria.constBegin(); requestCriteriaIterator != requestCriteria.constEnd(); requestCriteriaIterator++) {
                    if (booleanOptions[booleanOptionsPos].contains ((*requestCriteriaIterator), Qt::CaseInsensitive)) {
                        matched = (! matched);
                        break;
                    } else if (! booleanOptions[1 - booleanOptionsPos].contains ((*requestCriteriaIterator), Qt::CaseInsensitive)) {
                        qWarning() << QString("[%1#%2] Unable to evaluate '%3' as a boolean value").arg(requestHelperName).arg(requestId).arg(*requestCriteriaIterator);
                        return (false);
                    }
                }
                return (matched);
            } else {
                qInfo() << QString("[%1#%2] Unable to apply selected comparison operator on a boolean value!").arg(requestHelperName).arg(requestId);
            }
        } else if (jsonValueInformation.isDouble ()) {
            if (requestMathMatchOperator == OperatorLessThan ||
                requestMathMatchOperator == OperatorLessThanOrEquals ||
                requestMathMatchOperator == OperatorEquals ||
                requestMathMatchOperator == OperatorNotEquals ||
                requestMathMatchOperator == OperatorGreaterThanOrEquals ||
                requestMathMatchOperator == OperatorGreaterThan) {
                double doubleValue = jsonValueInformation.toDouble ();
                double doubleComparison;
                bool conversionOk;
                for (QStringList::const_iterator requestCriteriaIterator = requestCriteria.constBegin(); requestCriteriaIterator != requestCriteria.constEnd(); requestCriteriaIterator++) {
                    doubleComparison = requestCriteriaIterator->toDouble (&conversionOk);
                    if (conversionOk) {
                        if ((requestMathMatchOperator == OperatorLessThan && doubleValue < doubleComparison) ||
                            (requestMathMatchOperator == OperatorLessThanOrEquals && doubleValue <= doubleComparison) ||
                            (requestMathMatchOperator == OperatorEquals && doubleValue == doubleComparison) ||
                            (requestMathMatchOperator == OperatorNotEquals && doubleValue != doubleComparison) ||
                            (requestMathMatchOperator == OperatorGreaterThanOrEquals && doubleValue >= doubleComparison) ||
                            (requestMathMatchOperator == OperatorGreaterThan && doubleValue > doubleComparison)) {
                            return (true);
                        }
                    } else {
                        qWarning() << QString("[%1#%2] Unable to evaluate '%3' as a numeric value").arg(requestHelperName).arg(requestId).arg(*requestCriteriaIterator);
                        return (false);
                    }
                }
            } else {
                qInfo() << QString("[%1#%2] Unable to apply selected comparison operator on a numeric value!").arg(requestHelperName).arg(requestId);
            }
        } else if (jsonValueInformation.isString ()) {
            if (requestMathMatchOperator == OperatorString ||
                requestMathMatchOperator == OperatorLessThan ||
                requestMathMatchOperator == OperatorLessThanOrEquals ||
                requestMathMatchOperator == OperatorEquals ||
                requestMathMatchOperator == OperatorNotEquals ||
                requestMathMatchOperator == OperatorGreaterThanOrEquals ||
                requestMathMatchOperator == OperatorGreaterThan) {
                QString stringValue (jsonValueInformation.toString ());
                int compareResult;
                for (QStringList::const_iterator requestCriteriaIterator = requestCriteria.constBegin(); requestCriteriaIterator != requestCriteria.constEnd(); requestCriteriaIterator++) {
                    if (requestMathMatchOperator == OperatorString) {
                        QRegExp regexComparison ((*requestCriteriaIterator), requestCaseSensitivity, requestPatternSyntax);
                        if (regexComparison.isValid()) {
                            if (JobWorker::regexMatches (regexComparison, stringValue)) {
                                return (true);
                            }
                        } else {
                            qWarning() << QString("[%1#%2] Expression specification '%3' could not be parsed: '%4'").arg(requestHelperName).arg(requestId).arg(*requestCriteriaIterator).arg(regexComparison.errorString());
                            return (false);
                        }
                    } else {
                        compareResult = stringValue.compare ((*requestCriteriaIterator), requestCaseSensitivity);
                        if ((requestMathMatchOperator == OperatorLessThan && compareResult < 0) ||
                            (requestMathMatchOperator == OperatorLessThanOrEquals && compareResult <= 0) ||
                            (requestMathMatchOperator == OperatorEquals && compareResult == 0) ||
                            (requestMathMatchOperator == OperatorNotEquals && compareResult != 0) ||
                            (requestMathMatchOperator == OperatorGreaterThanOrEquals && compareResult >= 0) ||
                            (requestMathMatchOperator == OperatorGreaterThan && compareResult > 0)) {
                            return (true);
                        }
                    }
                }
            } else {
                qInfo() << QString("[%1#%2] Unable to apply selected comparison operator on a string value!").arg(requestHelperName).arg(requestId);
            }
        } else {
            qInfo() << QString("[%1#%2] Unexpected JSON type '%3' while evaluating the leaf.").arg(requestHelperName).arg(requestId).arg(JobWorker::jsonType(jsonValueInformation));
        }
    }
    return (false);
}

bool JobWorker::regexMatches (const QRegExp& regularExpression, const QString& testString) {
    QRegExp::PatternSyntax patternSyntax = regularExpression.patternSyntax();
    switch (patternSyntax) {
        case QRegExp::RegExp:
            /* FALLTHROUGH */
        case QRegExp::RegExp2:
            return (regularExpression.indexIn (testString) >= 0);
        default:
            return (regularExpression.exactMatch (testString));
    }
}

JobWorker::JobWorker (const QString& requestChannel, QObject* parent) :
    QObject (parent),
    finishRequested (false),
    rngdInitialized (false),
    requestChannel (requestChannel),
    runtimeEnvironment (new QJSEngine (this)),
    javascriptBridge (new JavascriptBridge ((*runtimeEnvironment), requestChannel, this)),
    requestId (1),
    retryTimer (new QTimer (this)),
    currentTimestamp (0) {
    QObject::connect (this->javascriptBridge, &JavascriptBridge::valueReturnedFromJavascript, this, &JobWorker::valueReturnedFromJavascript);
    QObject::connect (this->retryTimer, &QTimer::timeout, this, &JobWorker::setCurrentTimestamp);
    QObject::connect (this->retryTimer, &QTimer::timeout, this, &JobWorker::processIncomingRequest);
    int helperNamesLength = AppRuntime::helperNames.count ();
    for (int helperNamesPos = 0; helperNamesPos < helperNamesLength; helperNamesPos++) {
        AppHelperInfo* appHelperInfo = new AppHelperInfo;
        this->helperInstances.append (appHelperInfo);
        appHelperInfo->name = AppRuntime::helperNames.at(helperNamesPos);
        appHelperInfo->databaseCache = new ObjectCacheDatabase (appHelperInfo->name, AppRuntime::dbTblPrefix);
        appHelperInfo->memoryCache = new ObjectCacheMemory (appHelperInfo->name, (*(appHelperInfo->databaseCache)));
        appHelperInfo->entryPoint = javascriptBridge->makeEntryPoint (helperNamesPos);
        if (appHelperInfo->entryPoint.isCallable ()) {
            this->javascriptBridge->invokeMethod (appHelperInfo->entryPoint, this->helperInstances.count(), JavascriptMethod::getSupportedUrls);
        }
    }
}

JobWorker::~JobWorker () {
    if ((! (this->runningRequests.isEmpty() && this->finishRequested && this->incomingRequests.isEmpty ())) || this->retryTimer->isActive ()) {
        qFatal("Program tried to destruct a JobWorker unexpectedly!");
    }
    while (! this->helperInstances.isEmpty ()) {
        AppHelperInfo* appHelperInfo (this->helperInstances.takeLast ());
        delete (appHelperInfo->memoryCache);
        delete (appHelperInfo->databaseCache);
        delete (appHelperInfo);
    }
    delete (this->retryTimer);
    delete (this->runtimeEnvironment);
}

void JobWorker::valueReturnedFromJavascript (unsigned int context, const QString& method, const QJSValue& returnedValue) {
    if (method == "getSupportedUrls") {
        this->processSupportedUrls (context, returnedValue);
    } else if (method == "getObjectFromUrl") {
        this->processObjectFromUrl (context, returnedValue);
    } else if (method == "getPropertiesFromObject") {
        this->processPropertiesFromObject (context, returnedValue);
    } else if (method == "getPropertiesFromObjectCache") {
        QJSValue callback (returnedValue.property("callback"));
        if (callback.isCallable() && context < ((unsigned) this->helperInstances.count())) {
            AppJobRequestFromHelper* helperRequest = new AppJobRequestFromHelper ();
            helperRequest->data.callback = callback;
            helperRequest->data.object = returnedValue.property("object");
            helperRequest->helper.id = context;
            helperRequest->helper.name = this->helperInstances[context]->name;
            this->incomingRequests.append (helperRequest);
            this->processIncomingRequest ();
        }
    } else {
        qCritical() << QString("Javascript returned value from an unexpected method invocation: (context=%1, method='%2', returnedValue='%3')").arg(context).arg(method).arg(JavascriptBridge::QJS2QString (returnedValue));
    }
}

void JobWorker::processIncomingRequest () {
    AppJobRequest* request (this->incomingRequests.takeLast());
    if (this->incomingRequests.isEmpty ()) {
        this->retryTimer->stop ();
    }
    AppRequestType requestType (request->type());
    AppJobRequestFromHelper* helperRequest = Q_NULLPTR;
    AppJobRequestFromSquid* squidRequest = Q_NULLPTR;
    AppHelperInfo* appHelperInfo = Q_NULLPTR;
    QString urlString;
    if (requestType == RequestFromHelper) {
        helperRequest = downcast<AppJobRequestFromHelper*> (request);
    } else if (requestType == RequestFromSquid) {
        squidRequest = downcast<AppJobRequestFromSquid*> (request);
    }
    if (helperRequest != Q_NULLPTR || squidRequest != Q_NULLPTR) {
        if (request->helper.name.isEmpty ()) {
            if (requestType == RequestFromSquid) {
                urlString = squidRequest->data.url.toString ();
                int numHelpers = this->helperInstances.count ();
                for (int helperPos = squidRequest->helper.id; helperPos < numHelpers; helperPos++) {
                    appHelperInfo = this->helperInstances[helperPos];
                    if (appHelperInfo->entryPoint.isCallable ()) {
                        int numSupportedUrls = appHelperInfo->supportedURLs.count ();
                        for (int supportedUrlPos = 0; supportedUrlPos < numSupportedUrls; supportedUrlPos++) {
                            if (JobWorker::regexMatches (appHelperInfo->supportedURLs[supportedUrlPos], urlString)) {
                                squidRequest->helper.name = appHelperInfo->name;
                                squidRequest->helper.id = helperPos;
                                break;
                            }
                        }
                    }
                    if (! squidRequest->helper.name.isEmpty()) {
                        break;
                    }
                }
                if (squidRequest->helper.name.isEmpty ()) {
                    if (squidRequest->helper.isOnProgress) {
                        squidRequest->helper.isOnProgress = false;
                        squidRequest->helper.id = 0;
                        this->incomingRequests.prepend (squidRequest);
                        this->startRetryTimer ();
                    } else {
                        QMap<unsigned int,AppJobRequest*>::iterator end(this->runningRequests.end());
                        this->squidResponseOut (end, squidRequest, "Unable to find a helper that can handle the requested URL", false);
                    }
                }
            } else {
                qFatal("Unexpected code flow!");
            }
        } else {
            appHelperInfo = this->helperInstances[request->helper.id];
        }
        if (! request->helper.name.isEmpty ()) {
            unsigned int requestId (this->requestId);
            this->requestId += 2;
            this->runningRequests.insert (requestId, request);
            if (requestType == RequestFromHelper) {
                this->processObjectFromUrl (requestId, helperRequest->data.object);
            } else if (requestType == RequestFromSquid) {
                qDebug() << QString("[%1#%2] Invoking 'getObjectFromUrl();'...").arg(appHelperInfo->name).arg(requestId);
                if (! this->javascriptBridge->invokeMethod (appHelperInfo->entryPoint, requestId, JavascriptMethod::getObjectFromUrl, urlString)) {
                    this->tryNextHelper (requestId);
                }
            } else {
                qFatal("Unexpected code flow!");
            }
        }
    } else {
        qFatal("Invalid procedure call: 'AppJobRequest*' could not be downcasted to 'AppJobRequestFromHelper*' nor to 'AppJobRequestFromSquid*'!");
    }
}

void JobWorker::setCurrentTimestamp () {
    // I need to update this->currentTimestamp manually if the JobWorker stops receiving jobs from JobDispatcher
    // probably because of an EOF at STDIN or an absence of requests from Squid.
    qint64 currentTimestamp (AppRuntime::currentDateTime().toMSecsSinceEpoch());
    if (! this->rngdInitialized) {
        qsrand ((uint) currentTimestamp);
        QThread::msleep (1);
        this->rngdInitialized = true;
    }
    this->currentTimestamp = (currentTimestamp / 1000);
}

void JobWorker::squidRequestIn (AppJobRequestFromSquid* request, const qint64 timestampNow) {
    qDebug() << QString("JobDispatcher has sent an ACL matching request to channel #%1.").arg(this->requestChannel);
    if (this->currentTimestamp < timestampNow) {
        this->currentTimestamp = timestampNow;
    }
    request->helper.id = 0;
    request->helper.name.clear();
    request->helper.isOnProgress = false;
    this->incomingRequests.append (request);
    this->processIncomingRequest ();
}

void JobWorker::quit () {
    this->finishRequested = true;
    int numPendingRequests (this->runningRequests.count () + this->incomingRequests.count());
    if (numPendingRequests == 0) {
        qInfo() << QString("Finished handler for channel #%1.").arg(this->requestChannel);
        emit finished ();
    } else if (numPendingRequests == 1) {
        qInfo() << QString("A finish request was received by handler for channel #%1, but there is a pending answer.").arg(this->requestChannel);
    } else {
        qInfo() << QString("A finish request was received by handler for channel #%1, but there are %2 pending answers.").arg(this->requestChannel).arg(numPendingRequests);
    }
}

//////////////////////////////////////////////////////////////////

JobCarrier::JobCarrier (const QString& requestChannel, QObject* parent) :
    QObject (parent),
    started (false),
    threadObj (new QThread (this)),
    workerObj (new JobWorker (requestChannel, Q_NULLPTR)) {
    this->workerObj->moveToThread (this->threadObj);
    QObject::connect (this->workerObj, &JobWorker::finished, this->threadObj, &QThread::quit);
    QObject::connect (this->threadObj, &QThread::finished, this->workerObj, &QObject::deleteLater);
    QObject::connect (this->threadObj, &QThread::finished, this, &JobCarrier::finished);
    QObject::connect (this, &JobCarrier::squidRequestOut, this->workerObj, &JobWorker::squidRequestIn, Qt::QueuedConnection);
}

JobCarrier::~JobCarrier () {
    if (this->threadObj->isRunning ()) {
        qFatal("Program tried to destruct a JobCarrier unexpectedly!");
    }
    delete (this->threadObj);
}

void JobCarrier::start (QThread::Priority priority) {
    if (this->started) {
        qFatal("Invalid procedure call: this method must be called only once!");
    } else {
        this->threadObj->start (priority);
        this->started = true;
    }
}

void JobCarrier::squidRequestIn (AppJobRequestFromSquid* request, const qint64 timestampNow) {
    emit squidRequestOut (request, timestampNow);
}
