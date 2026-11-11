# Extensions your app ships

Your app can come with Chrome extensions of its own, such as an ad blocker you offer, or helper code that works inside pages. You list them at launch, Wren Runtime installs them in the Profiles you choose, and only your app can remove them. They're ordinary extensions, with their buttons in your toolbar and a place on Chrome's extensions page.

This page also covers managing a Profile's extensions from your app, Manifest V2 extensions, installing someone's extensions from the Chrome Web Store, and letting an extension talk to your app. The code is in Swift. The [table at the end](#the-names-in-swift-rust-and-c) gives each name in Rust and in C.

## Two kinds

|  | Bundled | Internal |
| --- | --- | --- |
| What it is | A `.crx` file, installed as Chrome installs one | A folder, loaded the way Chrome loads its own built-in extensions |
| On Chrome's extensions page | Yes, marked as managed by your app | No |
| Can be turned off | Yes, by the person or your app | No |
| Use it for | Features people see, such as an ad blocker | Code your app trusts as it trusts itself |

Most apps want bundled extensions, and most of this page is about them. [Internal extensions](#internal-extensions) have a section of their own.

## Listing your extensions

Put each extension's `.crx` file in your app bundle, and list it in `Startup` with an absolute path:

```swift
var startup = Startup(userDataDirectory: directory, keychain: .system)
startup.extensions = [
    AppExtension(path: resources.appendingPathComponent("Blocker.crx").path, enabledOnInstall: false),
]
```

List the same extensions at every launch, because no launch remembers the last one's list.

Before Chromium starts, Wren Runtime reads each file's extension ID and version. A missing file, a file that isn't a CRX3 file, a manifest without a valid version, or two files with the same ID stop the launch, and `run` returns `.invalidStartup` with the reason on stderr. Chromium checks each file's signature later, when it installs it, and leaves out any extension it refuses, with a message in its log. To check a file before you ship it, such as one your build downloads, run [`wren extension-id`](wren-cli.md#wren-extension-id).

## Choosing which Profiles get them

By default, every Profile gets every extension you list. To choose, set `profileExtensions`. Wren Runtime calls it once for each persistent or ephemeral Profile, as the Profile loads, with an entry for each extension in the order you listed them. Each entry's `gets` starts as true.

```swift
delegate.profileExtensions = { request in
    if request.kind == .ephemeral || !settings.blocksAds(inProfile: request.profileID) {
        request.extensions[0].gets = false
    }
}
```

It runs inside Chromium's call, before there's a `Profile` for the ID, and sometimes before `ready`. So read only your own settings there, and make no Wren Runtime calls. An incognito Profile has what its original Profile has, and isn't asked about. A Profile's answer holds until the next launch.

## What happens at each launch

Once Chromium has loaded a Profile's extensions, Wren Runtime brings them in line with your list, in the background:

- An extension the Profile gets, but doesn't have, is installed. One the Profile has at a lower version is updated, and keeps its data.
- One the Profile has at the same or a higher version stays as it is.
- One that Wren Runtime installed in an earlier launch, but that the Profile no longer gets, is removed, with its data.

Extensions the person installed themselves are never touched. Once your app has read a Profile's extensions, `.extensions` changes report each install and removal.

Chromium updates your extension as it does any other, from the `update_url` in its manifest, or from the Chrome Web Store without one. A later launch that lists a lower version keeps the newer one. If the person had installed an extension with the same ID, it becomes your app's.

## Turning an extension on and off

`enabledOnInstall` says whether the extension is on when Wren Runtime installs it, and `profileExtensions` can say otherwise for one Profile with an entry's `enabled`. An extension installed off doesn't run until something turns it on. After the install, the Profile keeps whatever the person or your app chose, across launches and updates.

So an opt-in feature is an extension listed with `enabledOnInstall: false`, with a menu item or setting that calls `profile.setExtensionEnabled`. Turning it off keeps its data. To remove it and its data, stop giving it to the Profile.

You can read and change a Profile's extensions with or without a window, as Chrome's extensions page does. `profile.extensionIDs` lists them, and `profile.extensionState` gives each one's state. A Browser's Profile is `browser.profile`, and an incognito Profile gives its original's.

## Incognito

An extension runs in incognito windows only once it's allowed there, as with Allow in Incognito on Chrome's extensions page. Chrome leaves that to the person, and so does Wren Runtime unless you say otherwise. `allowedInIncognitoOnInstall` allows it from the install, and an entry's `allowedInIncognito` in `profileExtensions` can say otherwise for one Profile. After the install, the Profile keeps whatever the person or your app chose.

```swift
startup.extensions = [
    AppExtension(
        path: resources.appendingPathComponent("Blocker.crx").path, enabledOnInstall: false,
        allowedInIncognitoOnInstall: true),
]
```

An extension's state has `allowedInIncognito`, and `canAllowInIncognito`, which is false when its manifest refuses incognito, or when the Profile can't go incognito. `profile.setExtensionAllowedInIncognito` changes the setting, and Chromium then restarts the extension if it's on, as Chrome's page does. `.extensions` changes report every change, from your app or from the page.

## Pinning, and requests for site access

Pinned extensions are the ones whose buttons Chrome shows in its toolbar, in order. They belong to the Profile, for all its windows, so you read and change them through the Profile:

```swift
try profile.setExtensionPinned(blockerID, true)
try profile.moveExtension(blockerID, to: 0)
```

`profile.pinnedExtensions` lists their IDs in order. `setExtensionPinned` pins an extension at the end or unpins it, and `moveExtension(_:to:)` moves a pinned one, to the end for an index past it. Each Browser's extension state also has `pinned`, for drawing its toolbar.

As in Chrome, an incognito Profile lists its original's pins and can't change them, and a pin set by policy stays. An extension that's off is left out of the list and keeps its place, and pinning or moving it throws `.notAllowed`. So does pinning an extension in the moment after it loads, before Chromium has given it its button, and a `.extensions` change reports when it has one.

An extension's state also has `showsRequests`, which says whether it can ask in the toolbar for access to a site, as set on Chrome's extensions page. `profile.setExtensionShowsRequests` changes it. `.extensions` changes report every change to pins and to `showsRequests`, whether from your app or from Chrome's menus.

## What people can do with your extension

Nobody can remove your app's extension but your app. Chrome's extensions page greys out Remove, and the extension's own `chrome.management.uninstallSelf` fails. The person can still turn it off, as with any extension.

The page says the extension is managed by your app, using your `productName`, in the person's language, with your `productIconPNG` as its badge. Without them, it says Chromium, with Chromium's logo. Safety Check doesn't ask anyone to remove it for not coming from the Chrome Web Store, though its other warnings, such as for malware, still apply.

## Packing a .crx

Pack the extension's folder with [`wren pack-extension`](wren-cli.md#wren-pack-extension). The first time, `--new-key` makes a private key:

```sh
wren pack-extension Blocker --key ~/keys/blocker.pem --new-key --output Blocker.crx
```

Keep the key somewhere safe, outside the extension's folder and out of your app, and pack every later version with it, without `--new-key`. The key decides the extension's ID, so a new key makes a different extension, which loses the old one's data. Raise the manifest's `version` with each new file you ship, or Wren Runtime leaves the installed one in place.

A `.crx` made by Chrome's Pack Extension, or one an extension's author publishes, works the same way.

## Manifest V2 extensions

Chromium no longer runs Manifest V2 extensions. It refuses to install them, and turns off any that are installed. `manifestV2` lets some run again:

- `.none`, the default, behaves as Chrome does.
- `.listed` runs the extensions whose IDs are in `manifestV2IDs`, yours or the person's.
- `.all` runs every one, including any unpacked extension someone loads.

```swift
startup.extensions = [
    AppExtension(path: resources.appendingPathComponent("uBlock0.crx").path, enabledOnInstall: false)
]
startup.manifestV2 = .listed
startup.manifestV2IDs = ["fkgkibajhfbepljeaefdnfnegdcjomkh"]
```

A Manifest V2 extension then has all of Manifest V2, including `webRequestBlocking`, which content blockers such as uBlock Origin need. The Chrome Web Store still refuses Manifest V2 extensions, whatever the setting. Prefer `.listed`, because it runs only what you chose.

## Internal extensions

An internal extension is code your app runs inside Chromium as an extension, which nobody else manages. It's a folder whose `manifest.json` has a `key`, the public key that decides its ID:

```swift
startup.extensions = [
    AppExtension(kind: .internal, path: resources.appendingPathComponent("Helper").path)
]
```

Wren Runtime loads it the way Chrome loads its own built-in extensions, such as its PDF viewer, into each Profile that gets it, from its folder, at every launch that lists it. It's never installed, so there's nothing to remove. A launch that doesn't give it to a Profile just doesn't load it there, and its data stays in the Profile. `enabledOnInstall` and `allowedInIncognitoOnInstall` don't apply, and it always runs in incognito windows.

A built-in extension has more rights than any other:

| What | An internal extension |
| --- | --- |
| Who sees it | It isn't on Chrome's extensions page, or in `extensionIDs`. Its button and side panel work as any extension's do. |
| Who can turn it off | Nobody, not even your app. Nobody can limit the sites it reads. |
| Extension APIs | It passes every API's checks, so it has Chromium's private APIs, such as `settingsPrivate`, and others such as `processes`, `dns` and `hid`. |
| Pages | It can script pages other extensions can't, and use `chrome://resources` and `chrome://theme`. It can let every page message it, through `externally_connectable`. |
| Your schemes | Its content scripts can match your app's own schemes when a pattern names one, such as `app://ui/*`. `<all_urls>` doesn't reach them, and other extensions can't name them. |

So ship an internal extension only for code your app trusts as it trusts itself.

Its ID comes from a key pair. Only the public half goes in the manifest, in base64 or as a PEM block with its BEGIN and END lines, and the folder is never signed:

```sh
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out helper.pem
openssl pkey -in helper.pem -pubout -outform DER | base64
```

Keep `helper.pem` only if you might later pack the extension as a `.crx` with the same ID. `wren extension-id Helper` prints the ID the key gives.

## Installing someone's extensions from the Chrome Web Store

When someone moves to your browser from another, you can install their Chrome Web Store extensions by ID with `profile.installFromWebStore`, as Chrome installs someone's synced extensions on a new Mac. They're the person's own, not your app's. Each comes in turned on, with the permissions it asks for, and the person can turn it off or remove it. Chromium shows no dialog, so ask first, and show the permissions if you like.

```swift
profile.installFromWebStore(ids) { result in
    guard case .success(let results) = result else { return }
    for (id, installed) in zip(ids, results) where installed != .installed {
        report(id, installed)
    }
}
```

The completion runs once every ID is done, with a result for each, in order: installed, already installed, not found in the store, not allowed by policy or Chrome's blocklist, or failed. `.extensions` changes report each install. An incognito Profile installs into its original. If the Profile closes first, the completion fails with `.closed`, and whatever was installed stays.

## Letting an extension talk to your app

Chrome lets an extension talk to a program on the Mac through native messaging. `chrome.runtime.connectNative` opens a port to a named host, and `chrome.runtime.sendNativeMessage` sends one message and waits for the answer. In Chrome, the host is a program Chromium starts. With Wren Runtime, your app can be the host instead, and answer inside its own process. No program starts, and only the extensions you name can reach it.

Name the host at launch, with the extensions that may reach it:

```swift
startup.nativeHosts = [NativeHost(name: "com.example.app", extensionIDs: [helperID])]
```

After `ready`, set the host's handler. It runs on the main thread for each port's events: opened, each message as JSON text, and closed.

```swift
try WrenApp.setNativeHostHandler("com.example.app") { port, event in
    guard case .message(let json) = event else { return }
    try? port.send(answer(to: json, from: port.extensionID))
}
```

The extension needs the `nativeMessaging` permission, and calls the host by name, as it would in Chrome:

```js
const reply = await chrome.runtime.sendNativeMessage("com.example.app", {
  ask: "status",
});
```

The extension sees the same errors Chrome would give it:

| When | The extension's error |
| --- | --- |
| It isn't in the host's `extensionIDs` | "Access to the specified native messaging host is forbidden." |
| Your app has set no handler for the host | "Specified native messaging host not found." |
| Your app closes the port with `port.close()` | "Native host has exited." |
| A policy blocks the host | "Access to the native messaging host was disabled by the system administrator." |

- **Message sizes.** Each message from your app can be up to 1 MiB, as for any host, and `port.send` refuses a larger one, or text that isn't JSON, with `.invalidArgument`. An extension's message can be up to 64 MiB.
- **Names.** A host's name uses lower-case letters, digits, "." and "\_", with no "." at either end or next to another. Each host lists at least one extension ID, without wildcards. A bad name or ID, or two hosts with the same name, stop the launch with `.invalidStartup`.
- **Ports.** A port gives the extension's ID and its Profile, and an extension in an incognito Profile gives the Profile it came from. Each port ends with one `.closed`, whichever side closed it, and its calls then fail with `.closed`.
- **Treat messages as input.** They come from an extension, so check them as you'd check a URL a page asks for.
- **Policies.** The `NativeMessagingBlocklist` and `NativeMessagingAllowlist` policies apply to your app's hosts. `NativeMessagingUserLevelHosts` doesn't, because your hosts are part of your app.
- **Hosts on disk.** A name your app doesn't declare goes to the hosts on disk, as in Chromium, and one it declares hides a host on disk with that name. Chromium looks for a person's hosts in `NativeMessagingHosts` in your app's data folder, so a host a program installed for Chrome, such as a password manager's, isn't found. Hosts in `/Library/Application Support/Chromium/NativeMessagingHosts` are.

## The names in Swift, Rust and C

| Swift | Rust | C |
| --- | --- | --- |
| `Startup.extensions`, `AppExtension` | `Startup::extensions`, `AppExtension::bundled` | `WrenStartup.extensions`, `WrenAppExtension` |
| `.internal` | `AppExtension::internal` | `WREN_APP_EXTENSION_INTERNAL` |
| `enabledOnInstall`, `allowedInIncognitoOnInstall` | `enabled_on_install`, `allowed_in_incognito_on_install` | `enabled_on_install`, `allowed_in_incognito_on_install` |
| `AppDelegate.profileExtensions`, `ProfileExtensionsRequest` | `App::profile_extensions`, `ProfileExtensionsRequest` | `on_profile_extensions`, `WrenProfileExtensionsRequest` |
| `Startup.manifestV2`, `Startup.manifestV2IDs` | `Startup::manifest_v2`, `Startup::manifest_v2_ids` | `WrenStartup.manifest_v2`, `WrenStartup.manifest_v2_ids` |
| `profile.extensionIDs` | `Profile::extension_ids` | `Wren_GetExtensionCount`, `Wren_CopyExtensionID` |
| `profile.extensionState` | `Profile::extension_state` | `Wren_GetExtensionState` |
| `profile.setExtensionEnabled` | `Profile::set_extension_enabled` | `Wren_SetExtensionEnabled` |
| `profile.setExtensionAllowedInIncognito` | `Profile::set_extension_allowed_in_incognito` | `Wren_SetExtensionAllowedInIncognito` |
| `profile.pinnedExtensions` | `Profile::pinned_extensions` | `Wren_GetPinnedExtensionCount`, `Wren_CopyPinnedExtensionID` |
| `profile.setExtensionPinned` | `Profile::set_extension_pinned` | `Wren_SetExtensionPinned` |
| `profile.moveExtension(_:to:)` | `Profile::move_extension` | `Wren_MoveExtension` |
| `profile.setExtensionShowsRequests` | `Profile::set_extension_shows_requests` | `Wren_SetExtensionShowsRequests` |
| `profile.installFromWebStore`, `WebStoreInstallResult` | `Profile::install_from_web_store`, `WebStoreInstallResult` | `Wren_InstallFromWebStore`, `WrenWebStoreInstallResult` |
| `Startup.nativeHosts`, `NativeHost` | `Startup::native_hosts`, `NativeHost` | `WrenStartup.native_hosts`, `WrenNativeHost` |
| `WrenApp.setNativeHostHandler`, `NativePortEvent` | `MainThread::set_native_host_handler`, `NativePortEvent` | `Wren_SetNativeHostHandler`, `WrenNativePortEvent` |
| `port.send`, `port.close` | `NativePort::send`, `NativePort::close` | `Wren_SendNativePortMessage`, `Wren_CloseNativePort` |
