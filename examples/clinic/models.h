#ifndef MODELS_H
#define MODELS_H
#include <qivot.h>
#include <qigadget.h>   // Q_GADGET + QI_QML_FIELD — exposes fields to QML directly

// A small clinical schema. Every model is a Q_GADGET, so its fields are readable
// straight from QML (patient.firstName) and a QiListModel over a query exposes
// them as roles with no hand-mapping. All data is synthetic — no real patients.
// Dates are stored as sortable "yyyy-MM-dd" strings; times as minutes from midnight.

/// A clinician the schedule is organized around.
class Provider : public QiModel {
    Q_GADGET
    QI_MODEL
    QI_QML_FIELD(QString, name)
    QI_QML_FIELD(QString, specialty)
    QI_QML_FIELD(QString, color)      // hex, used for the schedule column
};
QI_DECLARE_MODEL(Provider, "provider",
    QI_FIELD(name), QI_FIELD(specialty), QI_FIELD(color));

/// A patient record (the "chart").
class Patient : public QiModel {
    Q_GADGET
    QI_MODEL
    QI_QML_FIELD(QString, mrn)
    QI_QML_FIELD(QString, firstName)
    QI_QML_FIELD(QString, lastName)
    QI_QML_FIELD(QString, dob)        // yyyy-MM-dd
    QI_QML_FIELD(QString, sex)
    QI_QML_FIELD(QString, phone)
    QI_QML_FIELD(QString, bloodType)
    QI_QML_FIELD(QString, allergies)  // "" = none known
};
QI_DECLARE_MODEL(Patient, "patient",
    QI_FIELD(mrn), QI_FIELD(firstName), QI_FIELD(lastName), QI_FIELD(dob),
    QI_FIELD(sex), QI_FIELD(phone), QI_FIELD(bloodType), QI_FIELD(allergies));
Q_DECLARE_METATYPE(Patient)           // so it can be a Q_PROPERTY value in QML

/// A booked visit. `patientId`/`providerId` are foreign keys (plain ints here).
class Appointment : public QiModel {
    Q_GADGET
    QI_MODEL
    QI_QML_FIELD(int,     patientId)
    QI_QML_FIELD(int,     providerId)
    QI_QML_FIELD(QString, day)        // yyyy-MM-dd
    QI_QML_FIELD(int,     minute)     // minutes from midnight (9:30 = 570)
    QI_QML_FIELD(int,     durationMin)
    QI_QML_FIELD(QString, reason)
    QI_QML_FIELD(QString, status)     // scheduled | arrived | completed | cancelled
};
QI_DECLARE_MODEL(Appointment, "appointment",
    QI_FIELD(patientId), QI_FIELD(providerId), QI_FIELD(day), QI_FIELD(minute),
    QI_FIELD(durationMin), QI_FIELD(reason), QI_FIELD(status));

/// A set of vital signs taken at a visit.
class Vital : public QiModel {
    Q_GADGET
    QI_MODEL
    QI_QML_FIELD(int,     patientId)
    QI_QML_FIELD(QString, takenOn)    // yyyy-MM-dd
    QI_QML_FIELD(int,     systolic)
    QI_QML_FIELD(int,     diastolic)
    QI_QML_FIELD(int,     heartRate)
    QI_QML_FIELD(double,  tempC)
    QI_QML_FIELD(double,  weightKg)
    QI_QML_FIELD(int,     heightCm)
    QI_QML_FIELD(int,     spo2)
};
QI_DECLARE_MODEL(Vital, "vital",
    QI_FIELD(patientId), QI_FIELD(takenOn), QI_FIELD(systolic), QI_FIELD(diastolic),
    QI_FIELD(heartRate), QI_FIELD(tempC), QI_FIELD(weightKg), QI_FIELD(heightCm), QI_FIELD(spo2));
Q_DECLARE_METATYPE(Vital)

/// An entry on the patient's problem list.
class Problem : public QiModel {
    Q_GADGET
    QI_MODEL
    QI_QML_FIELD(int,     patientId)
    QI_QML_FIELD(QString, name)
    QI_QML_FIELD(QString, status)     // active | resolved
    QI_QML_FIELD(QString, onset)      // yyyy-MM-dd
};
QI_DECLARE_MODEL(Problem, "problem",
    QI_FIELD(patientId), QI_FIELD(name), QI_FIELD(status), QI_FIELD(onset));

/// A medication on the patient's list.
class Medication : public QiModel {
    Q_GADGET
    QI_MODEL
    QI_QML_FIELD(int,     patientId)
    QI_QML_FIELD(QString, name)
    QI_QML_FIELD(QString, dose)
    QI_QML_FIELD(QString, frequency)
    QI_QML_FIELD(bool,    active)
};
QI_DECLARE_MODEL(Medication, "medication",
    QI_FIELD(patientId), QI_FIELD(name), QI_FIELD(dose), QI_FIELD(frequency), QI_FIELD(active));

/// A clinical note from a visit.
class Note : public QiModel {
    Q_GADGET
    QI_MODEL
    QI_QML_FIELD(int,     patientId)
    QI_QML_FIELD(int,     providerId)
    QI_QML_FIELD(QString, date)       // yyyy-MM-dd
    QI_QML_FIELD(QString, kind)       // Office Visit | Phone | Lab Review | ...
    QI_QML_FIELD(QString, body)
};
QI_DECLARE_MODEL(Note, "note",
    QI_FIELD(patientId), QI_FIELD(providerId), QI_FIELD(date), QI_FIELD(kind), QI_FIELD(body));

#endif // MODELS_H
