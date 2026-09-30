// iOS only: the insets the status bar, notch and home indicator cover.
// Qt 6.8 doesn't expose these to QML (QWindow::safeAreaMargins arrives in 6.9),
// so read them straight from UIKit.
#include "safearea.h"
#import <UIKit/UIKit.h>

QMargins iosSafeAreaMargins()
{
    // Prefer the key window; early in startup there may not be one yet, and any
    // of the app's windows reports the same device insets.
    UIWindow *best = nil;
    for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
        if (![scene isKindOfClass:UIWindowScene.class]) continue;
        for (UIWindow *window in ((UIWindowScene *)scene).windows) {
            if (window.isKeyWindow) { best = window; break; }
            if (!best) best = window;
        }
    }
    if (!best) return QMargins();
    const UIEdgeInsets in = best.safeAreaInsets;
    return QMargins(int(in.left), int(in.top), int(in.right), int(in.bottom));
}
