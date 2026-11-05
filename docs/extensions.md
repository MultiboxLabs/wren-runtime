# Extensions your app ships

An app can ship Chrome extensions of its own, such as an ad blocker it offers. It lists them in `Startup` at every launch, and Wren installs them in the Profiles the app chooses. They are ordinary extensions, installed as Chromium installs a `.crx` file the user opens. The user sees them on Chrome's extensions page, with their buttons, but cannot remove them. The rules are in [`WrenExtensions.h`](../include/WrenExtensions.h), under "App extensions". The [table at the end](#the-names-in-swift-rust-and-c) gives each name in Rust and in C.

There are two kinds. A bundled extension, the usual kind, is a `.crx` file the user sees. An [internal extension](#internal-extensions) is a folder, loaded as one of Chromium's hidden component extensions. Either may be a [Manifest V2](#manifest-v2) extension if the app says so, and either can [talk to the app](#talking-to-the-app).

## Listing them

Each extension is a `.crx` file in your app's bundle, with an absolute path. List the same ones at every launch. No launch inherits the list of the one before.

```swift
var startup = Startup(userDataDirectory: directory, keychain: .system)
startup.extensions = [
    AppExtension(path: resources + "/Blocker.crx", enabledOnInstall: false),
]
```

Wren reads each file's extension ID and version before Chromium starts. A missing file, a file that is not a CRX3 file, a manifest without a valid version, or two files with one ID stop the launch. `run` then returns `.invalidStartup`, and Wren prints the reason to stderr. Chromium checks each file's signature later, when it installs it. Chromium logs a file it refuses, such as one whose manifest it rejects, and leaves that extension out. To check a file's ID and signatures before you ship it, such as one your build fetches, run [`wren extension-id`](wren-cli.md#wren-extension-id).

## Choosing the Profiles

Without `profileExtensions`, every Profile gets every extension you list, each as its `AppExtension` says. Set it to choose. Wren calls it once for each persistent or ephemeral Profile in a launch, as the Profile loads. The request names the Profile by its ID, and says whether it is `.persistent` or `.ephemeral`. It has one entry for each extension, in the order of `Startup.extensions`. Each entry's `gets` starts as true.

```swift
delegate.profileExtensions = { request in
    if request.kind == .ephemeral || !settings.blocksAds(inProfile: request.profileID) {
        request.extensions[0].gets = false
    }
}
```

It runs inside Chromium's call, before Wren has a `Profile` for that ID, and may run before `ready`. So it reads only your own state, and makes no Wren calls. An off-the-record Profile has what its original has, and Wren never asks about it. A Profile's answer holds until the next launch.

## What Wren does in each Profile

Once Chromium has loaded a Profile's extensions, Wren works through the list:

- It installs each extension the Profile gets that it does not have, or has at a lower version. An update keeps the extension's data.
- An extension the Profile has at the same or a higher version stays as it is.
- It uninstalls each extension it installed in an earlier launch that the Profile no longer gets. Its data goes with it. Extensions the user installed are never touched.

This happens in the background. Once your app has read a Profile's extensions, `.extensions` changes report each install and removal for that Profile, as they do any other.

Chromium updates an app's extension as it does any other, from the `update_url` in its manifest, or from the Chrome Web Store without one. A later launch that lists a lower version keeps the newer one. An extension the user installed with the same ID becomes your app's.

## On or off

`enabledOnInstall` says whether the extension is on when Wren installs it in a Profile. An entry's `enabled` in the request starts as that, and may say otherwise for one Profile. An extension installed off never runs until it is turned on. After the install, the Profile keeps whatever the user or your app chose, across launches, and the request's values do nothing for it, at an update too.

So an opt-in feature is an extension listed with `enabledOnInstall` false, and a menu item or setting that turns it on and off with `profile.setExtensionEnabled`. Turning it off keeps its data. To remove it and its data, stop giving it to the Profile.

A Profile's installed extensions are read and changed through the Profile, with or without a window, as on Chrome's extensions page. A Browser's Profile is `browser.profile`. An off-the-record Profile gives its original's.

## Allow in Incognito

An extension runs in the Profile's off-the-record Profiles only once it is allowed in incognito, as with Allow in Incognito on Chrome's extensions page. Chrome leaves that to the user, and so does Wren by default. `allowedInIncognitoOnInstall` allows it as Wren installs it, and an entry's `allowedInIncognito` in the request may say otherwise for one Profile. After the install, the Profile keeps whatever the user or your app chose.

```swift
startup.extensions = [
    AppExtension(
        path: resources + "/Blocker.crx", enabledOnInstall: false,
        allowedInIncognitoOnInstall: true),
]
```

An extension's state has `allowedInIncognito`, and `canAllowInIncognito`, which is false when its manifest refuses incognito or the Profile may not go off the record. Chrome's extensions page shows its switch only when that is true. `profile.setExtensionAllowedInIncognito` changes it. Chromium then reloads an extension that is on, as Chrome's page does, so its worker and pages start again. The page and your app change the same setting, and each change is reported as `.extensions`.

## Pins and requests

The pinned extensions are the ones whose buttons Chrome shows in its toolbar, in order. They are the Profile's, for all its windows, so your app reads and changes them through the Profile, with or without a window. `profile.pinnedExtensions` lists their IDs. `profile.setExtensionPinned` pins one last or unpins it, and `profile.moveExtension(_:to:)` moves a pinned one; an index past the end moves it last. Each Browser's action state reads `pinned` too, for drawing its toolbar.

```swift
try profile.setExtensionPinned(blockerID, true)
try profile.moveExtension(blockerID, to: 0)
```

As in Chrome, an off-the-record Profile cannot change them and lists its original's, and a pin set by policy stays. An extension that is off is left out of the list and keeps its place, and pinning or moving it throws `.notAllowed`. So does pinning an extension in the moment after it loads, before Chromium has given it its action; `.extensions` reports when it has one.

An extension's state also has `showsRequests`, whether it may show its requests for access to a site in the toolbar, as on Chrome's extensions page. `profile.setExtensionShowsRequests` changes it, for an extension on or off. Each change to the pins or to `showsRequests`, by your app or by the user in Chrome's menus, is reported as `.extensions`.

## What the user can do

The user cannot remove your app's extension. Chrome's extensions page greys out Remove, and the extension's own `chrome.management.uninstallSelf` fails. Only your app removes one, by no longer giving it to the Profile. The user may still turn it off on Chrome's extensions page, as with any extension.

The page says the extension is "Managed by" your app, in the user's language, by `Startup.productName`. Its badge is your app's icon, `Startup.productIconPNG`. Without them, it says Chromium, with Chromium's logo. Safety Check does not ask the user to remove it for not coming from the Chrome Web Store. Its other warnings, such as for malware, still apply.

## Making a .crx

Pack the extension's folder with [`wren pack-extension`](wren-cli.md#wren-pack-extension). The first pack makes a private key, with `--new-key`:

```sh
wren pack-extension Blocker --key ~/keys/blocker.pem --new-key --output Blocker.crx
```

Keep the key, outside the extension's folder and out of your app, and pack every later version with it, without `--new-key`. The key fixes the extension's ID. A new key makes a new extension, which loses the old one's data. Raise the manifest's `version` with each new file you ship, or Wren leaves the installed one in place.

A `.crx` made by Chrome's Pack extension, or one an extension's author publishes, works the same.

## Manifest V2

Chromium no longer runs Manifest V2 extensions. It refuses to install one, and turns off one that is installed. `manifestV2` lets some run again:

- `.none`, the default, as in Chrome.
- `.listed` runs the extensions whose IDs are in `manifestV2IDs`, yours or the user's.
- `.all` runs every one, an unpacked one the user loads among them.

```swift
startup.extensions = [AppExtension(path: resources + "/uBlock0.crx", enabledOnInstall: false)]
startup.manifestV2 = .listed
startup.manifestV2IDs = ["fkgkibajhfbepljeaefdnfnegdcjomkh"]
```

A Manifest V2 extension then has all of Manifest V2, `webRequestBlocking` among it, which content blockers such as uBlock Origin need. The Chrome Web Store still refuses Manifest V2 extensions, whatever the setting. Prefer `.listed`. It runs only what you chose, while `.all` also runs any old extension a user loads.

## Internal extensions

An internal extension is code your app runs inside Chromium as an extension, which the user never manages. It is a folder whose `manifest.json` has a `key`, the public key that gives its ID:

```swift
startup.extensions = [AppExtension(kind: .internal, path: resources + "/Helper")]
```

Wren adds it as a component extension, as Chrome's own PDF viewer is, to each Profile that gets it, as the Profile loads. Chromium loads it from its folder at every launch that lists it. It is never installed, so nothing is removed: a launch that does not give it to a Profile does not load it there. Its data stays in the Profile. `enabledOnInstall` and `allowedInIncognitoOnInstall` do not apply. It always runs in off-the-record Profiles.

A component extension has more rights than any other:

| What | A component extension |
| --- | --- |
| Seen by the user | Not on Chrome's extensions page, and not among `extensionIDs`. Its action and side panel work as any extension's. |
| Turned off or removed | Never, by the user or the app. Nobody can limit the sites it reads. It runs in off-the-record Profiles too. |
| Extension APIs | Every API's checks of allowlists, locations and manifest versions pass. So it has Chromium's private APIs, such as `settingsPrivate`, `metricsPrivate` and `safeBrowsingPrivate`, and `processes`, `dns` and `hid`. |
| Pages | It may script pages other extensions may not, and use `chrome://resources` and `chrome://theme`. It may let every page message it, through `externally_connectable`. |
| Your schemes | Its content scripts may match your app's own schemes, the ones in `schemes`, when the pattern names one, as `app://ui/*`. `<all_urls>` does not reach them. Other extensions cannot name them. |

So ship an internal extension only for code your app trusts as it trusts itself. Its ID comes from a key pair. Only the public half goes in the manifest, in bare base64 or as a PEM block with its BEGIN and END lines, and the folder is never signed:

```sh
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out helper.pem
openssl pkey -in helper.pem -pubout -outform DER | base64
```

Keep `helper.pem` only if you want the same ID for the extension packed as a `.crx` later. `wren extension-id Helper` prints the ID the key gives.

## The user's extensions from the store

An app that brings a user over from another browser can install the user's Chrome Web Store extensions by ID, with `profile.installFromWebStore`. Chrome installs a user's synced extensions on a new Mac the same way. They are the user's own, not your app's: each comes in on, with the permissions it asks for, and the user may turn it off or remove it on Chrome's extensions page, which lists it as from the store. Chromium shows no dialog, so ask the user first, and show the permissions if you wish.

```swift
profile.installFromWebStore(ids) { result in
    guard case .success(let results) = result else { return }
    for (id, installed) in zip(ids, results) where installed != .installed {
        report(id, installed)
    }
}
```

The completion runs once every ID is done, with one result for each, in order: installed, already installed, not found in the store, not allowed by policy or Chrome's blocklist, or failed. An ID given twice gets the same result twice. `.extensions` changes report each install. An off-the-record Profile installs into its original. If the Profile closes first, the completion fails with `.closed`, and what was installed stays.

## Talking to the app

An extension talks to a program on the Mac through Chrome's native messaging: `chrome.runtime.connectNative` opens a port to a named host, and `chrome.runtime.sendNativeMessage` sends one message and waits for one answer. In Chrome the host is a program Chromium starts from a manifest on disk. Your app can be the host instead. It names the host at startup, with the extensions that may reach it, and answers each port in its own process. No program starts, and nothing on the Mac but those extensions can reach it.

```swift
startup.nativeHosts = [NativeHost(name: "com.example.app", extensionIDs: [helperID])]
```

After `ready`, set the host's handler. It runs on the main thread, from the event loop, for each port's events: opened, each message as JSON text, and closed.

```swift
try WrenApp.setNativeHostHandler("com.example.app") { port, event in
    guard case .message(let json) = event else { return }
    try? port.send(answer(to: json, from: port.extensionID))
}
```

The extension needs the `nativeMessaging` permission, and calls the host by its name as it would call a host in Chrome:

```js
const reply = await chrome.runtime.sendNativeMessage("com.example.app", {
  ask: "status",
});
```

What the extension sees is what Chrome gives it:

| When | The extension's error |
| --- | --- |
| It is not in the host's `extensionIDs` | "Access to the specified native messaging host is forbidden." |
| The app has set no handler for the host | "Specified native messaging host not found." |
| The app closes the port (`port.close()`) | "Native host has exited." |
| Policy blocks the host, as for any host | "Access to the native messaging host was disabled by the system administrator." |

Each of the app's messages is at most 1 MiB, as any host's. `port.send` refuses a larger one, or text that is not JSON, with `.invalidArgument`. An extension's message may be up to 64 MiB.

A host's name follows Chromium's rule: lower-case letters, digits, "." and "\_", with no "." at either end or beside another. Each host lists at least one extension ID, and no wildcard, as Chromium's host manifests require. A bad name or ID, or two hosts with one name, stop the launch with `.invalidStartup`.

A port reports the extension's ID and its Profile. An extension in an off-the-record Profile reports the Profile it came from, as Chromium tells a host process. Each port ends with one `.closed`, whichever side closed it, and the port's calls then fail with `.closed`. A message comes from an extension, so treat it as input, as you treat a URL a page asks for.

The `NativeMessagingBlocklist` and `NativeMessagingAllowlist` policies apply to the app's hosts. `NativeMessagingUserLevelHosts` does not: the app's hosts are part of the app.

A name the app does not declare goes to the hosts on disk, as in Chromium, and a name it declares hides a host on disk with that name. Chromium reads a user's hosts from `NativeMessagingHosts` in the app's user data folder, so a host a program installed for Chrome, such as a password manager's, is not found. Hosts in `/Library/Application Support/Chromium/NativeMessagingHosts` are.

## The names in Swift, Rust and C

| Swift | Rust | C |
| --- | --- | --- |
| `Startup.extensions` | `Startup::extensions` | `WrenStartup.extensions` |
| `AppExtension` | `AppExtension`, `AppExtension::bundled` | `WrenAppExtension` |
| `.bundled` | `AppExtensionKind::Bundled` | `WREN_APP_EXTENSION_BUNDLED` |
| `.internal` | `AppExtension::internal` | `WREN_APP_EXTENSION_INTERNAL` |
| `Startup.manifestV2` | `Startup::manifest_v2` | `WrenStartup.manifest_v2` |
| `Startup.manifestV2IDs` | `Startup::manifest_v2_ids` | `WrenStartup.manifest_v2_ids` |
| `.none`, `.listed`, `.all` | `ManifestV2::None`, `Listed`, `All` | `WREN_MANIFEST_V2_NONE`, `_LISTED`, `_ALL` |
| `enabledOnInstall` | `enabled_on_install` | `enabled_on_install` |
| `allowedInIncognitoOnInstall` | `allowed_in_incognito_on_install`, `AppExtension::allowed_in_incognito` | `allowed_in_incognito_on_install` |
| `AppDelegate.profileExtensions` | `App::profile_extensions` | `WrenAppDelegate.on_profile_extensions` |
| `ProfileExtensionsRequest` | `ProfileExtensionsRequest` | `WrenProfileExtensionsRequest` |
| `ProfileExtension` | `ProfileExtension` | `WrenProfileExtension` |
| `Profile.id` | `Profile::id` | `Wren_CopyProfileID` |
| `profile.extensionIDs` | `Profile::extension_ids` | `Wren_GetExtensionCount`, `Wren_CopyExtensionID` |
| `profile.setExtensionEnabled` | `Profile::set_extension_enabled` | `Wren_SetExtensionEnabled` |
| `profile.setExtensionAllowedInIncognito` | `Profile::set_extension_allowed_in_incognito` | `Wren_SetExtensionAllowedInIncognito` |
| `profile.extensionState` | `Profile::extension_state` | `Wren_GetExtensionState` |
| `profile.pinnedExtensions` | `Profile::pinned_extensions` | `Wren_GetPinnedExtensionCount`, `Wren_CopyPinnedExtensionID` |
| `profile.setExtensionPinned` | `Profile::set_extension_pinned` | `Wren_SetExtensionPinned` |
| `profile.moveExtension(_:to:)` | `Profile::move_extension` | `Wren_MoveExtension` |
| `profile.setExtensionShowsRequests`, `ExtensionState.showsRequests` | `Profile::set_extension_shows_requests`, `ExtensionState::shows_requests` | `Wren_SetExtensionShowsRequests`, `WrenExtensionState.shows_requests` |
| `.extensions` | `ChangeKind::Extensions` | `WREN_CHANGE_EXTENSIONS` |
| `profile.installFromWebStore` | `Profile::install_from_web_store` | `Wren_InstallFromWebStore` |
| `WebStoreInstallResult` | `WebStoreInstallResult` | `WrenWebStoreInstallResult` |
| `Startup.nativeHosts`, `NativeHost` | `Startup::native_hosts`, `NativeHost` | `WrenStartup.native_hosts`, `WrenNativeHost` |
| `WrenApp.setNativeHostHandler` | `MainThread::set_native_host_handler` | `Wren_SetNativeHostHandler` |
| `NativePortEvent` | `NativePortEvent` | `WrenNativePortEvent` |
| `port.profile`, `port.extensionID` | `NativePort::profile`, `NativePort::extension_id` | `Wren_GetNativePortProfile`, `Wren_CopyNativePortExtensionID` |
| `port.send`, `port.close` | `NativePort::send`, `NativePort::close` | `Wren_SendNativePortMessage`, `Wren_CloseNativePort` |
