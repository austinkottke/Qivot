#ifndef ITEM_H
#define ITEM_H
#include <qivot.h>

/// One card in the infinite feed. All of this is real, DB-backed data —
/// the gradient colors, title, category and metric are stored per row and
/// streamed to the UI a page at a time.
class Item : public QiModel {
    QI_MODEL
public:
    QiField<QString> title;
    QiField<QString> subtitle;   // category
    QiField<QString> colorA;     // gradient start (hex)
    QiField<QString> colorB;     // gradient end   (hex)
    QiField<int>     metric;     // a number to show on the card
};
QI_DECLARE_MODEL(Item, "item",
    QI_FIELD(title), QI_FIELD(subtitle),
    QI_FIELD(colorA), QI_FIELD(colorB), QI_FIELD(metric));

#endif // ITEM_H
