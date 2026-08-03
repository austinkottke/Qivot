#ifndef THEME_H
#define THEME_H

#include <QObject>
#include <QColor>

/// Design tokens + a few view-formatting helpers, exposed to every QML component
/// as the `Theme` context property. Colours switch between a light and a dark
/// palette at runtime via `dark` — every token is a NOTIFYing read so bindings
/// re-evaluate the moment the theme flips.
class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool   dark    READ dark WRITE setDark NOTIFY changed)
    // Responsive breakpoint: main.qml sets this from the window width so every
    // component can adapt (phone-narrow layouts) by reading Theme.compact.
    Q_PROPERTY(bool   compact READ compact WRITE setCompact NOTIFY changed)
    Q_PROPERTY(QColor bg     READ bg     NOTIFY changed)
    Q_PROPERTY(QColor bg2    READ bg2    NOTIFY changed)   // gradient second stop
    Q_PROPERTY(QColor card   READ card   NOTIFY changed)
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)
    Q_PROPERTY(QColor accent2 READ accent2 NOTIFY changed) // gradient partner for accent
    Q_PROPERTY(QColor ink    READ ink    NOTIFY changed)
    Q_PROPERTY(QColor muted  READ muted  NOTIFY changed)
    Q_PROPERTY(QColor border READ border NOTIFY changed)
    Q_PROPERTY(QColor field  READ field  NOTIFY changed)
    Q_PROPERTY(QColor track  READ track  NOTIFY changed)
    Q_PROPERTY(QColor chip   READ chip   NOTIFY changed)
    Q_PROPERTY(QColor good   READ good   NOTIFY changed)
    Q_PROPERTY(QColor warn   READ warn   NOTIFY changed)
    Q_PROPERTY(QColor bad    READ bad    NOTIFY changed)
public:
    explicit Theme(QObject *parent = nullptr) : QObject(parent) {}

    bool dark() const { return m_dark; }
    void setDark(bool d) { if (m_dark == d) return; m_dark = d; emit changed(); }
    Q_INVOKABLE void toggle() { setDark(!m_dark); }

    bool compact() const { return m_compact; }
    void setCompact(bool c) { if (m_compact == c) return; m_compact = c; emit changed(); }

    QColor bg()     const { return m_dark ? QColor("#0B0E14") : QColor("#EEF1F6"); }
    QColor bg2()    const { return m_dark ? QColor("#0E1522") : QColor("#E7ECF3"); }
    QColor card()   const { return m_dark ? QColor("#161B24") : QColor("#FFFFFF"); }
    QColor accent() const { return m_dark ? QColor("#4F8CFF") : QColor("#2563EB"); }
    QColor accent2() const { return m_dark ? QColor("#7C6CFF") : QColor("#4F46E5"); }
    QColor ink()    const { return m_dark ? QColor("#E7ECF5") : QColor("#1F2733"); }
    QColor muted()  const { return m_dark ? QColor("#8B93A3") : QColor("#6B7280"); }
    QColor border() const { return m_dark ? QColor("#242C3A") : QColor("#E2E6EE"); }
    QColor field()  const { return m_dark ? QColor("#1E2531") : QColor("#F1F3F8"); }
    QColor track()  const { return m_dark ? QColor("#0E131C") : QColor("#E6E9F0"); }
    QColor chip()   const { return m_dark ? QColor("#222A38") : QColor("#EDF1F7"); }
    QColor good()   const { return m_dark ? QColor("#34D399") : QColor("#10B981"); }
    QColor warn()   const { return m_dark ? QColor("#FBBF24") : QColor("#F59E0B"); }
    QColor bad()    const { return m_dark ? QColor("#F87171") : QColor("#EF4444"); }

    /// Colour for a Vision status code: 'A' active, 'I' inactive, 'D' dormant.
    Q_INVOKABLE QString statusColor(const QString &code) const;
    /// Human label for a Vision status code.
    Q_INVOKABLE QString statusLabel(const QString &code) const;
    /// Colour on a red→amber→green ramp for a 0..100 percentage.
    Q_INVOKABLE QString ratioColor(double pct) const;
    /// Deterministic avatar colour from a string key.
    Q_INVOKABLE QString keyColor(const QString &key) const;
    /// Two-letter initials from "Last, First" or "First Last".
    Q_INVOKABLE QString initials(const QString &name) const;
    /// Format a "yyyy-MM-dd" string for display.
    Q_INVOKABLE QString dateLabel(const QString &iso, const QString &fmt = "MMM d, yyyy") const;

signals:
    void changed();

private:
    bool m_dark = true;   // dark by default — this demo is meant to look good in the dark
    bool m_compact = false;
};

#endif // THEME_H
