#ifndef POST_H
#define POST_H

#include <qivot.h>
#include <qigadget.h>

/// A model exposed to Qt's meta-object system / QML.
/**
  `Q_GADGET` (written literally so qmake's automoc processes this header) makes
  the model introspectable; `QI_QML_FIELD` declares each field as a Q_PROPERTY.
  Everything else — save(), load(), queries — works exactly as for a plain model.
 */
class Post : public QiModel {
    Q_GADGET
    QI_MODEL
    QI_QML_FIELD(int,     remoteId)
    QI_QML_FIELD(QString, title)
    QI_QML_FIELD(QString, author)
};

QI_DECLARE_MODEL(Post, "post",
                 QI_FIELD(remoteId, QiUnique | QiNotNull),
                 QI_FIELD(title),
                 QI_FIELD(author));

#endif // POST_H
