#ifndef QiTRANSACTION_H
#define QiTRANSACTION_H

#include <qiconnection.h>

/// RAII scope guard for a database transaction
/**
  QiTransaction begins a transaction on construction and, unless you commit(),
  rolls it back automatically when it goes out of scope. This makes a batch of
  writes atomic and much faster (SQLite otherwise fsyncs after every statement).

\code
    QiTransaction transaction;      // BEGIN

    for (Order &order : orders)
        order.save();

    transaction.commit();           // COMMIT (rolled back automatically if we
                                    //         return / throw before this)
\endcode

  @remarks It is non-copyable.
 */
class QiTransaction {
public:
    /// Begin a transaction on the given connection (default: the default connection)
    explicit QiTransaction(QiConnection connection = QiConnection::defaultConnection())
        : m_connection(connection) , m_committed(false) {
        m_active = m_connection.transaction();
    }

    /// Roll back the transaction if it was neither committed nor rolled back
    ~QiTransaction() {
        if (m_active && !m_committed)
            m_connection.rollback();
    }

    /// Commit the transaction
    /**
      @return TRUE if the commit succeeded.
     */
    bool commit() {
        if (!m_active || m_committed)
            return false;
        m_committed = m_connection.commit();
        return m_committed;
    }

    /// Roll back the transaction explicitly
    bool rollback() {
        if (!m_active || m_committed)
            return false;
        bool res = m_connection.rollback();
        m_committed = true; // prevent the destructor from rolling back again
        return res;
    }

    /// TRUE while the transaction is open (begun, not yet committed/rolled back)
    bool isActive() const {
        return m_active && !m_committed;
    }

private:
    QiTransaction(const QiTransaction &) = delete;
    QiTransaction &operator=(const QiTransaction &) = delete;

    QiConnection m_connection;
    bool m_active;
    bool m_committed;
};

#endif // QiTRANSACTION_H
