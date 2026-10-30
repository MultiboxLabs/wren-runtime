# Updating your app with Sparkle

Wren does not check for updates or install them. Your app does that with an updater of its own. On the Mac that is usually [Sparkle](https://sparkle-project.org), and this page uses Sparkle 2. Wren gives the updater what Chrome's own has:

- Chromium's About page (Settings > About) shows what your updater is doing, with its Relaunch button once an update is ready.
- The About page asks your app to check as it opens.
- Your app relaunches as Chrome does, keeping each Profile's session.
- A relaunch can be handed to Sparkle, which installs the update before it launches the app again.

The calls are in [`WrenUpdates.h`](../include/WrenUpdates.h). Both of Wren's samples update this way: `Updates.swift` in the native sample and `host/src/updates.rs` in the web sample.

## Adding Sparkle

Download Sparkle's release archive from its GitHub releases, and keep `Sparkle.framework` in your project, in `Frameworks` for example. Its `bin` folder has the tools for keys and feeds.

The executable links the framework, and finds it in the bundle's `Frameworks` folder. Sparkle's install name starts with `@rpath`, so the executable needs that folder on its rpath. In `Package.swift`:

```swift
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
```

In Rust, a `build.rs` does the same:

```rust
fn main() {
    println!("cargo:rustc-link-search=framework=Frameworks");
    println!("cargo:rustc-link-lib=framework=Sparkle");
    println!("cargo:rustc-link-arg=-Wl,-rpath,@executable_path/../Frameworks");
}
```

Rust has no Sparkle crate. objc2 calls it: `define_class!` makes the delegate, and `msg_send!` makes `SPUStandardUpdaterController`.

Then `wren-app.json` bundles the framework and gives Sparkle its settings:

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

`wren bundle` copies the framework into `Contents/Frameworks` and signs it as Sparkle documents: each nested part before what holds it, with the hardened runtime, keeping each part's entitlements. See [The `wren` command](wren-cli.md).

## Keys and the feed

Sparkle checks each update against your public key. Make the key pair once with `generate_keys`, which keeps the private key in your login Keychain and prints the public key for `SUPublicEDKey`. Keep the private key safe: whoever has it can sign updates for your app.

For each release, zip the bundled app with `ditto -c -k --keepParent MyBrowser.app MyBrowser-1.1.zip`, put the zip in a folder with the older ones, and run `generate_appcast` on the folder. It writes `appcast.xml`, signed with the key. Upload both where `SUFeedURL` points.

Sparkle compares `CFBundleVersion`, which is the manifest's `build`, or its `version` if there is no `build`.

## Telling About what Sparkle does

Without a state, About has no update line, and never asks your app to check. Set one as soon as the updater starts, and keep it current from Sparkle's delegate:

| Sparkle tells the delegate | Set |
| --- | --- |
| `updater(_:mayPerform:)`, a check starts | `.checking` |
| `updater(_:willDownloadUpdate:with:)` | `.downloading`, with progress -1 if you do not track it |
| `updater(_:willInstallUpdateOnQuit:immediateInstallationBlock:)` | `.ready`, with the item's `displayVersionString` |
| `updater(_:didFinishUpdateCycleFor:error:)`, no error or no update | `.upToDate` |
| the same, with another error | `.failed`, with the error's description |

About's check comes through `updateCheckRequested`. Answer it with `checkForUpdatesInBackground()`, unless an update already waits or Sparkle is busy (`sessionInProgress`).

Chrome downloads its updates with no window of its own, and Sparkle can too. Set `automaticallyDownloadsUpdates` to true, and background checks download what they find, with no alert. Then return true from `willInstallUpdateOnQuit` and keep its block: it installs the update and launches the app again.

```swift
func start() {
    controller = SPUStandardUpdaterController(
        startingUpdater: true, updaterDelegate: self, userDriverDelegate: nil)
    controller?.updater.automaticallyDownloadsUpdates = true
    set(UpdateState(.checking))
}

func set(_ state: UpdateState) {
    try? WrenApp.setUpdateState(state)
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
```

Give each delegate method Sparkle's selector with `@objc(…)`. A Swift name that nearly matches Sparkle's would otherwise leave the method uncalled, with only a warning.

In Rust these are `MainThread::set_update_state` with an `UpdateState`, and `App::update_check`. In C, `Wren_SetUpdateState` and `on_update_check`.

## Relaunching into the update

About's Relaunch button relaunches as Chrome does. To offer the same in your own UI, such as a Relaunch to Update button, call `WrenApp.relaunch()`. Pages are asked first with beforeunload, and one may cancel.

If Chromium relaunched the app itself, Sparkle could still be replacing the bundle. So hand the relaunch to Sparkle once the quit is certain, in `quitCommitted`:

```swift
delegate.quitCommitted = {
    guard let install = updates.install, WrenApp.isRelaunching,
        (try? WrenApp.leaveRelaunchToApp()) != nil
    else { return }
    install()
}
```

`leaveRelaunchToApp` stops Chromium launching the app, and keeps everything else of the relaunch: each Profile's session is kept, and the next launch's last session reads `relaunched`. Calling the block tells Sparkle to install and launch the app. It installs once the app's process has gone.

On a plain quit, do nothing. Sparkle installs the update once the app has quit, and does not launch it.

Neither Wren nor Sparkle says that a launch is the first after an update. `relaunched` is true after any relaunch, and false after an update installed at a quit. To know, store your version at each launch and compare.

## Signing

Ad hoc, `wren bundle` signs Sparkle's framework too, and the app runs. Sparkle accepts an update from one ad hoc build to another, because the EdDSA signature proves it.

With a Developer ID, the app's library validation is on, so Sparkle must be signed by the same team. `wren bundle --identity` signs it with the app. Notarise the bundle with `wren notarize`, then zip it for the feed.

## Testing an update on your Mac

Build the app at the new version and zip it into a folder, then build the old version again. Run `generate_appcast` on the folder and serve it locally, with `python3 -m http.server 8123 --bind 127.0.0.1`. Then point the old build at it without editing the bundle. Sparkle reads `SUFeedURL` from the user defaults first:

```sh
defaults write com.example.mybrowser SUFeedURL http://127.0.0.1:8123/appcast.xml
```

Sparkle launches the app again with no arguments, so test without any your app depends on. Delete the default when you are done.
