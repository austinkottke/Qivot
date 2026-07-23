#ifndef MODELS_H
#define MODELS_H
#include <qivot.h>

// A single tiny model — the "scratch" rows we edit inside nested transactions.
class Item : public QiModel {
    QI_MODEL
public:
    QiField<QString> label;
};
QI_DECLARE_MODEL(Item, "item", QI_FIELD(label));

#endif // MODELS_H
