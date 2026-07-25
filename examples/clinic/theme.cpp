#include "theme.h"
#include "models.h"       // Clinic::ApptStatus
#include <QDate>

QString Theme::statusColor(int s) const {
    switch (Clinic::ApptStatus(s)) {
        case Clinic::Arrived:   return "#10B981";
        case Clinic::Completed: return "#94A3B8";
        case Clinic::Cancelled: return "#EF4444";
        default:                return "#3B82F6";   // Scheduled
    }
}

QString Theme::avatarColor(int id) const {
    static const QString pal[] = { "#3B82F6","#10B981","#8B5CF6","#F59E0B","#EF4444",
                                   "#06B6D4","#EC4899","#14B8A6","#6366F1","#F97316" };
    return pal[qAbs(id) % 10];
}

QString Theme::initials(const QString &first, const QString &last) const {
    return (first.left(1) + last.left(1)).toUpper();
}

QString Theme::dateLabel(const QString &iso, const QString &fmt) const {
    const QDate d = QDate::fromString(iso, "yyyy-MM-dd");
    return d.isValid() ? d.toString(fmt) : QString();
}
