#ifndef MODELS_H
#define MODELS_H
#include <qivot.h>

// A tiny sales schema: each Customer has many Sales.
class Customer : public QiModel {
    QI_MODEL
public:
    QiField<QString> name;
};
QI_DECLARE_MODEL(Customer, "customer", QI_FIELD(name));

class Sale : public QiModel {
    QI_MODEL
public:
    QiForeignKey<Customer> customer;    // FK -> customer(id)
    QiField<int>           amount;      // whole dollars
};
QI_DECLARE_MODEL(Sale, "sale", QI_FIELD(customer), QI_FIELD(amount));

// A "view model" — not a table. It carries the window-function query results as
// typed objects (qiRawQuery maps result columns onto these fields by name).
class LeaderRow : public QiModel {
    QI_MODEL
public:
    QiField<QString> name;
    QiField<int>     total;       // SUM(amount) for the customer
    QiField<int>     orders;      // COUNT(*)
    QiField<int>     rnk;         // RANK() OVER (ORDER BY total DESC)
    QiField<int>     running;     // running SUM(total) OVER (...)
};
QI_DECLARE_MODEL(LeaderRow, "leaderrow",
                 QI_FIELD(name), QI_FIELD(total), QI_FIELD(orders),
                 QI_FIELD(rnk), QI_FIELD(running));

#endif // MODELS_H
