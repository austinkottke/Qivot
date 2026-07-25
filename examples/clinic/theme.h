#ifndef THEME_H
#define THEME_H

#include <QObject>
#include <QColor>

/// Design tokens + a few view-formatting helpers, exposed to every QML component
/// as the `Theme` context property (so components share one style, no prop-drilling).
class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(QColor bg     MEMBER m_bg     CONSTANT)
    Q_PROPERTY(QColor card   MEMBER m_card   CONSTANT)
    Q_PROPERTY(QColor teal   MEMBER m_teal   CONSTANT)
    Q_PROPERTY(QColor ink    MEMBER m_ink    CONSTANT)
    Q_PROPERTY(QColor muted  MEMBER m_muted  CONSTANT)
    Q_PROPERTY(QColor border MEMBER m_border CONSTANT)
    Q_PROPERTY(QColor field  MEMBER m_field  CONSTANT)
public:
    explicit Theme(QObject *parent = nullptr) : QObject(parent) {}

    /// Colour for an appointment status (takes a Clinic::ApptStatus value).
    Q_INVOKABLE QString statusColor(int apptStatus) const;
    /// Deterministic avatar colour from a record id.
    Q_INVOKABLE QString avatarColor(int id) const;
    /// Two-letter initials from a first + last name.
    Q_INVOKABLE QString initials(const QString &first, const QString &last) const;
    /// Format a "yyyy-MM-dd" string for display.
    Q_INVOKABLE QString dateLabel(const QString &iso, const QString &fmt = "MMM d, yyyy") const;

private:
    QColor m_bg{"#EEF1F6"}, m_card{"#FFFFFF"}, m_teal{"#0E8C93"}, m_ink{"#1F2733"},
           m_muted{"#6B7280"}, m_border{"#E2E6EE"}, m_field{"#F1F3F8"};
};

#endif // THEME_H
