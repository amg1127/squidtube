#include "jobworker.h"

jobWorker::jobWorker() {
    this->networkAccessManager.moveToThread (this);
    QObject::connect (this, SIGNAL(jobAdded(jobWork)), this, SLOT(jobConsumer(jobWork)));
}

void jobWorker::run () {
    qDebug() << "stdinReader thread started.";
    this->exec ();
    qDebug() << "stdinReader thread finished.";
}

void jobWorker::addJob (const jobWork& job) {
    emit jobAdded (job);
}

void jobWorker::jobConsumer (const jobWork& job) {
#warning Testing...
    qDebug() << job.requestChannel << job.requestCaseSensivity << job.requestPatternSyntax << job.requestProperty << job.requestTokens;
}
