# Updating your app with Sparkle

Your browser can update itself the way Chrome does. It checks and downloads in the background, shows its progress on Chromium's About page, and relaunches into the new version with every window restored. Wren Runtime doesn't download or install updates itself. Your app uses an updater of its own, which on a Mac is usually [Sparkle](https://sparkle-project.org), and Wren Runtime connects it to Chromium:

- Chromium's About page, in Settings, shows what your updater is doing, with its Relaunch button once an update is ready.
- The About page asks your app to check for updates when it opens.
- Your app relaunches as Chrome does, keeping each Profile's session.
- Sparkle can take over the relaunch, and install the update before it starts the app again.

This page uses Sparkle 2, with code in Swift. Both samples update this way, in `Updates.swift` in the native sample and `host/src/updates.rs` in the web sample.

## Adding Sparkle

Download Sparkle's release archive from its GitHub releases, and put `Sparkle.framework` in your project, such as in a `Frameworks` folder. The archive's `bin` folder has Sparkle's tools for keys and feeds.

Your executable links the framework, and finds it in the app bundle's `Frameworks` folder at run time. In `Package.swift`:

```swift
// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "MyBrowser",
    platforms: [.macOS(.v13)],
    dependencies: [.package(path: ".wren/runtime/wren-swift")],
    targets: [
        .executableTarget(
            name: "MyBrowser",
            dependencies: [.product(name: "Wren", package: "wren-swift")],
            swiftSettings: [.unsafeFlags(["-F", "Frameworks"])],
            linkerSettings: [
                .unsafeFlags([
                    "-F", "Frameworks", "-Xlinker", "-rpath", "-Xlinker",
                    "@executable_path/../Frameworks",
                ])
            ])
    ],
    swiftLanguageVersions: [.v5]
)
```

In Rust, a `build.rs` does the same:

```rust
fn main() {
    println!("cargo:rustc-link-search=framework=Frameworks");
    println!("cargo:rustc-link-lib=framework=Sparkle");
    println!("cargo:rustc-link-arg=-Wl,-rpath,@executable_path/../Frameworks");
}
```

There's no Sparkle crate for Rust, so the web sample calls Sparkle through objc2, with `define_class!` for the delegate and `msg_send!` for Sparkle's controller.

Then `wren-app.json` bundles the framework, and gives Sparkle its settings:

```json
{
  "frameworks": ["Frameworks/Sparkle.framework"],
  "infoPlist": {
    "SUFeedURL": "https://example.com/appcast.xml",
    "SUPublicEDKey": "your public key",
    "SUEnableAutomaticChecks": true
  }
}
```

`wren bundle` copies the framework into the app and signs it the way Sparkle's documentation asks. See [The `wren` command](wren-cli.md).

## Keys and the feed

Sparkle checks every update against your public key. Make the key pair once with Sparkle's `generate_keys`, which keeps the private key in your login Keychain and prints the public key for `SUPublicEDKey`. Keep the private key safe, because whoever has it can sign updates for your app.

For each release, zip the bundled app with `ditto -c -k --keepParent MyBrowser.app MyBrowser-1.1.zip`, put the zip in a folder with the older ones, and run `generate_appcast` on the folder. It writes `appcast.xml`, signed with your key. Upload both to where `SUFeedURL` points.

Sparkle compares versions by `CFBundleVersion`, which comes from the manifest's `build`, or its `version` if there's no `build`.

## Showing Sparkle's progress on the About page

The About page shows no update line, and never asks your app to check, until you set an update state. Set one as soon as Sparkle starts, then keep it current from Sparkle's delegate:

| Sparkle tells its delegate | Set the state to |
| --- | --- |
| `updater(_:mayPerform:)`, as a check starts | `.checking` |
| `updater(_:willDownloadUpdate:with:)` | `.downloading`, with progress -1 if you don't track it |
| `updater(_:willInstallUpdateOnQuit:immediateInstallationBlock:)` | `.ready`, with the update's `displayVersionString` |
| `updater(_:didFinishUpdateCycleFor:error:)`, with no error or no update found | `.upToDate` |
| The same, with any other error | `.failed`, with the error's description |

When the About page opens, it asks for a check through `updateCheckRequested`. Answer with Sparkle's `checkForUpdatesInBackground()`, unless an update is already waiting or Sparkle is busy.

Chrome downloads updates without showing a window, and Sparkle can too. With `automaticallyDownloadsUpdates` on, background checks download what they find, without an alert. Then return true from `willInstallUpdateOnQuit`, and keep the block it gives you, which installs the update and starts the app again.

```swift
import AppKit
import Sparkle
import Wren

final class Updates: NSObject, SPUUpdaterDelegate {
    private var controller: SPUStandardUpdaterController?
    // Sparkle's block that installs the waiting update and starts the app again.
    private(set) var install: (() -> Void)?

    func start() {
        controller = SPUStandardUpdaterController(
            startingUpdater: true, updaterDelegate: self, userDriverDelegate: nil)
        controller?.updater.automaticallyDownloadsUpdates = true
        set(UpdateState(.checking))
    }

    // The About page asks for a check as it opens.
    func checkRequested() {
        guard let updater = controller?.updater, install == nil, !updater.sessionInProgress
        else { return }
        updater.checkForUpdatesInBackground()
    }

    private func set(_ state: UpdateState) {
        try? WrenApp.setUpdateState(state)
    }

    @objc(updater:mayPerformUpdateCheck:error:)
    func updater(_ updater: SPUUpdater, mayPerform updateCheck: SPUUpdateCheck) throws {
        if install == nil { set(UpdateState(.checking)) }
    }

    @objc(updater:willInstallUpdateOnQuit:immediateInstallationBlock:)
    func updater(
        _ updater: SPUUpdater, willInstallUpdateOnQuit item: SUAppcastItem,
        immediateInstallationBlock install: @escaping () -> Void
    ) -> Bool {
        self.install = install
        set(UpdateState(.ready, version: item.displayVersionString))
        return true
    }
}

let updates = Updates()
delegate.updateCheckRequested = { updates.checkRequested() }
```

Give each delegate method Sparkle's selector with `@objc(…)`, as above. A Swift name that nearly matches Sparkle's would otherwise leave the method uncalled, with only a compiler warning.

## Relaunching into the update

The About page's Relaunch button relaunches your app as Chrome does. To offer the same in your own UI, such as a Relaunch to Update button, call `WrenApp.relaunch()`. Every page gets its `beforeunload` first, and any of them can cancel.

If Chromium started the app again by itself, Sparkle could still be replacing the bundle. So once the quit is certain, in `quitCommitted`, hand the relaunch to Sparkle:

```swift
delegate.quitCommitted = {
    guard let install = updates.install, WrenApp.isRelaunching,
        (try? WrenApp.leaveRelaunchToApp()) != nil
    else { return }
    install()
}
```

`leaveRelaunchToApp` stops Chromium from starting the app, and keeps the rest of the relaunch. Each Profile's session is kept, and the next launch's last session is marked `relaunched`. Calling Sparkle's block makes Sparkle install the update once your app's process has gone, then start the app.

On a plain quit, there's nothing to do. Sparkle installs the update after the app quits, and doesn't start it again.

Neither Wren Runtime nor Sparkle tells you that a launch is the first after an update. `relaunched` is true after any relaunch, and false after an update installed at a quit. To know, store your version at each launch and compare.

## Signing

When `wren bundle` signs your app ad hoc, it signs Sparkle too, and the app runs. Sparkle accepts an update from one ad hoc build to another, because your EdDSA signature proves where it came from.

With a Developer ID, your app only loads code signed by the same team, so Sparkle has to be signed with your identity. `wren bundle --identity` signs it with the app. Notarise the bundle with `wren notarize`, then zip it for the feed.

## Testing an update on your Mac

1. Build the app at the new version, and zip it into a folder.
2. Build the old version again.
3. Run `generate_appcast` on the folder, and serve it with `python3 -m http.server 8123 --bind 127.0.0.1`.
4. Point the old build at your feed. Sparkle reads `SUFeedURL` from the user defaults before the bundle, so you don't need to change the bundle:

   ```sh
   defaults write com.example.mybrowser SUFeedURL http://127.0.0.1:8123/appcast.xml
   ```

Sparkle starts the app again without any arguments, so test without any your app depends on. Delete the default when you're done.

## The names in Swift, Rust and C

| Swift | Rust | C |
| --- | --- | --- |
| `WrenApp.setUpdateState`, `UpdateState` | `MainThread::set_update_state`, `UpdateState` | `Wren_SetUpdateState`, `WrenUpdateState` |
| `AppDelegate.updateCheckRequested` | `App::update_check` | `on_update_check` |
| `WrenApp.relaunch()` | `MainThread::relaunch` | `Wren_Relaunch` |
| `WrenApp.isRelaunching` | `MainThread::is_relaunching` | `Wren_IsRelaunching` |
| `WrenApp.leaveRelaunchToApp()` | `MainThread::leave_relaunch_to_app` | `Wren_LeaveRelaunchToApp` |
| `AppDelegate.quitCommitted` | `App::quit_committed` | `on_quit_committed` |
