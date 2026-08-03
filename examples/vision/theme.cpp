#include "theme.h"
#include <QDate>
#include <QStringList>
#include <QCryptographicHash>

QString Theme::statusColor(const QString &code) const {
    if (code == "A") return m_dark ? "#34D399" : "#10B981";   // active
    if (code == "D") return m_dark ? "#FBBF24" : "#F59E0B";   // dormant
    if (code == "I") return "#94A3B8";                        // inactive
    return "#94A3B8";
}

QString Theme::statusLabel(const QString &code) const {
    if (code == "A") return "Active";
    if (code == "D") return "Dormant";
    if (code == "I") return "Inactive";
    if (code == "H") return "Overhead";
    return code;
}

QString Theme::ratioColor(double pct) const {
    if (pct >= 85) return m_dark ? "#34D399" : "#10B981";   // green
    if (pct >= 60) return m_dark ? "#FBBF24" : "#F59E0B";   // amber
    return m_dark ? "#F87171" : "#EF4444";                  // red
}

QString Theme::keyColor(const QString &key) const {
    static const QString palLight[] = { "#3B82F6","#10B981","#8B5CF6","#F59E0B","#EF4444",
                                        "#06B6D4","#EC4899","#14B8A6","#6366F1","#F97316" };
    static const QString palDark[]  = { "#5B9BFF","#34D399","#A78BFA","#FBBF24","#F87171",
                                        "#22D3EE","#F472B6","#2DD4BF","#818CF8","#FB923C" };
    const QByteArray h = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5);
    const int i = quint8(h.at(0)) % 10;
    return m_dark ? palDark[i] : palLight[i];
}

QString Theme::initials(const QString &name) const {
    QString n = name;
    n.replace(',', ' ');
    const QStringList parts = n.trimmed().split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return "?";
    if (parts.size() == 1) return parts.first().left(2).toUpper();
    return (parts.first().left(1) + parts.at(1).left(1)).toUpper();
}

QString Theme::dateLabel(const QString &iso, const QString &fmt) const {
    const QDate d = QDate::fromString(iso, "yyyy-MM-dd");
    return d.isValid() ? d.toString(fmt) : iso;
}
