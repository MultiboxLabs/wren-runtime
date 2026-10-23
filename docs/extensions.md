# Extensions your app ships

An app can ship Chrome extensions of its own, such as an ad blocker it offers.
It lists them in `Startup` at every launch, and Wren installs them in the
Profiles the app chooses. They are ordinary extensions, installed as Chromium
installs a `.crx` file the user opens. The user sees them on Chrome's extensions
page, with their buttons, but cannot remove them. The rules are in
[`WrenExtensions.h`](../include/WrenExtensions.h),
under "App extensions". The [table at the end](#the-names-in-swift-rust-and-c)
gives each name in Rust and in C.

There are two kinds. A bundled extension, the usual kind, is a `.crx` file the
user sees. An [internal extension](#internal-extensions) is a folder, loaded as
one of Chromium's hidden component extensions. Either may be a
[Manifest V2](#manifest-v2) extension if the app says so.

## Listing them

Each extension is a `.crx` file in your app's bundle, with an absolute path.
List the same ones at every launch. No launch inherits the list of the one
before.

```swift
var startup = Startup(userDataDirectory: directory, keychain: .system)
startup.extensions = [
    AppExtension(path: resources + "/Blocker.crx", enabledOnInstall: false),
]
```

Wren reads each file's extension ID and version before Chromium starts. A
missing file, a file that is not a CRX3 file, a manifest without a valid
version, or two files with one ID stop the launch. `run` then returns
`.invalidStartup`, and Wren prints the reason to stderr. Chromium checks each
file's signature later, when it installs it. Chromium logs a file it refuses,
such as one whose manifest it rejects, and leaves that extension out.

## Choosing the Profiles

Without `profileExtensions`, every Profile gets every extension you list. Set it
to choose. Wren calls it once for each persistent or ephemeral Profile in a
launch, as the Profile loads. `gets` has one entry for each extension, in the
order of `Startup.extensions`, and each starts as true.

```swift
delegate.profileExtensions = { profileID, gets in
    if !settings.blocksAds(inProfile: profileID) {
        gets[0] = false
    }
}
```

It runs inside Chromium's call, before Wren has a `Profile` for that ID, and may
run before `ready`. So it reads only your own state, and makes no Wren calls. An
off-the-record Profile has what its original has, and Wren never asks about it.
A Profile's answer holds until the next launch.

## What Wren does in each Profile

Once Chromium has loaded a Profile's extensions, Wren works through the list:

- It installs each extension the Profile gets that it does not have, or has at a
  lower version. An update keeps the extension's data.
- An extension the Profile has at the same or a higher version stays as it is.
- It uninstalls each extension it installed in an earlier launch that the
  Profile no longer gets. Its data goes with it. Extensions the user installed
  are never touched.

This happens in the background. `.extensionActions` changes report each install
and removal, as they do any other.

Chromium updates an app's extension as it does any other, from the `update_url`
in its manifest, or from the Chrome Web Store without one. A later launch that
lists a lower version keeps the newer one. An extension the user installed with
the same ID becomes your app's.

## On or off

`enabledOnInstall` says whether the extension is on when Wren installs it in a
Profile. An extension installed off never runs until it is turned on. After the
install, the Profile keeps whatever the user or your app chose, across launches.

So an opt-in feature is an extension listed with `enabledOnInstall` false, and a
menu item or setting that turns it on and off with
`browser.setExtensionEnabled`. Turning it off keeps its data. To remove it and
its data, stop giving it to the Profile.

## What the user can do

The user cannot remove your app's extension. Chrome's extensions page greys out
Remove, and the extension's own `chrome.management.uninstallSelf` fails. Only
your app removes one, by no longer giving it to the Profile. The user may still
turn it off on Chrome's extensions page, as with any extension.

The page says the extension is "Managed by" your app, in the user's language, by
`Startup.productName`. Its badge is your app's icon, `Startup.productIconPNG`.
Without them, it says Chromium, with Chromium's logo. Safety Check does not ask
the user to remove it for not coming from the Chrome Web Store. Its other
warnings, such as for malware, still apply.

## Making a .crx

Pack the extension's folder with
[`wren pack-extension`](wren-cli.md#wren-pack-extension). The first pack makes a
private key, with `--new-key`:

```sh
wren pack-extension Blocker --key ~/keys/blocker.pem --new-key --output Blocker.crx
```

Keep the key, outside the extension's folder and out of your app, and pack every
later version with it, without `--new-key`. The key fixes the extension's ID. A
new key makes a new extension, which loses the old one's data. Raise the
manifest's `version` with each new file you ship, or Wren leaves the installed
one in place.

A `.crx` made by Chrome's Pack extension, or one an extension's author
publishes, works the same.

## Manifest V2

Chromium no longer runs Manifest V2 extensions. It refuses to install one, and
turns off one that is installed. `manifestV2` lets some run again:

- `.none`, the default, as in Chrome.
- `.listed` runs the extensions whose IDs are in `manifestV2IDs`, yours or the
  user's.
- `.all` runs every one, an unpacked one the user loads among them.

```swift
startup.extensions = [AppExtension(path: resources + "/uBlock0.crx", enabledOnInstall: false)]
startup.manifestV2 = .listed
startup.manifestV2IDs = ["fkgkibajhfbepljeaefdnfnegdcjomkh"]
```

A Manifest V2 extension then has all of Manifest V2, `webRequestBlocking` among
it, which content blockers such as uBlock Origin need. The Chrome Web Store
still refuses Manifest V2 extensions, whatever the setting. Prefer `.listed`. It
runs only what you chose, while `.all` also runs any old extension a user loads.

## Internal extensions

An internal extension is code your app runs inside Chromium as an extension,
which the user never manages. It is a folder whose `manifest.json` has a `key`,
the public key that gives its ID:

```swift
startup.extensions = [AppExtension(kind: .internal, path: resources + "/Helper")]
```

Wren adds it as a component extension, as Chrome's own PDF viewer is, to each
Profile that gets it, as the Profile loads. Chromium loads it from its folder at
every launch that lists it. It is never installed, so nothing is removed: a
launch that does not give it to a Profile does not load it there. Its data stays
in the Profile. `enabledOnInstall` does not apply.

A component extension has more rights than any other:

| What                  | A component extension                                                                                                                                                                                               |
| --------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Seen by the user      | Not on Chrome's extensions page, and not among `extensionIDs`. Its action and side panel work as any extension's.                                                                                                   |
| Turned off or removed | Never, by the user or the app. Nobody can limit the sites it reads. It runs in off-the-record Profiles too.                                                                                                         |
| Extension APIs        | Every API's checks of allowlists, locations and manifest versions pass. So it has Chromium's private APIs, such as `settingsPrivate`, `metricsPrivate` and `safeBrowsingPrivate`, and `processes`, `dns` and `hid`. |
| Pages                 | It may script pages other extensions may not, and use `chrome://resources` and `chrome://theme`. It may let every page message it, through `externally_connectable`.                                                |

So ship an internal extension only for code your app trusts as it trusts itself.
Its ID comes from a key pair. Only the public half goes in the manifest, as
base64, and the folder is never signed:

```sh
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out helper.pem
openssl pkey -in helper.pem -pubout -outform DER | base64
```

Keep `helper.pem` only if you want the same ID for the extension packed as a
`.crx` later.

## The names in Swift, Rust and C

| Swift                           | Rust                                    | C                                          |
| ------------------------------- | --------------------------------------- | ------------------------------------------ |
| `Startup.extensions`            | `Startup::extensions`                   | `WrenStartup.extensions`                   |
| `AppExtension`                  | `AppExtension`, `AppExtension::bundled` | `WrenAppExtension`                         |
| `.bundled`                      | `AppExtensionKind::Bundled`             | `WREN_APP_EXTENSION_BUNDLED`               |
| `.internal`                     | `AppExtension::internal`                | `WREN_APP_EXTENSION_INTERNAL`              |
| `Startup.manifestV2`            | `Startup::manifest_v2`                  | `WrenStartup.manifest_v2`                  |
| `Startup.manifestV2IDs`         | `Startup::manifest_v2_ids`              | `WrenStartup.manifest_v2_ids`              |
| `.none`, `.listed`, `.all`      | `ManifestV2::None`, `Listed`, `All`     | `WREN_MANIFEST_V2_NONE`, `_LISTED`, `_ALL` |
| `enabledOnInstall`              | `enabled_on_install`                    | `enabled_on_install`                       |
| `AppDelegate.profileExtensions` | `App::profile_extensions`               | `WrenAppDelegate.on_profile_extensions`    |
| `Profile.id`                    | `Profile::id`                           | `Wren_CopyProfileID`                       |
| `browser.setExtensionEnabled`   | `Browser::set_extension_enabled`        | `Wren_SetExtensionEnabled`                 |
| `browser.extensionState`        | `Browser::extension_state`              | `Wren_GetExtensionState`                   |
| `.extensionActions`             | `ChangeKind::ExtensionActions`          | `WREN_CHANGE_EXTENSION_ACTIONS`            |
