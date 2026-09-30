# Qivot on iOS and Android

Qivot runs unchanged on phones. The ORM, SQLite, windowed models, `QiAsync` and
the QML bindings are the same code you use on desktop. The full unit test suite
passes on the iOS simulator and on an Android emulator, and CI checks both on
every change (see [Continuous integration](#continuous-integration)).

This guide takes you from a desktop Qivot app to one running on a phone. It
uses the [Contacts example](../examples/contacts) throughout: a 10,000-row
address book with search, an A–Z index, swipe-to-delete and a detail card.

| | |
|---|---|
| **Qt** | 6.8 or newer, with the iOS and/or Android kits |
| **iOS** | macOS with Xcode 15 or 16 |
| **Android** | Android SDK + NDK (Android Studio installs both) and JDK 17 |

---

## 1. Install the tools

### Xcode and Android Studio

Install **Xcode** from the App Store and open it once so it installs its
simulators. Install **Android Studio**, then in *Settings › Languages &
Frameworks › Android SDK* add:

- an SDK platform (Android 14 / API 34 is what Qt 6.8 targets),
- *SDK Tools › NDK (Side by side)*. Qt 6.8 is built against **NDK r26b
  (26.1.10909125)**; newer NDKs generally work too (this guide was checked with r28),
- an emulator image. On an Apple Silicon Mac pick an **arm64-v8a** image.

You also need **JDK 17** (`brew install openjdk@17`, or the JDK bundled with
Android Studio).

### The Qt mobile kits

Xcode and Android Studio provide compilers and simulators. They don't provide
**Qt itself built for phones**, which is a separate download. Each mobile kit
also needs the *desktop* Qt of the same version beside it, because its build
tools (`moc`, `rcc`, `androiddeployqt`) run on your Mac or PC.

With the **Qt Online Installer**, tick *macOS*, *iOS* and *Android* under
Qt 6.8.x. Or use [aqtinstall](https://github.com/miurahr/aqtinstall), which is
free, needs no Qt account, and is what CI uses:

```bash
pip install aqtinstall
aqt install-qt mac desktop 6.8.3 clang_64          -O ~/Qt   # host tools
aqt install-qt mac ios     6.8.3 ios               -O ~/Qt   # iOS
aqt install-qt all_os android 6.8.3 android_arm64_v8a -O ~/Qt   # Android (phones, Apple Silicon emulators)
```

For an x86_64 emulator (Intel Macs, Linux, CI) use `android_x86_64` instead.
You end up with `~/Qt/6.8.3/macos`, `~/Qt/6.8.3/ios` and
`~/Qt/6.8.3/android_arm64_v8a`.

---

## 2. What a mobile app needs from you

Qivot needs no mobile-specific code. Your *app* needs three small changes,
and Contacts shows each one.

### 2.1 Put the database somewhere writable

A desktop app can open `app.db` relative to where it runs. A phone app starts
in `/`, which it can't write to, so every insert fails. Use the app's own data
directory:

```cpp
#include <QDir>
#include <QStandardPaths>

QString dbPath = "app.db";
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    dbPath = dir + "/app.db";
#endif

QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
db.setDatabaseName(dbPath);
```

That directory survives app updates and is removed when the app is uninstalled.
Seed data once (`if (Model::objects().count() == 0) …`) rather than on every
launch, or startup stays slow and user data is lost.

### 2.2 Use `QGuiApplication`, even with no UI

A phone app is started by Qt's platform plugin, which only runs under a
`QGuiApplication`. That applies even to something with no window, such as a
test runner. With a `QCoreApplication` and `QT -= gui`, Android packages the
app without its launcher, and iOS fails to link with
`Undefined symbols: _qt_main_wrapper`.

```cpp
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
using App = QGuiApplication;
#else
using App = QCoreApplication;
#endif
```

```qmake
ios|android: QT += gui
```

(A QML app already uses `QGuiApplication`, so this only matters for headless tools.)

### 2.3 Let iOS build a bundle

iOS only runs apps packaged as `.app` bundles, so don't switch bundling off there:

```qmake
!ios: CONFIG -= app_bundle
```

### 2.4 (Optional) Draw behind the iOS status bar

By default Qt 6.8 places the window below the status bar, so a black band sits
above your UI. To extend your background behind the clock and battery, set
`Qt::MaximizeUsingFullscreenGeometryHint` and pad your top bar by the
safe-area inset. Qt 6.8 doesn't expose that inset (`QWindow::safeAreaMargins`
arrives in 6.9), so Contacts reads it from UIKit in a few lines of
Objective-C++. See [`safearea_ios.mm`](../examples/contacts/safearea_ios.mm) and
the `#ifdef Q_OS_IOS` block in [`main.cpp`](../examples/contacts/main.cpp).

### 2.5 Touch-friendly QML

Things that behave differently once there's a finger and an on-screen keyboard:

- **Keep a search field outside a `ListView` whose model it resets.** In
  Contacts, every keystroke re-queries, which resets the model. A `TextField`
  inside the list's `header` lost keyboard focus on each reset, so only the
  first letter was ever typed. The field now floats over a slot in the header
  and scrolls with it.
- **Android's Back button** closes the app unless you handle it. Contacts pops
  its `StackView` in `onClosing` when a card is open.
- **Draw small icons instead of using symbol characters.** Android shows `✉` and
  `☎` as color emoji regardless of the text-style selector, and some Android
  fonts lack `✕`. Two `Rectangle`s or a `Canvas` look the same everywhere.
- `Page`, `Popup` and `Drawer` already have `topInset`/`bottomInset`
  properties (they pad the background). Name your own safe-area properties
  something else, or QML reports *Cannot override FINAL property*.

---

## 3. Run on the iOS simulator

```bash
mkdir -p build-ios && cd build-ios
~/Qt/6.8.3/ios/bin/qmake ../examples/contacts/contacts.pro

# qmake's Xcode project generates moc/rcc files in a script phase that a clean
# xcodebuild checks too early — run that step first.
make -f contacts.xcodeproj/qt_preprocess.mak

xcodebuild -project contacts.xcodeproj -sdk iphonesimulator -arch x86_64 \
           -configuration Debug CODE_SIGNING_ALLOWED=NO build
```

> **Why `-arch x86_64`?** Qt 6.8's iOS libraries contain an arm64 slice for
> *devices* and an x86_64 slice for the *simulator*. They have no arm64
> simulator slice. On an Apple Silicon Mac the simulator runs the x86_64 app
> under Rosetta (`softwareupdate --install-rosetta` if you've never needed it).
> Building for `arm64` fails with *building for 'iOS-simulator', but linking in
> object file built for 'iOS'*.

Then boot a simulator, install and launch:

```bash
open -a Simulator
xcrun simctl boot "iPhone 15"            # any device from: xcrun simctl list devices available
xcrun simctl install booted Debug-iphonesimulator/contacts.app
xcrun simctl launch --console-pty booted "$(/usr/libexec/PlistBuddy -c 'Print CFBundleIdentifier' Debug-iphonesimulator/contacts.app/Info.plist)"
```

`--console-pty` streams the app's `qDebug()`/`qWarning()` output, including any
QML errors, to your terminal.

### On a real iPhone

Open `contacts.xcodeproj` in Xcode, select the *contacts* target, and under
*Signing & Capabilities* choose your team (a free Apple ID works for your own
devices). Pick your iPhone as the run destination and press ▶. Device builds
use the kit's arm64 slice, so leave the architecture at its default.

---

## 4. Run on the Android emulator

```bash
export ANDROID_SDK_ROOT=~/Library/Android/sdk
export ANDROID_NDK_ROOT=$ANDROID_SDK_ROOT/ndk/26.1.10909125   # or the NDK you installed
export JAVA_HOME=$(/usr/libexec/java_home -v 17)

mkdir -p build-android && cd build-android
~/Qt/6.8.3/android_arm64_v8a/bin/qmake ../examples/contacts/contacts.pro
make -j8
make install INSTALL_ROOT=$PWD/android-build
~/Qt/6.8.3/macos/bin/androiddeployqt \
    --input android-contacts-deployment-settings.json \
    --output android-build --android-platform android-34 \
    --jdk "$JAVA_HOME" --gradle
```

The APK is `android-build/build/outputs/apk/debug/android-build-debug.apk`.
Start an emulator from Android Studio's *Device Manager*, then:

```bash
adb install -r android-build/build/outputs/apk/debug/android-build-debug.apk
adb shell am start -n org.qtproject.example.contacts/org.qtproject.qt.android.bindings.QtActivity
adb logcat -s default:V qml:V        # qDebug / QML output
```

### On a real Android phone

Enable *Developer options › USB debugging* on the phone, plug it in, check that
`adb devices` lists it, then run the same `adb install` and `adb shell am start`.
Build with the `android_arm64_v8a` kit, which covers virtually every current phone.

---

## 5. Run the test suite on a phone

[`tools/run-mobile-tests.sh`](../tools/run-mobile-tests.sh) does all of the
above for the unit tests *and* the Contacts example, then reports pass/fail. It
builds, installs, runs all three test suites (core, SQLite, SQL dialects), then
launches Contacts and fails if it logs a QML error or exits early.

```bash
# iOS (boots the first available iPhone simulator)
QT_MOBILE=~/Qt/6.8.3/ios tools/run-mobile-tests.sh ios

# Android (start an emulator first, or plug in a phone)
QT_MOBILE=~/Qt/6.8.3/android_arm64_v8a QT_HOST=~/Qt/6.8.3/macos \
ANDROID_SDK_ROOT=~/Library/Android/sdk \
ANDROID_NDK_ROOT=~/Library/Android/sdk/ndk/26.1.10909125 \
JAVA_HOME=$(/usr/libexec/java_home -v 17) \
tools/run-mobile-tests.sh android
```

```
== iOS: running unit tests ==
Totals: 23 passed, 0 failed, 0 skipped, 0 blacklisted, 23ms
Totals: 56 passed, 0 failed, 0 skipped, 0 blacklisted, 5683ms
Totals: 12 passed, 0 failed, 0 skipped, 0 blacklisted, 4ms
All test cases passed!
== iOS: Contacts smoke test ==
Contacts started cleanly.
== All ios checks passed ==
```

`--build-only` compiles and packages without a device. `--run-only` reuses the
last build. Output goes to `build-mobile/<platform>/`, including the full logs
(`unittests.log`, `contacts.log`). The header of the script lists every
setting it reads.

On Android the test app aborts in Android's rendering thread as `main()`
returns. That happens *after* the results are written, so the script judges the
run by its output ("All test cases passed!" or "Error found!"), not by the
process exit status.

---

## 6. Continuous integration

Two workflows run the same script on every push to `main` and on pull requests
that touch `src/`, `tests/`, the Contacts example or the script itself:

| Workflow | Runner | What it does |
|---|---|---|
| [`mobile-android.yml`](../.github/workflows/mobile-android.yml) | `ubuntu-24.04` | Installs Qt 6.8 (`android_x86_64`) and NDK r26b, builds, boots an API 34 x86_64 emulator under KVM, runs the tests and the Contacts smoke test. Uploads the APKs. |
| [`mobile-ios.yml`](../.github/workflows/mobile-ios.yml) | `macos-15` | Installs Rosetta and Qt 6.8 for iOS, builds for the x86_64 simulator, runs the tests and the Contacts smoke test. |

Every run uploads its logs (and, on Android, the APKs) as workflow artifacts. The iOS job is
pinned to `macos-15` because the x86_64-simulator-under-Rosetta setup depends
on Xcode 16. Moving it to a newer image needs a Qt release whose iOS kit
includes an arm64 simulator slice (then drop `IOS_SIM_ARCH` to `arm64`).

---

## 7. Troubleshooting

| Symptom | Cause and fix |
|---|---|
| Every insert fails on the phone; works on desktop | Relative database path. The app starts in `/`. See [2.1](#21-put-the-database-somewhere-writable). |
| iOS link error: `Undefined symbols: _qt_main_wrapper` | App built with `QT -= gui`. See [2.2](#22-use-qguiapplication-even-with-no-ui). |
| androiddeployqt: *No platform plugin … included in the deployment* | Same cause: link Qt Gui and create a `QGuiApplication`. |
| iOS link error: *building for 'iOS-simulator', but linking in object file built for 'iOS'* | Built the simulator for arm64. Use `-arch x86_64` with Qt 6.8. |
| `xcodebuild`: *Build input file cannot be found: moc_….cpp* | Run `make -f <app>.xcodeproj/qt_preprocess.mak` before `xcodebuild`. |
| iOS shows a black band above the app | Qt places the window below the status bar. See [2.4](#24-optional-draw-behind-the-ios-status-bar). |
| `adb install`: *Requested internal only, but not enough space* | The emulator's data partition is full. Wipe it in Device Manager, or create an AVD with a larger *Internal storage*. |
| QML: *Cannot override FINAL property* on `topInset` | `Page`/`Popup`/`Drawer` already define it. Rename yours. |
| A search field only accepts the first letter | It lives inside a `ListView` whose model it resets. See [2.5](#25-touch-friendly-qml). |
| Qt 6 warning: *requested database does not belong to the calling thread* during `QiAsync` work | Harmless. An empty placeholder query looks up the default connection on the worker thread and is replaced with the worker's own connection before it runs anything. |
| Emulator shows *Try out your stylus* when typing | Android's keyboard tutorial, not your app. Dismiss it once. |
