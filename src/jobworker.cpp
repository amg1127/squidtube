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

#include "jobworker.h"

void JobWorker::startRetryTimer () {
    if (this->rngdInitialized) {
        this->retryTimer->start (AppConstants::AppHelperTimerTimeout + (static_cast<int> (((static_cast<double> (qrand())) / (static_cast<double> (RAND_MAX))) * (static_cast<double> (AppConstants::AppHelperTimerTimeout)))));
    } else {
        this->retryTimer->start (AppConstants::AppHelperTimerTimeout);
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
        qFatal ("Invalid procedure call: 'AppJobRequest*' could not be downcasted to 'AppJobRequestFromSquid*'!");
    }
}

void JobWorker::squidResponseOut (QMap<unsigned int,AppJobRequest*>::iterator& requestIdIterator, AppJobRequestFromSquid* squidRequest, const QString& msg, bool isMatch) {
    if (requestIdIterator != this->runningRequests.end ()) {
        unsigned int _requestId (requestIdIterator.key ());
        this->runningRequests.erase (requestIdIterator);
        emit writeAnswerLine (this->requestChannel, QStringLiteral("[%1#%2] %3").arg(squidRequest->helper.name, QString::number(_requestId), msg), false, isMatch);
    } else {
        emit writeAnswerLine (this->requestChannel, msg, false, isMatch);
    }
    delete (squidRequest);
    if (this->finishRequested && this->runningRequests.isEmpty () && this->incomingRequests.isEmpty ()) {
        qInfo ("No pending jobs now. Finished handler for channel #%s.", this->requestChannel.toLatin1().constData());
        emit finished ();
    }
}

void JobWorker::scriptResponseOut (QMap<unsigned int,AppJobRequest*>::iterator& requestIdIterator, AppJobRequestFromHelper *helperRequest, const QJSValue &returnValue) {
    unsigned int _requestId (requestIdIterator.key ());
    this->runningRequests.erase (requestIdIterator);
    JavascriptBridge::warnJsError ((*(this->runtimeEnvironment)), helperRequest->data.callback.call (QJSValueList() << returnValue), QByteArray("[") + helperRequest->helper.name.toLatin1() + "#" + QByteArray::number(_requestId) + "] Uncaught exception received while calling the callback received as argument from a previous call to 'getPropertiesFromObjectCache();' (className='" + helperRequest->object.className.toLatin1() + "', id='" + helperRequest->object.id.toLatin1() + "')");
    delete (helperRequest);
}

void JobWorker::processSupportedUrls (int helperInstance, const QJSValue& appHelperSupportedUrls) {
    if (helperInstance > 0 && helperInstance <= this->helperInstances.count() && appHelperSupportedUrls.isArray()) {
        AppHelperInfo* appHelperInfo (this->helperInstances[helperInstance - 1]);
        unsigned int length = appHelperSupportedUrls.property(QStringLiteral("length")).toUInt();
        if (length > 1) {
            qDebug ("[%s] Received %d patterns of supported URL's.", appHelperInfo->name.toLatin1().constData(), length);
        } else if (length == 1) {
            qDebug ("[%s] Received a pattern of supported URL's.", appHelperInfo->name.toLatin1().constData());
        } else {
            qWarning ("[%s] Helper declares no support for any URL pattern!", appHelperInfo->name.toLatin1().constData());
        }
        appHelperInfo->supportedURLs.clear ();
        for (unsigned int i = 0; i < length; i++) {
            QJSValue appHelperSupportedUrl (appHelperSupportedUrls.property (i));
            QRegExp regExpSupportedUrl;
            if (appHelperSupportedUrl.isString ()) {
                regExpSupportedUrl = QRegExp (appHelperSupportedUrl.toString(), Qt::CaseInsensitive, QRegExp::WildcardUnix);
            } else {
                regExpSupportedUrl = JavascriptBridge::RegExp2QRegExp (appHelperSupportedUrl);
            }
            if (regExpSupportedUrl.isValid ()) {
                if (regExpSupportedUrl.isEmpty ()) {
                    qWarning ("[%s] Discarding the empty regular expression #%d...", appHelperInfo->name.toLatin1().constData(), i);
                } else {
                    appHelperInfo->supportedURLs.append (regExpSupportedUrl);
                }
            } else {
                qWarning ("[%s] Invalid item returned: '%s'", appHelperInfo->name.toLatin1().constData(), JavascriptBridge::QJS2QString(appHelperSupportedUrl).toLatin1().constData());
            }
        }
    } else {
        qWarning ("Invalid returned data: (count=%d, instance=%d, data='%s')", this->helperInstances.count(), helperInstance, JavascriptBridge::QJS2QString(appHelperSupportedUrls).toLatin1().constData());
    }
}

void JobWorker::processObjectFromUrl (unsigned int _requestId, const QJSValue& appHelperObjectFromUrl) {
    QMap<unsigned int,AppJobRequest*>::iterator requestIdIterator = this->runningRequests.find (_requestId);
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
                QJSValue className = appHelperObjectFromUrl.property(QStringLiteral("className"));
                QJSValue id = appHelperObjectFromUrl.property(QStringLiteral("id"));
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
                        qInfo ("[%s#%u] Data requested by the helper could not be parsed as JSON: (rawData='%s', offset=%d, error=%d, errorString='%s')", request->helper.name.toLatin1().constData(), _requestId, jsonString.toLatin1().constData(), jsonParseError.offset, jsonParseError.error, jsonParseError.errorString().toLatin1().constData());
                    } else if (requestType == RequestFromSquid) {
                        qInfo ("[%s#%u] Data returned by the helper could not be parsed as JSON: (URL='%s', rawData='%s', offset=%d, error=%d, errorString='%s')", request->helper.name.toLatin1().constData(), _requestId, squidRequest->data.url.toDisplayString(QUrl::PrettyDecoded | QUrl::RemoveUserInfo).toLatin1().constData(), jsonString.toLatin1().constData(), jsonParseError.offset, jsonParseError.error, jsonParseError.errorString().toLatin1().constData());
                    } else {
                        qFatal ("Unexpected code flow!");
                    }
                }
                request->object.className = jsonHelperObjectFromUrl.object().value(QStringLiteral("className")).toString();
                request->object.id = jsonHelperObjectFromUrl.object().value(QStringLiteral("id")).toString();
            }
            if (request->object.className.isEmpty() || request->object.id.isEmpty ()) {
                if (requestType == RequestFromHelper) {
                    this->scriptResponseOut (requestIdIterator, helperRequest, QJSValue::NullValue);
                } else if (requestType == RequestFromSquid) {
                    this->tryNextHelper (requestIdIterator);
                } else {
                    qFatal ("Unexpected code flow!");
                }
            } else {
                AppHelperInfo* appHelperInfo (this->helperInstances[request->helper.id]);
                if (requestType == RequestFromHelper) {
                    qDebug ("[%s#%u] Helper is requesting information concerning (className='%s', id='%s')...", request->helper.name.toLatin1().constData(), _requestId, request->object.className.toLatin1().constData(), request->object.id.toLatin1().constData());
                } else if (requestType == RequestFromSquid) {
                    qDebug ("[%s#%u] Searching for information concerning (className='%s', id='%s')...", request->helper.name.toLatin1().constData(), _requestId, request->object.className.toLatin1().constData(), request->object.id.toLatin1().constData());
                } else {
                    qFatal ("Unexpected code flow!");
                }
                QJsonDocument objectData;
                qint64 objectTimestamp;
                CacheStatus cacheStatus = appHelperInfo->memoryCache->read (_requestId, request->object.className, request->object.id, this->currentTimestamp, objectData, objectTimestamp);
                if (cacheStatus == CacheHitPositive) {
                    qDebug ("[%s#%u] Information retrieved from the cache concerning (className='%s', id='%s') is fresh.", request->helper.name.toLatin1().constData(), _requestId, request->object.className.toLatin1().constData(), request->object.id.toLatin1().constData());
                    if (requestType == RequestFromHelper) {
                        this->scriptResponseOut (requestIdIterator, helperRequest, JavascriptBridge::QJson2QJS ((*(this->runtimeEnvironment)), objectData));
                    } else if (requestType == RequestFromSquid) {
                        bool matchResult = this->processCriteria (
                            squidRequest->helper.name,
                            _requestId,
                            squidRequest->data.properties.count(),
                            squidRequest->data.properties.constBegin(),
                            squidRequest->data.mathMatchOperator,
                            squidRequest->data.caseSensitivity,
                            squidRequest->data.patternSyntax,
                            squidRequest->data.criteria,
                            objectData
                        );
                        if (squidRequest->data.invertedMatch) {
                            matchResult = (! matchResult);
                        }
                        this->squidResponseOut (requestIdIterator, squidRequest, QStringLiteral("Cached data from the object with (className='%1', id='%2') %3 specified criteria").arg(request->object.className, request->object.id, ((matchResult) ? QStringLiteral("matches") : QStringLiteral("does not match"))), matchResult);
                    } else {
                        qFatal ("Unexpected code flow!");
                    }
                } else if (cacheStatus == CacheHitNegative) {
                    qInfo ("[%s#%u] Another thread or process tried to fetch information concerning the object with (className='%s', id='%s') recently and failed to do so.", request->helper.name.toLatin1().constData(), _requestId, request->object.className.toLatin1().constData(), request->object.id.toLatin1().constData());
                    if (requestType == RequestFromHelper) {
                        this->scriptResponseOut (requestIdIterator, helperRequest, QJSValue::NullValue);
                    } else if (requestType == RequestFromSquid) {
                        this->tryNextHelper (requestIdIterator);
                    } else {
                        qFatal ("Unexpected code flow!");
                    }
                } else if (cacheStatus == CacheOnProgress) {
                    qDebug ("[%s#%u] Another thread or process is currently fetching information concerning (className='%s', id='%s'). I will try to wait for it...", request->helper.name.toLatin1().constData(), _requestId, request->object.className.toLatin1().constData(), request->object.id.toLatin1().constData());
                    if (requestType == RequestFromHelper) {
                        this->runningRequests.erase (requestIdIterator);
                        this->incomingRequests.prepend (request);
                        this->startRetryTimer ();
                    } else if (requestType == RequestFromSquid) {
                        request->helper.isOnProgress = true;
                        this->tryNextHelper (requestIdIterator);
                    } else {
                        qFatal ("Unexpected code flow!");
                    }
                } else if (cacheStatus == CacheMiss) {
                    qDebug ("[%s#%u] Information concerning (className='%s', id='%s') was not found in the cache. Invoking 'getPropertiesFromObject();'...", request->helper.name.toLatin1().constData(), _requestId, request->object.className.toLatin1().constData(), request->object.id.toLatin1().constData());
                    // Note: remember the reminder saved into 'objectcache.cpp'...
                    QJsonDocument empty;
                    empty.setObject (QJsonObject ());
                    // Should a failure during a database write prevent me to request object information from a helper?
                    // In this moment, i guess it should not. So, errors from ObjectCache::write() are being ignored for now.
                    appHelperInfo->memoryCache->write (_requestId, request->object.className, request->object.id, empty, this->currentTimestamp);
                    QJSValue params = this->runtimeEnvironment->newArray (2);
                    params.setProperty (0, request->object.className);
                    params.setProperty (1, request->object.id);
                    if (! this->javascriptBridge->invokeMethod (appHelperInfo->entryPoint, _requestId, JavascriptMethod::getPropertiesFromObject, params)) {
                        // Register a failure into the database, so the negative TTL can count.
                        appHelperInfo->memoryCache->write (_requestId, request->object.className, request->object.id, QJsonDocument(), this->currentTimestamp);
                        if (requestType == RequestFromHelper) {
                            this->scriptResponseOut (requestIdIterator, helperRequest, QJSValue::NullValue);
                        } else if (requestType == RequestFromSquid) {
                            this->tryNextHelper (requestIdIterator);
                        } else {
                            qFatal ("Unexpected code flow!");
                        }
                    }
                } else {
                    qFatal ("Unexpected code flow!");
                }
            }
        } else {
            qFatal ("Invalid procedure call: 'AppJobRequest*' could not be downcasted to 'AppJobRequestFromHelper*' nor to 'AppJobRequestFromSquid*'!");
        }
    } else {
        qWarning ("Invalid returned data: (requestId=%u, data='%s')", _requestId, JavascriptBridge::QJS2QString(appHelperObjectFromUrl).toLatin1().constData());
    }
}

void JobWorker::processPropertiesFromObject (unsigned int _requestId, const QJSValue& appHelperPropertiesFromObject) {
    QMap<unsigned int,AppJobRequest*>::iterator requestIdIterator = this->runningRequests.find (_requestId);
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
                objectData = QJsonDocument::fromJson (appHelperPropertiesFromObject.toString().toUtf8(), &jsonParseError);
                if (jsonParseError.error != QJsonParseError::NoError) {
                    qInfo ("[%s#%u] Data returned by the helper could not be parsed as JSON: (className='%s', id='%s', offset=%d, error=%d, errorString='%s'!", request->helper.name.toLatin1().constData(), _requestId, request->object.className.toLatin1().constData(), request->object.id.toLatin1().constData(), jsonParseError.offset, jsonParseError.error, jsonParseError.errorString().toLatin1().constData());
                }
            }
            bool validData = ObjectCache::jsonDocumentHasData (objectData);
            if (! validData) {
                qWarning ("[%s#%u] Data returned by the helper is not valid: (className='%s', id='%s', rawData='%s')", request->helper.name.toLatin1().constData(), _requestId, request->object.className.toLatin1().constData(), request->object.id.toLatin1().constData(), objectData.toJson(QJsonDocument::Compact).constData());
                objectData = QJsonDocument();
            }
            if (! appHelperInfo->memoryCache->write (_requestId, request->object.className, request->object.id, objectData, this->currentTimestamp)) {
                qCritical ("[%s#%u] Failed to save object information for (className='%s', id='%s', rawData='%s')!", request->helper.name.toLatin1().constData(), _requestId, request->object.className.toLatin1().constData(), request->object.id.toLatin1().constData(), objectData.toJson (QJsonDocument::Compact).constData());
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
                        _requestId,
                        squidRequest->data.properties.count(),
                        squidRequest->data.properties.constBegin(),
                        squidRequest->data.mathMatchOperator,
                        squidRequest->data.caseSensitivity,
                        squidRequest->data.patternSyntax,
                        squidRequest->data.criteria,
                        objectData
                    );
                    if (squidRequest->data.invertedMatch) {
                        matchResult = (! matchResult);
                    }
                    this->squidResponseOut (requestIdIterator, squidRequest, QStringLiteral("Retrieved data from the object with (className='%1', id='%2') %3 specified criteria").arg(request->object.className, request->object.id, ((matchResult) ? QStringLiteral("matches") : QStringLiteral("does not match"))), matchResult);
                } else {
                    this->tryNextHelper (requestIdIterator);
                }
            } else {
                qFatal ("Unexpected code flow!");
            }
        } else {
            qFatal ("Invalid procedure call: 'AppJobRequest*' could not be downcasted to 'AppJobRequestFromHelper*' nor to 'AppJobRequestFromSquid*'!");
        }
    } else {
        qWarning ("Invalid returned data: (requestId=%u, data='%s')", _requestId, JavascriptBridge::QJS2QString(appHelperPropertiesFromObject).toLatin1().constData());
    }
}

const char* JobWorker::jsonType (const QJsonValue& jsonValue) {
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
    const unsigned int _requestId,
    const int level,
    const QLinkedList<AppSquidPropertyMatch>::const_iterator& requestPropertiesIterator,
    const AppSquidMathMatchOperator& requestMathMatchOperator,
    const Qt::CaseSensitivity& requestCaseSensitivity,
    const QRegExp::PatternSyntax& requestPatternSyntax,
    const QStringList& requestCriteria,
    const QJsonDocument& jsonDocumentInformation) {
    bool answer;
    if (jsonDocumentInformation.isObject ()) {
        answer = JobWorker::processCriteria (
            requestHelperName,
            _requestId,
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
            _requestId,
            level,
            requestPropertiesIterator,
            requestMathMatchOperator,
            requestCaseSensitivity,
            requestPatternSyntax,
            requestCriteria,
            jsonDocumentInformation.array()
        );
    } else {
        qInfo ("[%s#%u] Unexpected JSON format while parsing '%s'.", requestHelperName.toLatin1().constData(), _requestId, requestPropertiesIterator->componentName.toLatin1().constData());
        answer = false;
    }
    return (answer);
}

bool JobWorker::processCriteria (
    const QString& requestHelperName,
    const unsigned int _requestId,
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
                    _requestId,
                    level - 1,
                    requestPropertiesIterator + 1,
                    requestMathMatchOperator,
                    requestCaseSensitivity,
                    requestPatternSyntax,
                    requestCriteria,
                    jsonValueInformation.toObject().value(requestPropertiesItem.componentName)
                ));
            } else {
                qInfo ("[%s#%u] Unexpected JSON type '%s' while parsing '%s'. A '%s' was expected.", requestHelperName.toLatin1().constData(), _requestId, JobWorker::jsonType(jsonValueInformation), requestPropertiesItem.componentName.toLatin1().constData(), JobWorker::jsonType(QJsonValue::Object));
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
                            _requestId,
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
                    qFatal ("Unexpected code flow!");
                }
            } else {
                qInfo ("[%s#%u] Unexpected JSON type '%s' while parsing '%s'. A '%s' was expected.", requestHelperName.toLatin1().constData(), _requestId, JobWorker::jsonType(jsonValueInformation), requestPropertiesItem.componentName.toLatin1().constData(), JobWorker::jsonType(QJsonValue::Array));
            }
        } else {
            qFatal ("Unexpected code flow!");
        }
    } else {
        // Match the leaf as Bool, Double or String
        if (jsonValueInformation.isBool ()) {
            static QList<QStringList> booleanOptions (
                QList<QStringList> ()
                    << ( QStringList()
                        << QStringLiteral("n")
                        << QStringLiteral("no")
                        << QStringLiteral("0")
                        << QStringLiteral("false")
                        << QStringLiteral("$false")
                        << QStringLiteral("off")
                    ) << ( QStringList()
                        << QStringLiteral("y")
                        << QStringLiteral("yes")
                        << QStringLiteral("1")
                        << QStringLiteral("true")
                        << QStringLiteral("$true")
                        << QStringLiteral("on")
                    )
            );
            if (requestMathMatchOperator == OperatorString ||
                requestMathMatchOperator == OperatorEquals ||
                requestMathMatchOperator == OperatorNotEquals) {
                int booleanOptionsPos = ((jsonValueInformation.toBool ()) ? 1 : 0);
                if (requestMathMatchOperator == OperatorNotEquals) {
                    booleanOptionsPos = 1 - booleanOptionsPos;
                }
                for (QStringList::const_iterator requestCriteriaIterator = requestCriteria.constBegin(); requestCriteriaIterator != requestCriteria.constEnd(); requestCriteriaIterator++) {
                    if (booleanOptions[booleanOptionsPos].contains ((*requestCriteriaIterator), Qt::CaseInsensitive)) {
                        return (true);
                    } else if (! booleanOptions[1 - booleanOptionsPos].contains ((*requestCriteriaIterator), Qt::CaseInsensitive)) {
                        qWarning ("[%s#%u] Unable to evaluate '%s' as a boolean value.", requestHelperName.toLatin1().constData(), _requestId, requestCriteriaIterator->toLatin1().constData());
                        break;
                    }
                }
                return (false);
            } else {
                qInfo ("[%s#%u] Unable to apply selected comparison operator on a boolean value.", requestHelperName.toLatin1().constData(), _requestId);
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
                        static const quint64 acceptableError = (1u << 8);
                        if ((requestMathMatchOperator == OperatorLessThan            && ( doubleValue < doubleComparison)) ||
                            (requestMathMatchOperator == OperatorLessThanOrEquals    && ((doubleValue < doubleComparison)  || qFloatDistance (doubleValue, doubleComparison) < acceptableError)) ||
                            (requestMathMatchOperator == OperatorEquals              && (qFloatDistance (doubleValue, doubleComparison) <  acceptableError)) ||
                            (requestMathMatchOperator == OperatorNotEquals           && (qFloatDistance (doubleValue, doubleComparison) >= acceptableError)) ||
                            (requestMathMatchOperator == OperatorGreaterThanOrEquals && ((doubleValue > doubleComparison)  || qFloatDistance (doubleValue, doubleComparison) < acceptableError)) ||
                            (requestMathMatchOperator == OperatorGreaterThan         && ( doubleValue > doubleComparison))) {
                            return (true);
                        }
                    } else {
                        qWarning ("[%s#%u] Unable to evaluate '%s' as a numeric value.", requestHelperName.toLatin1().constData(), _requestId, requestCriteriaIterator->toLatin1().constData());
                        return (false);
                    }
                }
            } else {
                qInfo ("[%s#%u] Unable to apply selected comparison operator on a numeric value!", requestHelperName.toLatin1().constData(), _requestId);
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
                            qWarning ("[%s#%u] Expression specification '%s' could not be parsed: '%s'", requestHelperName.toLatin1().constData(), _requestId, requestCriteriaIterator->toLatin1().constData(), regexComparison.errorString().toLatin1().constData());
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
                qInfo ("[%s#%u] Unable to apply selected comparison operator on a string value!", requestHelperName.toLatin1().constData(), _requestId);
            }
        } else {
            qInfo ("[%s#%u] Unexpected JSON type '%s' while evaluating the leaf.", requestHelperName.toLatin1().constData(), _requestId, JobWorker::jsonType(jsonValueInformation));
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

JobWorker::JobWorker (const QString& _requestChannel, QObject* parent) :
    QObject (parent),
    finishRequested (false),
    rngdInitialized (false),
    requestChannel (_requestChannel),
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
        appHelperInfo->databaseCache = new ObjectCacheDatabase (appHelperInfo->name.toLocal8Bit(), AppRuntime::dbTblPrefix.toLocal8Bit());
        appHelperInfo->memoryCache = new ObjectCacheMemory (appHelperInfo->name.toLocal8Bit(), (*(appHelperInfo->databaseCache)));
        appHelperInfo->entryPoint = javascriptBridge->makeEntryPoint (helperNamesPos);
        if (appHelperInfo->entryPoint.isCallable ()) {
            this->javascriptBridge->invokeMethod (appHelperInfo->entryPoint, static_cast<unsigned> (this->helperInstances.count()), JavascriptMethod::getSupportedUrls);
        }
    }
}

JobWorker::~JobWorker () {
    if ((! (this->runningRequests.isEmpty() && this->finishRequested && this->incomingRequests.isEmpty ())) || this->retryTimer->isActive ()) {
        qFatal ("Program tried to destruct a JobWorker unexpectedly!");
    }
    this->runtimeEnvironment->collectGarbage ();
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
    if (method == QStringLiteral("getSupportedUrls")) {
        this->processSupportedUrls (static_cast<signed> (context), returnedValue);
    } else if (method == QStringLiteral("getObjectFromUrl")) {
        this->processObjectFromUrl (context, returnedValue);
    } else if (method == QStringLiteral("getPropertiesFromObject")) {
        this->processPropertiesFromObject (context, returnedValue);
    } else if (method == QStringLiteral("getPropertiesFromObjectCache")) {
        QJSValue callback (returnedValue.property(QStringLiteral("callback")));
        if (callback.isCallable() && context < (static_cast<unsigned> (this->helperInstances.count()))) {
            AppJobRequestFromHelper* helperRequest = new AppJobRequestFromHelper ();
            helperRequest->data.callback = callback;
            helperRequest->data.object = returnedValue.property(QStringLiteral("object"));
            helperRequest->helper.id = static_cast<signed> (context);
            helperRequest->helper.name = this->helperInstances[static_cast<signed> (context)]->name;
            this->incomingRequests.append (helperRequest);
            this->processIncomingRequest ();
        }
    } else {
        qCritical ("Javascript returned value from an unexpected method invocation: (context=%u, method='%s', returnedValue='%s')", context, method.toLatin1().constData(), JavascriptBridge::QJS2QString(returnedValue).toLatin1().constData());
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
            } else {
                qFatal ("Unexpected code flow!");
            }
        } else {
            appHelperInfo = this->helperInstances[request->helper.id];
        }
        if (request->helper.name.isEmpty ()) {
            if (requestType == RequestFromSquid) {
                if (squidRequest->helper.isOnProgress) {
                    squidRequest->helper.isOnProgress = false;
                    squidRequest->helper.id = 0;
                    this->incomingRequests.prepend (squidRequest);
                    this->startRetryTimer ();
                } else {
                    QMap<unsigned int,AppJobRequest*>::iterator end(this->runningRequests.end());
                    if (squidRequest->data.invertedMatch) {
                        this->squidResponseOut (end, squidRequest, QStringLiteral("Unable to find a helper that can handle the requested URL; sending a MATCH response as fallback"), true);
                    } else {
                        this->squidResponseOut (end, squidRequest, QStringLiteral("Unable to find a helper that can handle the requested URL; sending a NOMATCH response as fallback"), false);
                    }
                }
            } else {
                qFatal ("Unexpected code flow!");
            }
        } else {
            unsigned int _requestId (this->requestId);
            this->requestId += 2;
            QMap<unsigned int,AppJobRequest*>::iterator requestIdIterator (this->runningRequests.insert (_requestId, request));
            if (requestType == RequestFromHelper) {
                this->processObjectFromUrl (_requestId, helperRequest->data.object);
            } else if (requestType == RequestFromSquid) {
                qDebug ("[%s#%u] Invoking 'getObjectFromUrl();'...", appHelperInfo->name.toLatin1().constData(), _requestId);
                if (! this->javascriptBridge->invokeMethod (appHelperInfo->entryPoint, _requestId, JavascriptMethod::getObjectFromUrl, urlString)) {
                    this->tryNextHelper (requestIdIterator);
                }
            } else {
                qFatal ("Unexpected code flow!");
            }
        }
    } else {
        qFatal ("Invalid procedure call: 'AppJobRequest*' could not be downcasted to 'AppJobRequestFromHelper*' nor to 'AppJobRequestFromSquid*'!");
    }
}

void JobWorker::setCurrentTimestamp () {
    // I need to update this->currentTimestamp manually if the JobWorker stops receiving jobs from JobDispatcher
    // probably because of an EOF at STDIN or an absence of requests from Squid.
    qint64 _currentTimestamp (AppRuntime::currentDateTime().toMSecsSinceEpoch());
    if (! this->rngdInitialized) {
        qsrand (static_cast <uint> (_currentTimestamp));
        QThread::msleep (1);
        this->rngdInitialized = true;
    }
    this->currentTimestamp = (_currentTimestamp / 1000);
}

void JobWorker::squidRequestIn (AppJobRequestFromSquid* request, const qint64 timestampNow) {
    qDebug ("JobDispatcher has sent an ACL matching request to channel #%s.", this->requestChannel.toLatin1().constData());
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
        qInfo ("Finished handler for channel #%s.", this->requestChannel.toLatin1().constData());
        emit finished ();
    } else if (numPendingRequests == 1) {
        qInfo ("A finish request was received by handler for channel #%s, but there is a pending answer.", this->requestChannel.toLatin1().constData());
    } else {
        qInfo ("A finish request was received by handler for channel #%s, but there are %d pending answers.", this->requestChannel.toLatin1().constData(), numPendingRequests);
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
        qFatal ("Program tried to destruct a JobCarrier unexpectedly!");
    }
    delete (this->threadObj);
}

bool JobCarrier::start (QThread::Priority priority) {
    if (this->started) {
        qFatal ("Invalid procedure call: this method must be called only once!");
    } else {
        this->threadObj->start (priority);
        this->started = this->threadObj->isRunning ();
        return (this->started);
    }
}

void JobCarrier::squidRequestIn (AppJobRequestFromSquid* request, const qint64 timestampNow) {
    emit squidRequestOut (request, timestampNow);
}
