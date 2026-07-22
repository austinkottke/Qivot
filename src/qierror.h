#ifndef QiERROR_H
#define QiERROR_H

#include <QString>

/// A database error: a category and a human-readable message.
/**
  Qivot operations return `bool` (success/failure). When one fails, the reason is
  available from `lastError()` on the object involved — `QiModel::lastError()`
  after save()/load()/remove()/upsert(), or `QiConnection::lastError()` after a
  connection-level operation.

\code
    if (!user.save())
        qWarning() << user.lastError().text();   // e.g. "UNIQUE constraint failed: user.userId"
\endcode
 */
class QiError {
public:
    /// The category of error
    enum Type {
        NoError = 0,      ///< No error occurred
        ConnectionError,  ///< The connection is not open, or the driver is unsupported
        StatementError,   ///< A SQL statement failed to prepare or execute
        ValidationError,  ///< clean() rejected the record before writing
        NotFound,         ///< A requested record was not found
        NotSupported      ///< The operation is not supported
    };

    /// Construct a "no error"
    QiError() : m_type(NoError) {
    }

    /// Construct an error of the given type with a message
    QiError(Type type, const QString &text) : m_type(type) , m_text(text) {
    }

    /// The error category
    Type type() const { return m_type; }

    /// The human-readable message
    QString text() const { return m_text; }

    /// TRUE if this represents an actual error (type() != NoError)
    bool isValid() const { return m_type != NoError; }

    /// Reset to "no error"
    void clear() { m_type = NoError; m_text.clear(); }

private:
    Type m_type;
    QString m_text;
};

#endif // QiERROR_H
