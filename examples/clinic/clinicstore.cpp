#include "clinicstore.h"

#include <QDate>
#include <QMap>

// ---- formatters ------------------------------------------------------------
int ClinicStore::ageOf(const QString &dob) const {
    const QDate d = QDate::fromString(dob, "yyyy-MM-dd");
    if (!d.isValid()) return 0;
    const QDate now = QDate::currentDate();
    int age = now.year() - d.year();
    if (now.month() < d.month() || (now.month() == d.month() && now.day() < d.day())) age--;
    return age;
}

QString ClinicStore::minuteLabel(int m) const {
    int h = m / 60, mi = m % 60;
    const QString ap = h < 12 ? "AM" : "PM";
    int h12 = h % 12; if (h12 == 0) h12 = 12;
    return QString("%1:%2 %3").arg(h12).arg(mi, 2, 10, QChar('0')).arg(ap);
}

QString ClinicStore::todayIso() const { return QDate::currentDate().toString("yyyy-MM-dd"); }

QString ClinicStore::scheduleLabel() const {
    const QDate d = QDate::fromString(m_day, "yyyy-MM-dd");
    return d.isValid() ? d.toString("dddd, MMMM d") : m_day;
}

bool ClinicStore::isToday() const { return m_day == todayIso(); }

// ---- ctor ------------------------------------------------------------------
ClinicStore::ClinicStore(QObject *parent) : QObject(parent) {
    for (QiListModel *m : { &m_patients, &m_providers, &m_problems, &m_medications, &m_vitals,
                            &m_appointments, &m_notes, &m_schedule, &m_noteResults })
        m->setParent(this);   // C++ owns them; QML just reads them

    loadCaches();
    m_day = todayIso();
    refreshPatients();
    refreshSchedule();
    rebuildOverview();

    QiList<Patient> first = QiQuery<Patient>().orderBy("lastName asc, firstName asc").limit(1).all();
    if (first.size()) selectPatient(first.at(0)->id().toInt());
}

void ClinicStore::loadCaches() {
    QiList<Provider> ps = QiQuery<Provider>().orderBy("id asc").all();
    m_provName.clear(); m_provColor.clear(); m_provIndex.clear(); m_provIds.clear();
    for (int i = 0; i < ps.size(); i++) {
        Provider *p = ps.at(i);
        const int id = p->id().toInt();
        m_provName[id]  = p->name.get().toString();
        m_provColor[id] = p->color.get().toString();
        m_provIndex[id] = i;
        m_provIds << id;
    }
    m_providers.setList(ps);   // the schedule headers + the provider ComboBox bind to this

    QiList<Patient> all = QiQuery<Patient>().all();
    m_patName.clear();
    for (int i = 0; i < all.size(); i++) {
        Patient *p = all.at(i);
        m_patName[p->id().toInt()] = p->firstName.get().toString() + " " + p->lastName.get().toString();
    }
}

// ---- patient directory (just a query -> a model) ---------------------------
void ClinicStore::search(const QString &text) { m_query = text.trimmed(); refreshPatients(); }

void ClinicStore::refreshPatients() {
    QiQuery<Patient> q;
    if (!m_query.isEmpty()) {
        const QString like = "%" + m_query + "%";
        q = q.filter(QiWhere("lastName LIKE ", like)
                     || QiWhere("firstName LIKE ", like)
                     || QiWhere("mrn LIKE ", like));
    }
    m_patients.setList(q.orderBy("lastName asc, firstName asc").all());
    emit patientsChanged();
}

// ---- one patient's chart (five queries -> five models + two gadgets) -------
void ClinicStore::selectPatient(int id) {
    m_selectedId = id;

    QiList<Patient> pl = QiQuery<Patient>().filter(QiWhere("id = ", id)).limit(1).all();
    m_patient = pl.size() ? *pl.at(0) : Patient();

    m_problems.setList(QiQuery<Problem>().filter(QiWhere("patientId = ", id))
                         .orderBy("status asc, name asc").all());
    m_medications.setList(QiQuery<Medication>().filter(QiWhere("patientId = ", id))
                            .orderBy("active desc, name asc").all());

    QiList<Vital> vits = QiQuery<Vital>().filter(QiWhere("patientId = ", id))
                           .orderBy("takenOn desc").all();
    m_vitals.setList(vits);
    m_latestVital = vits.size() ? *vits.at(0) : Vital();

    m_appointments.setList(QiQuery<Appointment>().filter(QiWhere("patientId = ", id))
                             .orderBy("day desc, minute desc").all());
    m_notes.setList(QiQuery<Note>().filter(QiWhere("patientId = ", id))
                      .orderBy("date desc").all());

    emit chartChanged();
}

// ---- scheduler -------------------------------------------------------------
void ClinicStore::setScheduleDay(const QString &iso) { m_day = iso; refreshSchedule(); }
void ClinicStore::goToday() { setScheduleDay(todayIso()); }
void ClinicStore::shiftDay(int deltaDays) {
    setScheduleDay(QDate::fromString(m_day, "yyyy-MM-dd").addDays(deltaDays).toString("yyyy-MM-dd"));
}

void ClinicStore::refreshSchedule() {
    m_schedule.setList(QiQuery<Appointment>().filter(QiWhere("day = ", m_day))
                         .orderBy("minute asc").all());

    auto n = [&](const QString &st) {
        return QiQuery<Appointment>().filter(QiWhere("day = ", m_day)
                                             && QiWhere("status = ", st)).count();
    };
    const int total = QiQuery<Appointment>().filter(QiWhere("day = ", m_day)
                                                    && QiWhere("status <> ", "cancelled")).count();
    m_stats["total"] = total;
    m_stats["arrived"] = n("arrived");
    m_stats["completed"] = n("completed");
    emit scheduleChanged();
}

// ---- booking (transaction + double-booking guard) --------------------------
bool ClinicStore::book(int patientId, int providerId, int minute, int durationMin,
                       const QString &reason) {
    QiTransaction txn;
    const int newEnd = minute + durationMin;
    QiList<Appointment> same = QiQuery<Appointment>()
        .filter(QiWhere("day = ", m_day) && QiWhere("providerId = ", providerId)
                && QiWhere("status <> ", "cancelled")).all();
    for (int i = 0; i < same.size(); i++) {
        const int s = same.at(i)->minute.get().toInt();
        const int e = s + same.at(i)->durationMin.get().toInt();
        if (minute < e && s < newEnd) {
            txn.rollback();
            m_lastError = QString("%1 is already booked at %2.")
                              .arg(providerName(providerId)).arg(minuteLabel(s));
            emit errorChanged();
            return false;
        }
    }

    Appointment a;
    a.patientId = patientId; a.providerId = providerId; a.day = m_day;
    a.minute = minute; a.durationMin = durationMin;
    a.reason = reason.isEmpty() ? QString("Office Visit") : reason;
    a.status = QString("scheduled");
    if (!a.save()) { txn.rollback(); m_lastError = "Could not save the appointment."; emit errorChanged(); return false; }
    txn.commit();

    m_lastError.clear(); emit errorChanged();
    refreshSchedule(); rebuildOverview();
    if (m_selectedId == patientId) selectPatient(patientId);
    return true;
}

void ClinicStore::setStatus(int appointmentId, const QString &status) {
    QiList<Appointment> al = QiQuery<Appointment>().filter(QiWhere("id = ", appointmentId)).limit(1).all();
    if (al.size() == 0) return;
    Appointment *a = al.at(0);
    a->status = status;
    a->save();
    refreshSchedule(); rebuildOverview();
    if (m_selectedId == a->patientId.get().toInt()) selectPatient(m_selectedId);
}

// ---- full-text note search (FTS5) ------------------------------------------
void ClinicStore::searchNotes(const QString &text) {
    m_noteQuery = text.trimmed();
    if (m_noteQuery.isEmpty()) { m_noteResults.setList(QiList<Note>()); emit noteResultsChanged(); return; }

    QStringList toks; QString cur;
    const QString lower = m_noteQuery.toLower();
    for (int i = 0; i <= lower.size(); i++) {
        const QChar c = i < lower.size() ? lower.at(i) : QChar(' ');
        if (c.isLetterOrNumber()) cur += c;
        else if (!cur.isEmpty()) { toks << cur + "*"; cur.clear(); }
    }
    if (toks.isEmpty()) { m_noteResults.setList(QiList<Note>()); emit noteResultsChanged(); return; }

    m_noteResults.setList(QiQuery<Note>().search("note_fts", toks.join(' ')).limit(40).all());
    emit noteResultsChanged();
}

// ---- writes ----------------------------------------------------------------
void ClinicStore::addNote(int patientId, int providerId, const QString &kind, const QString &body) {
    if (body.trimmed().isEmpty()) return;
    Note n;
    n.patientId = patientId;
    n.providerId = providerId > 0 ? providerId : (m_provIds.isEmpty() ? 1 : m_provIds.first());
    n.date = todayIso();
    n.kind = kind.isEmpty() ? QString("Office Visit") : kind;
    n.body = body.trimmed();
    n.save();                                 // FTS index stays in sync automatically
    if (m_selectedId == patientId) selectPatient(patientId);
}

void ClinicStore::addVital(int patientId, int systolic, int diastolic, int heartRate,
                           double tempC, int spo2, double weightKg, int heightCm) {
    Vital v;
    v.patientId = patientId; v.takenOn = todayIso();
    v.systolic = systolic; v.diastolic = diastolic; v.heartRate = heartRate;
    v.tempC = tempC; v.spo2 = spo2; v.weightKg = weightKg; v.heightCm = heightCm;
    v.save();
    if (m_selectedId == patientId) selectPatient(patientId);   // header re-summarizes
}

int ClinicStore::addPatient(const QString &first, const QString &last, const QString &dob,
                            const QString &sex, const QString &phone) {
    Patient p;
    p.mrn = QString("MRN-%1").arg(200000 + Patient::objects().count());
    p.firstName = first.trimmed().isEmpty() ? QString("New") : first.trimmed();
    p.lastName  = last.trimmed().isEmpty() ? QString("Patient") : last.trimmed();
    p.dob = dob.isEmpty() ? QString("1990-01-01") : dob;
    p.sex = sex.isEmpty() ? QString("F") : sex;
    p.phone = phone; p.bloodType = "O+"; p.allergies = "";
    p.save();
    const int id = p.id().toInt();
    m_query.clear();
    loadCaches();          // refresh the patient-name lookup
    refreshPatients();
    rebuildOverview();
    selectPatient(id);
    return id;
}

// ---- analytics (aggregate queries) -----------------------------------------
void ClinicStore::rebuildOverview() {
    m_overview.clear();
    const QString today = todayIso();
    const QString weekEnd = QDate::currentDate().addDays(6).toString("yyyy-MM-dd");
    auto todayCount = [&](const QString &st) {
        return QiQuery<Appointment>().filter(QiWhere("day = ", today) && QiWhere("status = ", st)).count();
    };

    m_overview["patients"] = Patient::objects().count();
    const int todayTotal = QiQuery<Appointment>()
        .filter(QiWhere("day = ", today) && QiWhere("status <> ", "cancelled")).count();
    const int todayDone = todayCount("completed");
    m_overview["todayTotal"]   = todayTotal;
    m_overview["todayArrived"] = todayCount("arrived");
    m_overview["completion"]   = todayTotal > 0 ? qRound(100.0 * todayDone / todayTotal) : 0;
    m_overview["activeProblems"] = QiQuery<Problem>().filter(QiWhere("status = ", "active")).count();
    m_overview["upcoming"] = QiQuery<Appointment>()
        .filter(QiWhere("day >= ", today) && QiWhere("day <= ", weekEnd)
                && QiWhere("status <> ", "cancelled")).count();
    {
        QiList<Patient> ps = QiQuery<Patient>().all();
        long sum = 0;
        for (int i = 0; i < ps.size(); i++) sum += ageOf(ps.at(i)->dob.get().toString());
        m_overview["avgAge"] = ps.size() ? int(sum / ps.size()) : 0;
    }

    // appointments per day this week (GROUP BY)
    QMap<QString, int> perDay;
    {
        QiQuery<Appointment> q = QiQuery<Appointment>()
            .filter(QiWhere("day >= ", today) && QiWhere("day <= ", weekEnd)
                    && QiWhere("status <> ", "cancelled"))
            .select(QStringList() << "day" << "count(*)").groupBy("day");
        if (q.exec()) while (q.next()) perDay[q.value(0).toString()] = q.value(1).toInt();
    }
    QVariantList byDay; int maxDay = 1;
    for (int d = 0; d < 7; d++) {
        const QDate dt = QDate::currentDate().addDays(d);
        const int c = perDay.value(dt.toString("yyyy-MM-dd"), 0);
        maxDay = qMax(maxDay, c);
        QVariantMap m; m["label"] = dt.toString("ddd"); m["count"] = c; byDay << m;
    }
    m_overview["byDay"] = byDay; m_overview["byDayMax"] = maxDay;

    // appointments per provider this week (GROUP BY)
    QMap<int, int> perProv;
    {
        QiQuery<Appointment> q = QiQuery<Appointment>()
            .filter(QiWhere("day >= ", today) && QiWhere("day <= ", weekEnd)
                    && QiWhere("status <> ", "cancelled"))
            .select(QStringList() << "providerId" << "count(*)").groupBy("providerId");
        if (q.exec()) while (q.next()) perProv[q.value(0).toInt()] = q.value(1).toInt();
    }
    QVariantList byProv; int maxProv = 1;
    for (int id : m_provIds) {
        const int c = perProv.value(id, 0);
        maxProv = qMax(maxProv, c);
        QVariantMap m; m["name"] = m_provName.value(id); m["color"] = m_provColor.value(id); m["count"] = c;
        byProv << m;
    }
    m_overview["byProvider"] = byProv; m_overview["byProviderMax"] = maxProv;

    // today's status breakdown
    QVariantMap byStatus;
    for (const QString &s : { QString("scheduled"), QString("arrived"),
                              QString("completed"), QString("cancelled") })
        byStatus[s] = todayCount(s);
    m_overview["byStatus"] = byStatus;

    // top active conditions (GROUP BY name)
    QVariantList topCond;
    {
        QiQuery<Problem> q = QiQuery<Problem>().filter(QiWhere("status = ", "active"))
            .select(QStringList() << "name" << "count(*) c").groupBy("name").orderBy("c desc").limit(6);
        if (q.exec()) while (q.next()) {
            QVariantMap m; m["name"] = q.value(0).toString(); m["count"] = q.value(1).toInt();
            topCond << m;
        }
    }
    m_overview["topConditions"] = topCond;

    emit overviewChanged();
}
