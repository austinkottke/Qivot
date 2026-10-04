#ifndef VALIDATIONTESTS_H
#define VALIDATIONTESTS_H

#include <QObject>

/// QiValidation: rules on fields, per-field errors, warnings, contexts, the
/// database checks, dates, and the errors a failed save maps back to fields.
class ValidationTests : public QObject {
    Q_OBJECT
public:
    explicit ValidationTests(QObject *parent = nullptr) : QObject(parent) {}
private slots:
    void initTestCase();
    void textRules();
    void normalizing();
    void numbersAndChoices();
    void enforcedChecks();
    void conditionalAndCompare();
    void warningsAndContexts();
    void cleanAndPartial();
    void uniqueAndDatabaseErrors();
    void scopedUniqueAndExists();
    void overlappingBookings();
    void dates();
    void relativeDates();
    void calendarRules();
    void expiryAndZones();
    void lists();
    void humanize();
};

#endif // VALIDATIONTESTS_H
