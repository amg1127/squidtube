#ifndef JOBWORKER_H
#define JOBWORKER_H

#include <QNetworkAccessManager>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QThread>

class jobWork {
public:
    QString requestChannel;
    QString requestProperty;
    Qt::CaseSensitivity requestCaseSensivity;
    QRegExp::PatternSyntax requestPatternSyntax;
    QStringList requestTokens;
};
Q_DECLARE_METATYPE (jobWork)

class jobWorker : public QThread {
    Q_OBJECT
private:
    QNetworkAccessManager networkAccessManager;
private slots:
    void jobConsumer (const jobWork& job);
public:
    jobWorker();
    void addJob (const jobWork& job);
signals:
    void jobAdded (const jobWork& job);
#warning A signal that returns job result have to be declared.
};

#endif // JOBWORKER_H
