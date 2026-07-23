#ifndef CONTACT_H
#define CONTACT_H
#include <qivot.h>

class Contact : public QiModel {
    QI_MODEL
public:
    QiField<QString> firstName;
    QiField<QString> lastName;
    QiField<QString> phone;
};
QI_DECLARE_MODEL(Contact, "contact",
                 QI_FIELD(firstName), QI_FIELD(lastName), QI_FIELD(phone));

#endif // CONTACT_H
