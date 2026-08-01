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
    Q_PROPERTY(QColor accent MEMBER m_accent CONSTANT)
    Q_PROPERTY(QColor ink    MEMBER m_ink    CONSTANT)
    Q_PROPERTY(QColor muted  MEMBER m_muted  CONSTANT)
    Q_PROPERTY(QColor border MEMBER m_border CONSTANT)
    Q_PROPERTY(QColor field  MEMBER m_field  CONSTANT)
public:
    explicit Theme(QObject *parent = nullptr) : QObject(parent) {}

    /// Colour for an Erp::OppStage value.
    Q_INVOKABLE QString stageColor(int stage) const;
    /// Colour for an Erp::ProjectStatus value.
    Q_INVOKABLE QString projectStatusColor(int status) const;
    /// Colour for an invoice's displayed status (Draft/Sent/Paid/Overdue).
    Q_INVOKABLE QString invoiceStatusColor(int status, const QString &dueDate) const;
    /// Deterministic avatar colour from a record id.
    Q_INVOKABLE QString avatarColor(int id) const;
    /// Two-letter initials from a name (first + last word).
    Q_INVOKABLE QString initials(const QString &name) const;
    /// Format a "yyyy-MM-dd" string for display.
    Q_INVOKABLE QString dateLabel(const QString &iso, const QString &fmt = "MMM d, yyyy") const;

private:
    QColor m_bg{"#EEF1F6"}, m_card{"#FFFFFF"}, m_accent{"#2563EB"}, m_ink{"#1F2733"},
           m_muted{"#6B7280"}, m_border{"#E2E6EE"}, m_field{"#F1F3F8"};
};

#endif // THEME_H
