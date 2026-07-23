#ifndef MODELS_H
#define MODELS_H
#include <qivot.h>

// A classic one-to-many: an Author has many Books.
class Author : public QiModel {
    QI_MODEL
public:
    QiField<QString> name;
};
QI_DECLARE_MODEL(Author, "author", QI_FIELD(name));

class Book : public QiModel {
    QI_MODEL
public:
    QiForeignKey<Author> author;   // FK -> author(id)
    QiField<QString>     title;
};
QI_DECLARE_MODEL(Book, "book", QI_FIELD(author), QI_FIELD(title));

#endif // MODELS_H
