#ifndef CLINICSTORE_H
#define CLINICSTORE_H

#include <QObject>
#include <QQmlEngine>        // QML_ELEMENT
#include <QVariantList>
#include <QVariantMap>
#include <QString>

/// The one controller QML talks to. It runs Qivot queries and exposes the
/// results as plain QVariant lists/maps (so joined + computed fields — patient
/// names, ages, time labels — are easy to bind in QML).
class ClinicStore : public QObject {
    Q_OBJECT
    QML_ELEMENT

    // patient directory (left list) + selected chart
    Q_PROPERTY(QVariantList patients     READ patients     NOTIFY patientsChanged)
    Q_PROPERTY(QVariantMap  patient      READ patient      NOTIFY chartChanged)
    Q_PROPERTY(QVariantList problems     READ problems     NOTIFY chartChanged)
    Q_PROPERTY(QVariantList medications  READ medications  NOTIFY chartChanged)
    Q_PROPERTY(QVariantList vitals       READ vitals       NOTIFY chartChanged)
    Q_PROPERTY(QVariantList appointments READ appointments NOTIFY chartChanged)
    Q_PROPERTY(QVariantList notes        READ notes        NOTIFY chartChanged)
    Q_PROPERTY(int          selectedId   READ selectedId   NOTIFY chartChanged)

    // scheduler
    Q_PROPERTY(QVariantList providers    READ providers    CONSTANT)
    Q_PROPERTY(QVariantList schedule     READ schedule     NOTIFY scheduleChanged)
    Q_PROPERTY(QString      scheduleDay  READ scheduleDay  NOTIFY scheduleChanged)
    Q_PROPERTY(QString      scheduleLabel READ scheduleLabel NOTIFY scheduleChanged)
    Q_PROPERTY(bool         isToday      READ isToday      NOTIFY scheduleChanged)
    Q_PROPERTY(QVariantMap  stats        READ stats        NOTIFY scheduleChanged)
    Q_PROPERTY(QString      lastError    READ lastError    NOTIFY errorChanged)

    // analytics + full-text note search
    Q_PROPERTY(QVariantMap  overview     READ overview     NOTIFY overviewChanged)
    Q_PROPERTY(QVariantList noteResults  READ noteResults  NOTIFY noteResultsChanged)
    Q_PROPERTY(QString      noteQuery    READ noteQuery    NOTIFY noteResultsChanged)

public:
    explicit ClinicStore(QObject *parent = nullptr);

    QVariantList patients() const     { return m_patients; }
    QVariantMap  patient() const      { return m_patient; }
    QVariantList problems() const     { return m_problems; }
    QVariantList medications() const  { return m_medications; }
    QVariantList vitals() const       { return m_vitals; }
    QVariantList appointments() const { return m_appointments; }
    QVariantList notes() const        { return m_notes; }
    int          selectedId() const   { return m_selectedId; }
    QVariantList providers() const    { return m_providers; }
    QVariantList schedule() const     { return m_schedule; }
    QString      scheduleDay() const  { return m_day; }
    QString      scheduleLabel() const;
    bool         isToday() const;
    QVariantMap  stats() const        { return m_stats; }
    QString      lastError() const    { return m_lastError; }
    QVariantMap  overview() const     { return m_overview; }
    QVariantList noteResults() const  { return m_noteResults; }
    QString      noteQuery() const    { return m_noteQuery; }

    // --- actions QML calls ---
    Q_INVOKABLE void search(const QString &text);      // filter the patient list
    Q_INVOKABLE void selectPatient(int id);            // load a chart
    Q_INVOKABLE void setScheduleDay(const QString &iso);
    Q_INVOKABLE void shiftDay(int deltaDays);          // ‹ / › day navigation
    Q_INVOKABLE void goToday();

    /// Book a visit. Runs inside a transaction and refuses to double-book a
    /// provider (overlapping time) — returns false and sets lastError if it would.
    Q_INVOKABLE bool book(int patientId, int providerId, int minute,
                          int durationMin, const QString &reason);

    Q_INVOKABLE void setStatus(int appointmentId, const QString &status); // arrive/complete/cancel

    // full-text search over clinical notes (FTS5)
    Q_INVOKABLE void searchNotes(const QString &text);

    // write operations from the UI
    Q_INVOKABLE void addNote(int patientId, int providerId, const QString &kind, const QString &body);
    Q_INVOKABLE void addVital(int patientId, int systolic, int diastolic, int heartRate,
                              double tempC, int spo2, double weightKg, int heightCm);
    Q_INVOKABLE int  addPatient(const QString &first, const QString &last, const QString &dob,
                                const QString &sex, const QString &phone);

    // helpers usable from QML
    Q_INVOKABLE QString minuteLabel(int minutesFromMidnight) const;
    Q_INVOKABLE QString todayIso() const;

signals:
    void patientsChanged();
    void chartChanged();
    void scheduleChanged();
    void errorChanged();
    void overviewChanged();
    void noteResultsChanged();

private:
    void rebuildPatients();
    void rebuildChart();
    void rebuildSchedule();
    void rebuildOverview();
    QVariantMap providerById(int id) const;
    QString patientName(int id) const;
    static int ageFromDob(const QString &dob);

    QString m_query;      // current patient search text
    int     m_selectedId = -1;
    QString m_day;        // yyyy-MM-dd currently shown on the schedule

    QVariantList m_patients, m_providers, m_schedule;
    QVariantList m_problems, m_medications, m_vitals, m_appointments, m_notes;
    QVariantMap  m_patient, m_stats, m_overview;
    QVariantList m_noteResults;
    QString      m_noteQuery;
    QString      m_lastError;
};

#endif // CLINICSTORE_H
