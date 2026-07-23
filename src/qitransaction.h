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

  **Nesting.** QiTransactions nest safely: the outermost issues a real `BEGIN`
  and each inner one a `SAVEPOINT`, so an inner rollback undoes only its own work
  while the outer transaction continues.

\code
    QiTransaction outer;
    a.save();
    {
        QiTransaction inner;        // SAVEPOINT
        b.save();
        inner.rollback();           // undoes only b (ROLLBACK TO savepoint)
    }
    outer.commit();                 // commits a
\endcode

  @remarks It is non-copyable.
 */
class QiTransaction {
public:
    /// Begin a (possibly nested) transaction on the given connection.
    explicit QiTransaction(QiConnection connection = QiConnection::defaultConnection())
        : m_connection(connection) , m_committed(false) {
        m_depth = m_connection.beginScope();   // 0 on failure, else the nesting depth
        m_active = (m_depth > 0);
    }

    /// Roll back the transaction if it was neither committed nor rolled back
    ~QiTransaction() {
        if (m_active && !m_committed)
            m_connection.rollbackScope(m_depth);
    }

    /// Commit the transaction (`COMMIT`, or `RELEASE` for a nested one).
    /**
      @return TRUE if the commit succeeded.
     */
    bool commit() {
        if (!m_active || m_committed)
            return false;
        m_committed = m_connection.commitScope(m_depth);
        return m_committed;
    }

    /// Roll back explicitly (`ROLLBACK`, or `ROLLBACK TO` for a nested one).
    bool rollback() {
        if (!m_active || m_committed)
            return false;
        bool res = m_connection.rollbackScope(m_depth);
        m_committed = true; // prevent the destructor from rolling back again
        return res;
    }

    /// TRUE while the transaction is open (begun, not yet committed/rolled back)
    bool isActive() const {
        return m_active && !m_committed;
    }

    /// The nesting depth (1 = outermost). 0 if the transaction failed to begin.
    int depth() const { return m_depth; }

private:
    QiTransaction(const QiTransaction &) = delete;
    QiTransaction &operator=(const QiTransaction &) = delete;

    QiConnection m_connection;
    int  m_depth = 0;
    bool m_active;
    bool m_committed;
};

#endif // QiTRANSACTION_H
