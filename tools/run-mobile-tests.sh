#!/usr/bin/env bash
# Build Qivot for iOS or Android, run the unit tests on a simulator/emulator,
# and smoke-test the Contacts example. Used by .github/workflows/mobile.yml and
# by hand — see docs/Mobile.md.
#
#   tools/run-mobile-tests.sh ios     [--build-only | --run-only]
#   tools/run-mobile-tests.sh android [--build-only | --run-only]
#
# --build-only  compile and package, don't touch a device (CI builds before the
#               emulator is up, then calls --run-only inside it)
# --run-only    reuse the last build and just run it
#
# Environment:
#   QT_MOBILE          target Qt kit, e.g. ~/Qt/6.8.3/ios or ~/Qt/6.8.3/android_arm64_v8a
#   QT_HOST            desktop Qt of the same version (androiddeployqt lives here)
#   BUILD_DIR          where to build (default: build-mobile/<platform>)
#   ANDROID_SDK_ROOT   Android SDK            (android)
#   ANDROID_NDK_ROOT   Android NDK            (android)
#   JAVA_HOME          JDK 17                 (android)
#   ANDROID_PLATFORM   SDK platform to build against (default: android-34)
#   IOS_SIM_ARCH       simulator arch (default: x86_64 — Qt 6.8's iOS libraries
#                      ship no arm64 simulator slice; runs under Rosetta on Apple Silicon)
#   IOS_SIM_DEVICE     simulator UDID to use (default: first available iPhone)
#   TEST_TIMEOUT       seconds to wait for the test run (default: 600)
#
# Exits 0 only if every test passed and Contacts started without a QML error.

set -euo pipefail

PLATFORM="${1:-}"
MODE="${2:-all}"
case "$PLATFORM" in ios|android) ;; *)
    echo "usage: $0 ios|android [--build-only|--run-only]" >&2; exit 2 ;;
esac
case "$MODE" in all|--build-only|--run-only) ;; *)
    echo "unknown option: $MODE" >&2; exit 2 ;;
esac

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build-mobile/$PLATFORM}"
TEST_TIMEOUT="${TEST_TIMEOUT:-600}"
: "${QT_MOBILE:?set QT_MOBILE to the $PLATFORM Qt kit (see docs/Mobile.md)}"
QMAKE="$QT_MOBILE/bin/qmake"
[ -x "$QMAKE" ] || { echo "no qmake at $QMAKE" >&2; exit 2; }
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

say()  { printf '\n\033[1m== %s ==\033[0m\n' "$*"; }
fail() { printf '\n\033[31mFAIL: %s\033[0m\n' "$*" >&2; exit 1; }

# Lines worth echoing from a QtTest run.
TEST_LINES='^(Config|PASS|FAIL!|XFAIL|XPASS|SKIP|Totals|QWARN|   Loc)|All test cases passed|Error found'

# =============================================================================
#  iOS
# =============================================================================

ios_build() {  # <name> <.pro>
    local name="$1" pro="$2" dir="$BUILD_DIR/$1"
    say "iOS: building $name for the simulator ($IOS_SIM_ARCH)"
    mkdir -p "$dir"
    (cd "$dir" && "$QMAKE" "$pro")
    # qmake's Xcode project generates moc/rcc files in a script phase that Xcode
    # checks *after* resolving inputs, so a clean build can't find them. Run it first.
    make -C "$dir" -f "$name.xcodeproj/qt_preprocess.mak"
    xcodebuild -project "$dir/$name.xcodeproj" -sdk iphonesimulator -arch "$IOS_SIM_ARCH" \
               -configuration Debug CODE_SIGNING_ALLOWED=NO build -quiet
}

ios_app()       { echo "$BUILD_DIR/$1/Debug-iphonesimulator/$1.app"; }
ios_bundle_id() { /usr/libexec/PlistBuddy -c "Print CFBundleIdentifier" "$(ios_app "$1")/Info.plist"; }

ios_device() {
    if [ -n "${IOS_SIM_DEVICE:-}" ]; then echo "$IOS_SIM_DEVICE"; return; fi
    xcrun simctl list devices available \
        | grep -E '^ +iPhone' | head -1 | grep -oE '[0-9A-F]{8}(-[0-9A-F]{4}){3}-[0-9A-F]{12}' || true
}

ios_run() {
    local dev; dev="$(ios_device)"
    [ -n "$dev" ] || fail "no available iPhone simulator (create one in Xcode › Devices)"
    say "iOS: booting simulator $dev"
    xcrun simctl boot "$dev" 2>/dev/null || true
    xcrun simctl bootstatus "$dev" -b >/dev/null

    # ---- Unit tests: the app prints QtTest output and exits ----
    say "iOS: running unit tests"
    xcrun simctl install "$dev" "$(ios_app unittests)"
    local log="$BUILD_DIR/unittests.log"
    xcrun simctl launch --console-pty --terminate-running-process "$dev" "$(ios_bundle_id unittests)" \
        >"$log" 2>&1 &
    local pid=$! waited=0
    while kill -0 "$pid" 2>/dev/null && [ "$waited" -lt "$TEST_TIMEOUT" ]; do sleep 2; waited=$((waited + 2)); done
    kill "$pid" 2>/dev/null || true
    grep -E "$TEST_LINES" "$log" || true
    grep -q "All test cases passed" "$log" || fail "iOS unit tests did not pass (full log: $log)"

    # ---- Contacts: must start and stay up with no QML errors ----
    say "iOS: Contacts smoke test"
    xcrun simctl install "$dev" "$(ios_app contacts)"
    local clog="$BUILD_DIR/contacts.log"
    xcrun simctl launch --console-pty --terminate-running-process "$dev" "$(ios_bundle_id contacts)" \
        >"$clog" 2>&1 &
    pid=$!
    sleep 20                                    # first launch seeds 10,000 contacts
    local alive=0; kill -0 "$pid" 2>/dev/null && alive=1
    xcrun simctl terminate "$dev" "$(ios_bundle_id contacts)" 2>/dev/null || true
    kill "$pid" 2>/dev/null || true
    if grep -E '\.qml:[0-9]+|failed to load component' "$clog"; then
        fail "Contacts reported QML errors (full log: $clog)"
    fi
    [ "$alive" = 1 ] || fail "Contacts exited early (full log: $clog)"
    echo "Contacts started cleanly."
}

# =============================================================================
#  Android
# =============================================================================

android_build() {  # <name> <.pro>
    local name="$1" pro="$2" dir="$BUILD_DIR/$1"
    : "${QT_HOST:?set QT_HOST to the desktop Qt of the same version}"
    : "${ANDROID_SDK_ROOT:?}" "${ANDROID_NDK_ROOT:?}" "${JAVA_HOME:?}"
    local deployqt="$QT_HOST/bin/androiddeployqt"
    [ -x "$deployqt" ] || deployqt="$QT_HOST/libexec/androiddeployqt"
    say "Android: building $name"
    mkdir -p "$dir"
    (cd "$dir" && "$QMAKE" "$pro")
    make -C "$dir" -j"$JOBS"
    make -C "$dir" install INSTALL_ROOT="$dir/android-build"
    "$deployqt" --input "$dir/android-$name-deployment-settings.json" \
                --output "$dir/android-build" \
                --android-platform "${ANDROID_PLATFORM:-android-34}" \
                --jdk "$JAVA_HOME" --gradle
}

android_apk() { echo "$BUILD_DIR/$1/android-build/build/outputs/apk/debug/android-build-debug.apk"; }
android_pkg() { grep -o 'package="[^"]*"' "$BUILD_DIR/$1/android-build/AndroidManifest.xml" | cut -d'"' -f2; }
ACTIVITY=org.qtproject.qt.android.bindings.QtActivity

android_run() {
    local adb="$ANDROID_SDK_ROOT/platform-tools/adb"
    [ "$("$adb" get-state 2>/dev/null)" = device ] || fail "no emulator/device attached (adb devices)"

    # ---- Unit tests: Qt writes QtTest output to logcat ----
    say "Android: running unit tests"
    local pkg; pkg="$(android_pkg unittests)"
    "$adb" install -r "$(android_apk unittests)" >/dev/null
    "$adb" logcat -c
    "$adb" shell am start -n "$pkg/$ACTIVITY" >/dev/null
    local log="$BUILD_DIR/unittests.log" waited=0 started=0
    while [ "$waited" -lt "$TEST_TIMEOUT" ]; do
        sleep 3; waited=$((waited + 3))
        "$adb" logcat -d -v raw -s QTestLib:I default:D >"$log" 2>/dev/null || true
        grep -qE 'All test cases passed|Error found' "$log" && break
        # Died without a verdict (crash before the end)? Give it a moment to start first.
        if "$adb" shell pidof "$pkg" >/dev/null 2>&1; then started=1
        elif [ "$started" = 1 ]; then break; fi
    done
    grep -E "$TEST_LINES" "$log" || true
    # The process aborts in Android's render thread as main() returns — after the
    # results are written — so judge by the output, not the exit status.
    grep -q "All test cases passed" "$log" || fail "Android unit tests did not pass (full log: $log)"

    # ---- Contacts: must start and stay up with no QML errors ----
    say "Android: Contacts smoke test"
    pkg="$(android_pkg contacts)"
    "$adb" install -r "$(android_apk contacts)" >/dev/null
    "$adb" logcat -c
    "$adb" shell am start -n "$pkg/$ACTIVITY" >/dev/null
    sleep 20                                    # first launch seeds 10,000 contacts
    local clog="$BUILD_DIR/contacts.log"
    "$adb" logcat -d >"$clog" 2>/dev/null || true
    local alive=0; "$adb" shell pidof "$pkg" >/dev/null 2>&1 && alive=1
    "$adb" shell am force-stop "$pkg"
    if grep -E '\.qml:[0-9]+|failed to load component' "$clog"; then
        fail "Contacts reported QML errors (full log: $clog)"
    fi
    [ "$alive" = 1 ] || fail "Contacts exited early (full log: $clog)"
    echo "Contacts started cleanly."
}

# =============================================================================

IOS_SIM_ARCH="${IOS_SIM_ARCH:-x86_64}"
UNITTESTS_PRO="$ROOT/tests/unittests/unittests.pro"
CONTACTS_PRO="$ROOT/examples/contacts/contacts.pro"

if [ "$MODE" != --run-only ]; then
    "${PLATFORM}_build" unittests "$UNITTESTS_PRO"
    "${PLATFORM}_build" contacts  "$CONTACTS_PRO"
fi
if [ "$MODE" != --build-only ]; then
    "${PLATFORM}_run"
    say "All $PLATFORM checks passed"
fi
