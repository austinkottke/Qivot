#ifndef USER_H
#define USER_H

/// Example model - user table

#include "qimodel.h"

class User : public QiModel
{
    QI_MODEL
public:
    QiField<QString> userId;
    QiField<QString> name;

    QiField<QString> passwd;

    QiField<QDateTime> creationTime;
    QiField<QDateTime> lastLoginTime;

    virtual bool clean() {
        QString pw = passwd->toString();
        if (pw.size() < 8){ // passwd is too short.
            setError("password must be at least 8 characters");
            return false;
        }
        return true;
    }
};

QI_DECLARE_MODEL( User,
                  "user",
                  QI_FIELD(userId, QiUnique | QiNotNull),
                  QI_FIELD(name),
                  QI_FIELD(passwd , QiNotNull),
                  QI_FIELD(creationTime,QiDefault("CURRENT_TIMESTAMP") ),
                  QI_FIELD(lastLoginTime)
                  );


#endif // USER_H
