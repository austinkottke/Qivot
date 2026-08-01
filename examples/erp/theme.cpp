#include "theme.h"
#include "models.h"        // Erp::OppStage, Erp::ProjectStatus, Erp::InvoiceStatus
#include <QDate>
#include <QStringList>

QString Theme::stageColor(int s) const {
    switch (Erp::OppStage(s)) {
        case Erp::Lead:      return "#94A3B8";
        case Erp::Qualified: return "#3B82F6";
        case Erp::Proposal:  return "#F59E0B";
        case Erp::Won:       return "#10B981";
        case Erp::Lost:      return "#EF4444";
    }
    return "#94A3B8";
}

QString Theme::projectStatusColor(int s) const {
    switch (Erp::ProjectStatus(s)) {
        case Erp::Planning:  return "#94A3B8";
        case Erp::Active:    return "#10B981";
        case Erp::OnHold:    return "#F59E0B";
        case Erp::Completed: return "#6366F1";
    }
    return "#94A3B8";
}

QString Theme::invoiceStatusColor(int status, const QString &dueDate) const {
    const bool overdue = Erp::InvoiceStatus(status) == Erp::Sent
        && QDate::fromString(dueDate, "yyyy-MM-dd").isValid()
        && QDate::fromString(dueDate, "yyyy-MM-dd") < QDate::currentDate();
    if (overdue) return "#EF4444";
    switch (Erp::InvoiceStatus(status)) {
        case Erp::Draft: return "#94A3B8";
        case Erp::Sent:  return "#3B82F6";
        case Erp::Paid:  return "#10B981";
    }
    return "#94A3B8";
}

QString Theme::avatarColor(int id) const {
    static const QString pal[] = { "#3B82F6","#10B981","#8B5CF6","#F59E0B","#EF4444",
                                   "#06B6D4","#EC4899","#14B8A6","#6366F1","#F97316" };
    return pal[qAbs(id) % 10];
}

QString Theme::initials(const QString &name) const {
    const QStringList parts = name.trimmed().split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return "?";
    if (parts.size() == 1) return parts.first().left(2).toUpper();
    return (parts.first().left(1) + parts.last().left(1)).toUpper();
}

QString Theme::dateLabel(const QString &iso, const QString &fmt) const {
    const QDate d = QDate::fromString(iso, "yyyy-MM-dd");
    return d.isValid() ? d.toString(fmt) : QString();
}
