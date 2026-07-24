#include "clinicstore.h"
#include "models.h"

#include <QDate>
#include <QMap>

// ---- small formatting helpers ---------------------------------------------
static const QStringList kAvatarColors = {
    "#3B82F6","#10B981","#8B5CF6","#F59E0B","#EF4444","#06B6D4",
    "#EC4899","#14B8A6","#6366F1","#F97316" };
static QString avatarColor(int id) { return kAvatarColors.at(id % kAvatarColors.size()); }

int ClinicStore::ageFromDob(const QString &dob) {
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

// ---- ctor ------------------------------------------------------------------
ClinicStore::ClinicStore(QObject *parent) : QObject(parent) {
    QiList<Provider> ps = QiQuery<Provider>().orderBy("id asc").all();
    for (int i = 0; i < ps.size(); i++) {
        Provider *p = ps.at(i);
        QVariantMap m;
        m["id"] = p->id().toInt();
        m["name"] = p->name.get().toString();
        m["specialty"] = p->specialty.get().toString();
        m["color"] = p->color.get().toString();
        m_providers << m;
    }
    m_day = QDate::currentDate().toString("yyyy-MM-dd");
    rebuildPatients();
    rebuildSchedule();
    rebuildOverview();
    // open the first patient so the chart isn't empty
    if (!m_patients.isEmpty()) selectPatient(m_patients.first().toMap()["id"].toInt());
}

QString ClinicStore::todayIso() const { return QDate::currentDate().toString("yyyy-MM-dd"); }

QVariantMap ClinicStore::providerById(int id) const {
    for (const QVariant &v : m_providers)
        if (v.toMap()["id"].toInt() == id) return v.toMap();
    return {};
}

QString ClinicStore::patientName(int id) const {
    QiList<Patient> pl = QiQuery<Patient>().filter(QiWhere("id = ", id)).limit(1).all();
    if (pl.size() == 0) return "—";
    Patient *p = pl.at(0);
    return p->firstName.get().toString() + " " + p->lastName.get().toString();
}

QString ClinicStore::scheduleLabel() const {
    const QDate d = QDate::fromString(m_day, "yyyy-MM-dd");
    return d.isValid() ? d.toString("dddd, MMMM d") : m_day;
}

bool ClinicStore::isToday() const {
    return m_day == QDate::currentDate().toString("yyyy-MM-dd");
}

// ---- patient directory -----------------------------------------------------
void ClinicStore::search(const QString &text) {
    m_query = text.trimmed();
    rebuildPatients();
}

void ClinicStore::rebuildPatients() {
    m_patients.clear();

    QiQuery<Patient> q;
    if (!m_query.isEmpty()) {
        const QString like = "%" + m_query + "%";
        q = q.filter(QiWhere("lastName LIKE ", like)
                     || QiWhere("firstName LIKE ", like)
                     || QiWhere("mrn LIKE ", like));
    }
    QiList<Patient> ps = q.orderBy("lastName asc, firstName asc").all();

    // One query for everyone's next upcoming visit (instead of one per patient).
    const QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QMap<int, QString> nextByPatient;
    QiList<Appointment> up = QiQuery<Appointment>()
        .filter(QiWhere("day >= ", today) && QiWhere("status <> ", "cancelled"))
        .orderBy("day asc, minute asc").all();
    for (int i = 0; i < up.size(); i++) {
        Appointment *a = up.at(i);
        const int pid = a->patientId.get().toInt();
        if (!nextByPatient.contains(pid)) {
            const QDate d = QDate::fromString(a->day.get().toString(), "yyyy-MM-dd");
            nextByPatient[pid] = d.toString("MMM d") + " · " + minuteLabel(a->minute.get().toInt());
        }
    }

    for (int i = 0; i < ps.size(); i++) {
        Patient *p = ps.at(i);
        const int id = p->id().toInt();
        const QString fn = p->firstName.get().toString(), ln = p->lastName.get().toString();
        QVariantMap m;
        m["id"] = id;
        m["name"] = fn + " " + ln;
        m["mrn"] = p->mrn.get().toString();
        m["age"] = ageFromDob(p->dob.get().toString());
        m["sex"] = p->sex.get().toString();
        m["initials"] = QString(fn.left(1) + ln.left(1)).toUpper();
        m["color"] = avatarColor(id);
        m["next"] = nextByPatient.value(id, QString());
        m_patients << m;
    }
    emit patientsChanged();
}

// ---- one patient's chart ---------------------------------------------------
void ClinicStore::selectPatient(int id) {
    m_selectedId = id;
    m_patient.clear(); m_problems.clear(); m_medications.clear();
    m_vitals.clear(); m_appointments.clear(); m_notes.clear();

    QiList<Patient> pl = QiQuery<Patient>().filter(QiWhere("id = ", id)).limit(1).all();
    Patient *p = pl.size() == 0 ? nullptr : pl.at(0);
    if (p) {
        const QString fn = p->firstName.get().toString(), ln = p->lastName.get().toString();
        m_patient["id"] = id;
        m_patient["name"] = fn + " " + ln;
        m_patient["mrn"] = p->mrn.get().toString();
        m_patient["dob"] = p->dob.get().toString();
        m_patient["age"] = ageFromDob(p->dob.get().toString());
        m_patient["sex"] = p->sex.get().toString();
        m_patient["phone"] = p->phone.get().toString();
        m_patient["bloodType"] = p->bloodType.get().toString();
        m_patient["allergies"] = p->allergies.get().toString();
        m_patient["initials"] = QString(fn.left(1) + ln.left(1)).toUpper();
        m_patient["color"] = avatarColor(id);
    }

    // problems (active first)
    QiList<Problem> probs = QiQuery<Problem>().filter(QiWhere("patientId = ", id))
                              .orderBy("status asc, name asc").all();
    for (int i = 0; i < probs.size(); i++) {
        Problem *x = probs.at(i);
        QVariantMap m;
        m["name"] = x->name.get().toString();
        m["status"] = x->status.get().toString();
        m["onset"] = x->onset.get().toString();
        m_problems << m;
    }

    // medications (active first)
    QiList<Medication> meds = QiQuery<Medication>().filter(QiWhere("patientId = ", id))
                                .orderBy("active desc, name asc").all();
    for (int i = 0; i < meds.size(); i++) {
        Medication *x = meds.at(i);
        QVariantMap m;
        m["name"] = x->name.get().toString();
        m["dose"] = x->dose.get().toString();
        m["frequency"] = x->frequency.get().toString();
        m["active"] = x->active.get().toBool();
        m_medications << m;
    }

    // vitals, newest first; newest also becomes the summary on the header
    QiList<Vital> vits = QiQuery<Vital>().filter(QiWhere("patientId = ", id))
                           .orderBy("takenOn desc").all();
    for (int i = 0; i < vits.size(); i++) {
        Vital *x = vits.at(i);
        QVariantMap m;
        const QDate d = QDate::fromString(x->takenOn.get().toString(), "yyyy-MM-dd");
        m["date"] = d.toString("MMM d, yyyy");
        m["bp"] = QString("%1/%2").arg(x->systolic.get().toInt()).arg(x->diastolic.get().toInt());
        m["hr"] = x->heartRate.get().toInt();
        m["tempC"] = x->tempC.get().toDouble();
        m["spo2"] = x->spo2.get().toInt();
        m["weightKg"] = x->weightKg.get().toDouble();
        m["heightCm"] = x->heightCm.get().toInt();
        m_vitals << m;
        if (i == 0) {
            m_patient["bp"] = m["bp"];
            m_patient["hr"] = m["hr"];
            m_patient["tempC"] = QString::number(x->tempC.get().toDouble(), 'f', 1);
            m_patient["spo2"] = m["spo2"];
            m_patient["weightKg"] = QString::number(x->weightKg.get().toDouble(), 'f', 1);
            const double h = x->heightCm.get().toInt() / 100.0;
            m_patient["bmi"] = h > 0 ? QString::number(x->weightKg.get().toDouble() / (h * h), 'f', 1) : "—";
        }
    }

    // appointments, newest/future first
    const QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QiList<Appointment> appts = QiQuery<Appointment>().filter(QiWhere("patientId = ", id))
                                  .orderBy("day desc, minute desc").all();
    for (int i = 0; i < appts.size(); i++) {
        Appointment *a = appts.at(i);
        const QString day = a->day.get().toString();
        const QDate d = QDate::fromString(day, "yyyy-MM-dd");
        QVariantMap m;
        m["id"] = a->id().toInt();
        m["dateLabel"] = d.toString("ddd, MMM d");
        m["timeLabel"] = minuteLabel(a->minute.get().toInt());
        m["providerName"] = providerById(a->providerId.get().toInt())["name"];
        m["reason"] = a->reason.get().toString();
        m["status"] = a->status.get().toString();
        m["upcoming"] = (day >= today && a->status.get().toString() != "cancelled");
        m_appointments << m;
    }

    // notes, newest first
    QiList<Note> notes = QiQuery<Note>().filter(QiWhere("patientId = ", id))
                           .orderBy("date desc").all();
    for (int i = 0; i < notes.size(); i++) {
        Note *n = notes.at(i);
        const QDate d = QDate::fromString(n->date.get().toString(), "yyyy-MM-dd");
        QVariantMap m;
        m["dateLabel"] = d.toString("MMM d, yyyy");
        m["providerName"] = providerById(n->providerId.get().toInt())["name"];
        m["kind"] = n->kind.get().toString();
        m["body"] = n->body.get().toString();
        m_notes << m;
    }

    emit chartChanged();
}

// ---- scheduler -------------------------------------------------------------
void ClinicStore::setScheduleDay(const QString &iso) { m_day = iso; rebuildSchedule(); }
void ClinicStore::goToday() { setScheduleDay(QDate::currentDate().toString("yyyy-MM-dd")); }
void ClinicStore::shiftDay(int deltaDays) {
    const QDate d = QDate::fromString(m_day, "yyyy-MM-dd").addDays(deltaDays);
    setScheduleDay(d.toString("yyyy-MM-dd"));
}

void ClinicStore::rebuildSchedule() {
    m_schedule.clear();
    int total = 0, arrived = 0, completed = 0;

    QiList<Appointment> appts = QiQuery<Appointment>().filter(QiWhere("day = ", m_day))
                                  .orderBy("minute asc").all();
    for (int i = 0; i < appts.size(); i++) {
        Appointment *a = appts.at(i);
        const QString status = a->status.get().toString();
        const int minute = a->minute.get().toInt();
        const int dur = a->durationMin.get().toInt();
        const QVariantMap prov = providerById(a->providerId.get().toInt());
        QVariantMap m;
        m["id"] = a->id().toInt();
        m["patientId"] = a->patientId.get().toInt();
        m["patientName"] = patientName(a->patientId.get().toInt());
        m["providerId"] = a->providerId.get().toInt();
        m["providerName"] = prov["name"];
        m["providerColor"] = prov["color"];
        m["minute"] = minute;
        m["endMinute"] = minute + dur;
        m["durationMin"] = dur;
        m["timeLabel"] = minuteLabel(minute);
        m["reason"] = a->reason.get().toString();
        m["status"] = status;
        m_schedule << m;
        if (status != "cancelled") total++;
        if (status == "arrived") arrived++;
        if (status == "completed") completed++;
    }
    m_stats.clear();
    m_stats["total"] = total;
    m_stats["arrived"] = arrived;
    m_stats["completed"] = completed;
    m_stats["remaining"] = total - completed;
    emit scheduleChanged();
}

// ---- booking (transaction + double-booking guard) --------------------------
bool ClinicStore::book(int patientId, int providerId, int minute, int durationMin,
                       const QString &reason) {
    QiTransaction txn;   // BEGIN

    // Refuse to double-book: does this provider already have an overlapping,
    // non-cancelled visit on this day?
    const int newEnd = minute + durationMin;
    QiList<Appointment> same = QiQuery<Appointment>()
        .filter(QiWhere("day = ", m_day)
                && QiWhere("providerId = ", providerId)
                && QiWhere("status <> ", "cancelled")).all();
    for (int i = 0; i < same.size(); i++) {
        Appointment *a = same.at(i);
        const int s = a->minute.get().toInt();
        const int e = s + a->durationMin.get().toInt();
        if (minute < e && s < newEnd) {                 // overlap
            txn.rollback();
            m_lastError = QString("%1 is already booked at %2.")
                              .arg(providerById(providerId)["name"].toString())
                              .arg(minuteLabel(s));
            emit errorChanged();
            return false;
        }
    }

    Appointment a;
    a.patientId = patientId;
    a.providerId = providerId;
    a.day = m_day;
    a.minute = minute;
    a.durationMin = durationMin;
    a.reason = reason.isEmpty() ? QString("Office Visit") : reason;
    a.status = QString("scheduled");
    if (!a.save()) { txn.rollback(); m_lastError = "Could not save the appointment."; emit errorChanged(); return false; }
    txn.commit();

    m_lastError.clear(); emit errorChanged();
    rebuildSchedule();
    rebuildPatients();
    rebuildOverview();
    if (m_selectedId == patientId) selectPatient(patientId);
    return true;
}

void ClinicStore::setStatus(int appointmentId, const QString &status) {
    QiList<Appointment> al = QiQuery<Appointment>().filter(QiWhere("id = ", appointmentId)).limit(1).all();
    if (al.size() == 0) return;
    Appointment *a = al.at(0);
    a->status = status;
    a->save();
    rebuildSchedule();
    if (m_selectedId == a->patientId.get().toInt()) selectPatient(m_selectedId);
    rebuildPatients();
    rebuildOverview();
}

// ---- analytics (aggregate queries) -----------------------------------------
void ClinicStore::rebuildOverview() {
    m_overview.clear();
    const QString today = QDate::currentDate().toString("yyyy-MM-dd");
    const QString weekEnd = QDate::currentDate().addDays(6).toString("yyyy-MM-dd");

    // --- KPIs (count / avg) ---
    m_overview["patients"]    = Patient::objects().count();
    const int todayTotal = QiQuery<Appointment>()
        .filter(QiWhere("day = ", today) && QiWhere("status <> ", "cancelled")).count();
    const int todayDone = QiQuery<Appointment>()
        .filter(QiWhere("day = ", today) && QiWhere("status = ", "completed")).count();
    m_overview["todayTotal"]  = todayTotal;
    m_overview["todayArrived"] = QiQuery<Appointment>()
        .filter(QiWhere("day = ", today) && QiWhere("status = ", "arrived")).count();
    m_overview["todayDone"]   = todayDone;
    m_overview["completion"]  = todayTotal > 0 ? qRound(100.0 * todayDone / todayTotal) : 0;
    m_overview["activeProblems"] = QiQuery<Problem>().filter(QiWhere("status = ", "active")).count();
    m_overview["upcoming"]    = QiQuery<Appointment>()
        .filter(QiWhere("day >= ", today) && QiWhere("day <= ", weekEnd)
                && QiWhere("status <> ", "cancelled")).count();

    // average patient age (compute in C++ from the dob strings)
    {
        QiList<Patient> ps = QiQuery<Patient>().all();
        long sum = 0;
        for (int i = 0; i < ps.size(); i++) sum += ageFromDob(ps.at(i)->dob.get().toString());
        m_overview["avgAge"] = ps.size() ? int(sum / ps.size()) : 0;
    }

    // --- appointments per day this week (GROUP BY day) ---
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
        const int n = perDay.value(dt.toString("yyyy-MM-dd"), 0);
        maxDay = qMax(maxDay, n);
        QVariantMap m; m["label"] = dt.toString("ddd"); m["count"] = n; byDay << m;
    }
    m_overview["byDay"] = byDay;
    m_overview["byDayMax"] = maxDay;

    // --- appointments per provider this week (GROUP BY providerId) ---
    QMap<int, int> perProv;
    {
        QiQuery<Appointment> q = QiQuery<Appointment>()
            .filter(QiWhere("day >= ", today) && QiWhere("day <= ", weekEnd)
                    && QiWhere("status <> ", "cancelled"))
            .select(QStringList() << "providerId" << "count(*)").groupBy("providerId");
        if (q.exec()) while (q.next()) perProv[q.value(0).toInt()] = q.value(1).toInt();
    }
    QVariantList byProv; int maxProv = 1;
    for (const QVariant &pv : m_providers) {
        const QVariantMap p = pv.toMap();
        const int n = perProv.value(p["id"].toInt(), 0);
        maxProv = qMax(maxProv, n);
        QVariantMap m; m["name"] = p["name"]; m["color"] = p["color"]; m["count"] = n; byProv << m;
    }
    m_overview["byProvider"] = byProv;
    m_overview["byProviderMax"] = maxProv;

    // --- today's status breakdown ---
    QVariantMap byStatus;
    for (const QString &s : { QString("scheduled"), QString("arrived"),
                              QString("completed"), QString("cancelled") })
        byStatus[s] = QiQuery<Appointment>()
            .filter(QiWhere("day = ", today) && QiWhere("status = ", s)).count();
    m_overview["byStatus"] = byStatus;

    // --- top active conditions across the panel (GROUP BY name) ---
    QVariantList topCond;
    {
        QiQuery<Problem> q = QiQuery<Problem>().filter(QiWhere("status = ", "active"))
            .select(QStringList() << "name" << "count(*) c").groupBy("name")
            .orderBy("c desc").limit(6);
        if (q.exec()) while (q.next()) {
            QVariantMap m; m["name"] = q.value(0).toString(); m["count"] = q.value(1).toInt();
            topCond << m;
        }
    }
    m_overview["topConditions"] = topCond;

    emit overviewChanged();
}

// ---- full-text search over clinical notes (FTS5) ---------------------------
void ClinicStore::searchNotes(const QString &text) {
    m_noteQuery = text.trimmed();
    m_noteResults.clear();
    if (m_noteQuery.isEmpty()) { emit noteResultsChanged(); return; }

    QStringList toks;
    QString cur;
    const QString lower = m_noteQuery.toLower();
    for (int i = 0; i <= lower.size(); i++) {
        const QChar c = i < lower.size() ? lower.at(i) : QChar(' ');
        if (c.isLetterOrNumber()) cur += c;
        else if (!cur.isEmpty()) { toks << cur + "*"; cur.clear(); }
    }
    if (toks.isEmpty()) { emit noteResultsChanged(); return; }

    QiList<Note> hits = QiQuery<Note>().search("note_fts", toks.join(' ')).limit(40).all();
    for (int i = 0; i < hits.size(); i++) {
        Note *n = hits.at(i);
        const QDate d = QDate::fromString(n->date.get().toString(), "yyyy-MM-dd");
        QVariantMap m;
        m["patientId"] = n->patientId.get().toInt();
        m["patientName"] = patientName(n->patientId.get().toInt());
        m["kind"] = n->kind.get().toString();
        m["dateLabel"] = d.toString("MMM d, yyyy");
        m["body"] = n->body.get().toString();
        m_noteResults << m;
    }
    emit noteResultsChanged();
}

// ---- write operations ------------------------------------------------------
void ClinicStore::addNote(int patientId, int providerId, const QString &kind, const QString &body) {
    if (body.trimmed().isEmpty()) return;
    Note n;
    n.patientId = patientId;
    n.providerId = providerId > 0 ? providerId : (m_providers.isEmpty() ? 1 : m_providers.first().toMap()["id"].toInt());
    n.date = QDate::currentDate().toString("yyyy-MM-dd");
    n.kind = kind.isEmpty() ? QString("Office Visit") : kind;
    n.body = body.trimmed();
    n.save();                            // FTS index stays in sync automatically
    if (m_selectedId == patientId) selectPatient(patientId);
}

void ClinicStore::addVital(int patientId, int systolic, int diastolic, int heartRate,
                           double tempC, int spo2, double weightKg, int heightCm) {
    Vital v;
    v.patientId = patientId;
    v.takenOn = QDate::currentDate().toString("yyyy-MM-dd");
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
    p.lastName = last.trimmed().isEmpty() ? QString("Patient") : last.trimmed();
    p.dob = dob.isEmpty() ? QString("1990-01-01") : dob;
    p.sex = sex.isEmpty() ? QString("F") : sex;
    p.phone = phone;
    p.bloodType = "O+";
    p.allergies = "";
    p.save();
    const int id = p.id().toInt();
    m_query.clear();
    rebuildPatients();
    rebuildOverview();
    selectPatient(id);
    return id;
}
