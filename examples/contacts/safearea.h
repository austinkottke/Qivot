#ifndef SAFEAREA_H
#define SAFEAREA_H

#include <QMargins>

#ifdef Q_OS_IOS
/// Insets covered by the status bar / notch / home indicator, in points.
QMargins iosSafeAreaMargins();
#endif

#endif // SAFEAREA_H
