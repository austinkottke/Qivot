#ifndef TASK_H
#define TASK_H
#include <qivot.h>

class Task : public QiModel {
    QI_MODEL
public:
    QiField<QString> title;
    QiField<int>     done;   // 0 / 1
};
QI_DECLARE_MODEL(Task, "task", QI_FIELD(title), QI_FIELD(done, QiDefault(0)));

#endif // TASK_H
