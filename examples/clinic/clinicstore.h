#ifndef CLINICSTORE_H
#define CLINICSTORE_H

#include <QObject>
#include <QAbstractItemModel>
#include <QQmlEngine>        // QML_ELEMENT
#include <QVariantMap>
#include <QHash>
#include <QList>
#include <qilistmodel.h>
#include "models.h"

/// The one controller QML talks to. Query results are exposed as QiListModels
/// (roles derived straight from the model's fields — no hand-mapping), and the
/// selected chart is exposed as raw gadget values (patient.firstName in QML).
/// Joins and display formatting are small invokables, not copied onto every row.
class ClinicStore : public QObject {
    Q_OBJECT

    // --- list models (bind directly to ListView / Repeater / ComboBox) ---
    Q_PROPERTY(QAbstractItemModel *patients     READ patients     CONSTANT)
    Q_PROPERTY(QAbstractItemModel *providers    READ providers    CONSTANT)
    Q_PROPERTY(QAbstractItemModel *problems     READ problems     CONSTANT)
    Q_PROPERTY(QAbstractItemModel *medications  READ medications  CONSTANT)
    Q_PROPERTY(QAbstractItemModel *vitals       READ vitals       CONSTANT)
    Q_PROPERTY(QAbstractItemModel *appointments READ appointments CONSTANT)
    Q_PROPERTY(QAbstractItemModel *notes        READ notes        CONSTANT)
    Q_PROPERTY(QAbstractItemModel *schedule     READ schedule     CONSTANT)
    Q_PROPERTY(QAbstractItemModel *noteResults  READ noteResults  CONSTANT)

    // --- the selected chart, as raw gadget values ---
    Q_PROPERTY(Patient patient     READ patient     NOTIFY chartChanged)
    Q_PROPERTY(Vital   latestVital READ latestVital NOTIFY chartChanged)
    Q_PROPERTY(int     selectedId  READ selectedId  NOTIFY chartChanged)

    // --- scheduler ---
    Q_PROPERTY(QString     scheduleDay   READ scheduleDay   NOTIFY scheduleChanged)
    Q_PROPERTY(QString     scheduleLabel READ scheduleLabel NOTIFY scheduleChanged)
    Q_PROPERTY(bool        isToday       READ isToday       NOTIFY scheduleChanged)
    Q_PROPERTY(QVariantMap stats         READ stats         NOTIFY scheduleChanged)
    Q_PROPERTY(QString     lastError     READ lastError     NOTIFY errorChanged)

    // --- analytics + note search ---
    Q_PROPERTY(QVariantMap overview  READ overview  NOTIFY overviewChanged)
    Q_PROPERTY(QString     noteQuery READ noteQuery NOTIFY noteResultsChanged)

public:
    explicit ClinicStore(QObject *parent = nullptr);

    QAbstractItemModel *patients()     { return &m_patients; }
    QAbstractItemModel *providers()    { return &m_providers; }
    QAbstractItemModel *problems()     { return &m_problems; }
    QAbstractItemModel *medications()  { return &m_medications; }
    QAbstractItemModel *vitals()       { return &m_vitals; }
    QAbstractItemModel *appointments() { return &m_appointments; }
    QAbstractItemModel *notes()        { return &m_notes; }
    QAbstractItemModel *schedule()     { return &m_schedule; }
    QAbstractItemModel *noteResults()  { return &m_noteResults; }

    Patient patient() const     { return m_patient; }
    Vital   latestVital() const { return m_latestVital; }
    int     selectedId() const  { return m_selectedId; }
    QString scheduleDay() const { return m_day; }
    QString scheduleLabel() const;
    bool    isToday() const;
    QVariantMap stats() const   { return m_stats; }
    QString lastError() const   { return m_lastError; }
    QVariantMap overview() const { return m_overview; }
    QString noteQuery() const   { return m_noteQuery; }

    // --- actions ---
    Q_INVOKABLE void search(const QString &text);
    Q_INVOKABLE void selectPatient(int id);
    Q_INVOKABLE void setScheduleDay(const QString &iso);
    Q_INVOKABLE void shiftDay(int deltaDays);
    Q_INVOKABLE void goToday();
    Q_INVOKABLE bool book(int patientId, int providerId, int minute,
                          int durationMin, const QString &reason);
    Q_INVOKABLE void setStatus(int appointmentId, int status);   // status = Clinic::ApptStatus
    Q_INVOKABLE void searchNotes(const QString &text);
    Q_INVOKABLE void addNote(int patientId, int providerId, int kind, const QString &body); // kind = Clinic::NoteKind
    Q_INVOKABLE void addVital(int patientId, int systolic, int diastolic, int heartRate,
                              double tempC, int spo2, double weightKg, int heightCm);
    Q_INVOKABLE int  addPatient(const QString &first, const QString &last, const QString &dob,
                                const QString &sex, const QString &phone);

    // --- tiny joins / formatters for QML (kept off the row data) ---
    Q_INVOKABLE QString providerName(int id) const  { return m_provName.value(id, "—"); }
    Q_INVOKABLE QString providerColor(int id) const { return m_provColor.value(id, "#94A3B8"); }
    Q_INVOKABLE int     providerIndex(int id) const { return m_provIndex.value(id, 0); }
    Q_INVOKABLE QString patientName(int id) const   { return m_patName.value(id, "—"); }
    Q_INVOKABLE int     ageOf(const QString &dob) const;
    Q_INVOKABLE QString minuteLabel(int minutesFromMidnight) const;
    Q_INVOKABLE QString todayIso() const;
    Q_INVOKABLE QString statusLabel(int apptStatus) const;   // Clinic::ApptStatus -> text
    Q_INVOKABLE QString kindLabel(int noteKind) const;       // Clinic::NoteKind  -> text

signals:
    void patientsChanged();
    void chartChanged();
    void scheduleChanged();
    void errorChanged();
    void overviewChanged();
    void noteResultsChanged();

private:
    void loadCaches();       // provider + patient-name lookups
    void refreshPatients();
    void refreshSchedule();
    void rebuildOverview();

    QiListModel m_patients, m_providers, m_problems, m_medications, m_vitals,
                m_appointments, m_notes, m_schedule, m_noteResults;

    Patient m_patient;
    Vital   m_latestVital;
    int     m_selectedId = -1;
    QString m_query, m_day, m_noteQuery, m_lastError;
    QVariantMap m_stats, m_overview;

    // lookup caches (built once / on write)
    QHash<int, QString> m_provName, m_provColor, m_patName;
    QHash<int, int>     m_provIndex;
    QList<int>          m_provIds;      // provider ids in display order
};

#endif // CLINICSTORE_H
