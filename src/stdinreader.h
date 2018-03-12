#ifndef STDINREADER_H
#define STDINREADER_H

#include "appruntime.h"

#include <iostream>
#include <string>

#include <QStringList>
#include <QtDebug>
#include <QThread>

class stdinReader : public QThread {
    Q_OBJECT
protected:
    void run () override;
signals:
    void squidRequest (const QStringList& tokens);
};

#endif // STDINREADER_H
