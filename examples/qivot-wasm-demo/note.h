#ifndef NOTE_H
#define NOTE_H
#include <qivot.h>
#include <qigadget.h>

/// One sticky note. Q_GADGET so its fields are readable straight from QML.
class Note : public QiModel {
    Q_GADGET
    QI_MODEL
    QI_QML_FIELD(QString, text)
    QI_QML_FIELD(QString, color)
    QI_QML_FIELD(QString, createdAt)
};
QI_DECLARE_MODEL(Note, "note",
    QI_FIELD(text), QI_FIELD(color), QI_FIELD(createdAt));

#endif // NOTE_H
