#ifndef MODELS_H
#define MODELS_H
#include <qivot.h>

// A small but realistic clinical schema. All data is synthetic (generated at
// startup) — no real patients. Dates are stored as sortable "yyyy-MM-dd" strings
// and times as "minutes from midnight", which keeps day/agenda queries trivial.

/// A clinician the schedule is organized around.
class Provider : public QiModel {
    QI_MODEL
public:
    QiField<QString> name;
    QiField<QString> specialty;
    QiField<QString> color;      // hex, used for the schedule column
};
QI_DECLARE_MODEL(Provider, "provider",
    QI_FIELD(name), QI_FIELD(specialty), QI_FIELD(color));

/// A patient record (the "chart").
class Patient : public QiModel {
    QI_MODEL
public:
    QiField<QString> mrn;        // medical record number
    QiField<QString> firstName;
    QiField<QString> lastName;
    QiField<QString> dob;        // yyyy-MM-dd
    QiField<QString> sex;
    QiField<QString> phone;
    QiField<QString> bloodType;
    QiField<QString> allergies;  // "" = none known
};
QI_DECLARE_MODEL(Patient, "patient",
    QI_FIELD(mrn), QI_FIELD(firstName), QI_FIELD(lastName), QI_FIELD(dob),
    QI_FIELD(sex), QI_FIELD(phone), QI_FIELD(bloodType), QI_FIELD(allergies));

/// A booked visit. `patientId`/`providerId` are foreign keys (plain ints here).
class Appointment : public QiModel {
    QI_MODEL
public:
    QiField<int>     patientId;
    QiField<int>     providerId;
    QiField<QString> day;         // yyyy-MM-dd
    QiField<int>     minute;      // minutes from midnight (e.g. 9:30 = 570)
    QiField<int>     durationMin;
    QiField<QString> reason;
    QiField<QString> status;      // scheduled | arrived | completed | cancelled
};
QI_DECLARE_MODEL(Appointment, "appointment",
    QI_FIELD(patientId), QI_FIELD(providerId), QI_FIELD(day), QI_FIELD(minute),
    QI_FIELD(durationMin), QI_FIELD(reason), QI_FIELD(status));

/// A set of vital signs taken at a visit.
class Vital : public QiModel {
    QI_MODEL
public:
    QiField<int>     patientId;
    QiField<QString> takenOn;     // yyyy-MM-dd
    QiField<int>     systolic;
    QiField<int>     diastolic;
    QiField<int>     heartRate;
    QiField<double>  tempC;
    QiField<double>  weightKg;
    QiField<int>     heightCm;
    QiField<int>     spo2;
};
QI_DECLARE_MODEL(Vital, "vital",
    QI_FIELD(patientId), QI_FIELD(takenOn), QI_FIELD(systolic), QI_FIELD(diastolic),
    QI_FIELD(heartRate), QI_FIELD(tempC), QI_FIELD(weightKg), QI_FIELD(heightCm), QI_FIELD(spo2));

/// An entry on the patient's problem list.
class Problem : public QiModel {
    QI_MODEL
public:
    QiField<int>     patientId;
    QiField<QString> name;
    QiField<QString> status;      // active | resolved
    QiField<QString> onset;       // yyyy-MM-dd
};
QI_DECLARE_MODEL(Problem, "problem",
    QI_FIELD(patientId), QI_FIELD(name), QI_FIELD(status), QI_FIELD(onset));

/// A medication on the patient's list.
class Medication : public QiModel {
    QI_MODEL
public:
    QiField<int>     patientId;
    QiField<QString> name;
    QiField<QString> dose;
    QiField<QString> frequency;
    QiField<bool>    active;
};
QI_DECLARE_MODEL(Medication, "medication",
    QI_FIELD(patientId), QI_FIELD(name), QI_FIELD(dose), QI_FIELD(frequency), QI_FIELD(active));

/// A clinical note from a visit.
class Note : public QiModel {
    QI_MODEL
public:
    QiField<int>     patientId;
    QiField<int>     providerId;
    QiField<QString> date;        // yyyy-MM-dd
    QiField<QString> kind;        // Office Visit | Phone | Lab Review | ...
    QiField<QString> body;
};
QI_DECLARE_MODEL(Note, "note",
    QI_FIELD(patientId), QI_FIELD(providerId), QI_FIELD(date), QI_FIELD(kind), QI_FIELD(body));

#endif // MODELS_H
