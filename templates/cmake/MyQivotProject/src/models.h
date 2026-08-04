#pragma once

#include <qivot.h>
#include <QString>
#include <QDateTime>

// Define your Qivot models here
// See https://austinkottke.github.io/Qivot/ for documentation

class User : public QiModel {
    QI_MODEL
public:
    QiField<QString> name;
    QiField<QString> email;
};
QI_DECLARE_MODEL(User, "user",
                 QI_FIELD(name),
                 QI_FIELD(email, QiUnique | QiNotNull));


class Task : public QiModel {
    QI_MODEL
public:
    QiField<QString> title;
    QiField<int> priority;
    QiField<bool> done;
};
QI_DECLARE_MODEL(Task, "task",
                 QI_FIELD(title, QiNotNull),
                 QI_FIELD(priority, QiDefault(1)),
                 QI_FIELD(done, QiDefault(0)));
