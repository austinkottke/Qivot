#ifndef QiGADGET_H
#define QiGADGET_H

#include <QMetaType>
#include <qimodel.h>

/// Helper for exposing a QiModel to Qt's meta-object system (MOC) and QML.
/**
  A normal Qivot model (QI_MODEL + QiField) is deliberately *not* a QObject and
  is invisible to Qt's meta-object system. To expose a model to QML, make it a
  Q_GADGET and declare each field as a Q_PROPERTY.

  Write **`Q_GADGET`** (literally — qmake's automoc scans the source text for the
  keyword, so it must not be hidden behind a macro) followed by `QI_MODEL`, then
  use `QI_QML_FIELD` in place of each bare `QiField` member. The model must live
  in a header listed in your project's `HEADERS` so moc processes it.

\code
    class Post : public QiModel {
        Q_GADGET
        QI_MODEL
        QI_QML_FIELD(int,     remoteId)
        QI_QML_FIELD(QString, title)
    };
    QI_DECLARE_MODEL(Post, "post",
                     QI_FIELD(remoteId, QiUnique | QiNotNull),
                     QI_FIELD(title));
\endcode

  The result is a value type whose properties QML can read and write
  (`post.title`). Qivot's own save() / load() / query API keeps working
  unchanged — the gadget adds no per-instance state. For exposing a *list* of
  records to a QML view, see QiListModel.
 */

/// Declare a database field AND expose it to QML as a Q_PROPERTY
/**
  @param TYPE The field's value type (QString, int, double, QDateTime, ...).
  @param NAME The field name.

  Expands to a `QiField<TYPE> NAME;` member plus a Q_PROPERTY with read/write
  accessors that bridge the field's QVariant storage. Requires a literal
  `Q_GADGET` in the class so that moc processes the header.
 */
#define QI_QML_FIELD(TYPE, NAME) \
    Q_PROPERTY(TYPE NAME READ _qiGet_##NAME WRITE _qiSet_##NAME) \
public: \
    QiField<TYPE> NAME; \
    TYPE _qiGet_##NAME() const { return NAME; } \
    void _qiSet_##NAME(const TYPE &value) { NAME.set(QVariant::fromValue(value)); }

#endif // QiGADGET_H
