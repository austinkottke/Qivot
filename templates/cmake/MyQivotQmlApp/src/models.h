#pragma once

#include <qivot.h>
#include <qigadget.h>
#include <QString>
#include <QDateTime>

// Define models with Q_GADGET for QML binding
// See https://austinkottke.github.io/Qivot/ for documentation

class User : public QiModel {
    Q_GADGET
    QI_MODEL
public:
    QI_QML_FIELD(QString, name)
    QI_QML_FIELD(QString, email)
};
QI_DECLARE_MODEL(User, "user",
                 QI_FIELD(name),
                 QI_FIELD(email, QiUnique | QiNotNull));
Q_DECLARE_METATYPE(User)


class Task : public QiModel {
    Q_GADGET
    QI_MODEL
public:
    QI_QML_FIELD(QString, title)
    QI_QML_FIELD(int, priority)
    QI_QML_FIELD(bool, done)
};
QI_DECLARE_MODEL(Task, "task",
                 QI_FIELD(title, QiNotNull),
                 QI_FIELD(priority, QiDefault(1)),
                 QI_FIELD(done, QiDefault(0)));
Q_DECLARE_METATYPE(Task)
