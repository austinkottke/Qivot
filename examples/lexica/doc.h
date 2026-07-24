#ifndef DOC_H
#define DOC_H
#include <qivot.h>

/// One searchable entry in the corpus. `title` and `body` are the text we build a
/// full-text index over; `category` is just for display (a colored pill).
class Doc : public QiModel {
    QI_MODEL
public:
    QiField<QString> title;
    QiField<QString> body;
    QiField<QString> category;
};
QI_DECLARE_MODEL(Doc, "doc",
    QI_FIELD(title), QI_FIELD(body), QI_FIELD(category));

#endif // DOC_H
