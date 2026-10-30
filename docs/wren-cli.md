# The `wren` command

`wren` is the tool an app developer runs to get the Wren release an app is built with. It also turns the compiled executable into an app that carries Wren. It is one executable that needs nothing else installed. Each release ships it as `bin/wren`, and publishes it on its own as `wren`.

```text
wren --version
wren install [--manifest PATH]
wren bundle --executable PATH --manifest PATH --output PATH [--framework PATH] [--identity NAME] [--template PATH] [--incremental]
wren notarize PATH --keychain-profile NAME
wren notarize PATH --key FILE --key-id ID [--issuer UUID]
wren pack-extension FOLDER --key FILE [--new-key] --output PATH
wren extension-id PATH
wren help
```

It is the only command a release ships. Wren's own build tools are not part of it.

## Installing `wren`

Releases are published in the private repository `MultiboxLabs/wren-runtime`. With access to it, and the GitHub CLI signed in, put `wren` in a folder on your `PATH`:

```sh
gh release download --repo MultiboxLabs/wren-runtime --pattern wren \
  --output ~/.local/bin/wren
chmod +x ~/.local/bin/wren
```

That is the newest release's `wren`. Any release's `wren` works with the others.

## `wren install`

Run in a project's folder, beside its `wren-app.json`:

1. It reads the release the manifest names, such as `"wren": "0.3.0"`.
2. When that release is not on this Mac, it fetches its zip and `SHA256SUMS` from `MultiboxLabs/wren-runtime`. It signs in with `GH_TOKEN` or `GITHUB_TOKEN`, or else with the GitHub CLI's sign-in.
3. It refuses a zip that does not match `SHA256SUMS`.
4. It unpacks the zip into `~/.wren/runtimes/<version>`, or into `$WREN_RUNTIMES/<version>` when that is set, such as for a CI cache.
5. It links `.wren/runtime` in the project to that release, and keeps `.wren` out of git.

The project's build files name `.wren/runtime`. `Package.swift` depends on `.wren/runtime/wren-swift`, and `Cargo.toml` on `.wren/runtime/wren-rust`. A C build compiles with `-I .wren/runtime/include -F .wren/runtime`. To move to another release, change `"wren"` and run `wren install` again.

`.wren` is hidden and ignored by git, so tools that search a project and skip ignored files do not see the release in it. The release's documents are in `.wren/runtime/docs`, and its headers in `.wren/runtime/include`.

## From source to app

`swift build` makes an executable, not an app. A Wren app also needs the framework inside its bundle, helper apps for Chromium's child processes, and a signature macOS accepts for all of it. `wren bundle` does that.

```sh
# 1. Build the executable. Its Package.swift depends on the release's
#    Swift package: .package(path: ".wren/runtime/wren-swift")
wren install
swift build -c release

# 2. Bundle it, with the release the manifest names.
wren bundle \
  --executable .build/release/MyBrowser \
  --manifest wren-app.json \
  --output MyBrowser.app

# 3. Run it.
open MyBrowser.app
```

[Getting started](getting-started.md) goes through this with a first app.

The app's executable must be at `MyBrowser.app/Contents/MacOS`. It loads the framework from `../Frameworks` by a fixed path. So a bare executable outside a bundle does not start.

## `wren bundle`

| Option | Meaning |
| --- | --- |
| `--executable PATH` | The app's compiled executable. Required. |
| `--manifest PATH` | The app's manifest, `wren-app.json` (below). Required. |
| `--output PATH` | The `.app` to make. Replaced if it exists. Required. |
| `--framework PATH` | `Wren Framework.framework`. Defaults to `.wren/runtime`'s when the manifest names a release (which must be the one linked), else to the one in the release the command is in. |
| `--identity NAME` | A `codesign` identity. Defaults to `-`, an ad hoc signature. |
| `--template PATH` | The folder with Chromium's app `Info.plist` and localised strings. Found beside the framework (`app-template` in a release) unless given. |
| `--incremental` | Keep the bundle at `--output` if only the executable or the resources changed, and sign only the app. Ad hoc only. See [While you develop](#while-you-develop). |

It prints `Bundled <path>` and exits 0. The bundle it leaves has already passed `codesign --verify --deep --strict`.

### While you develop

`wren bundle` makes the whole bundle each time, which takes 2 to 3 seconds. With `--incremental` it keeps the bundle at `--output` when only the executable or the resources changed since the last `--incremental` bundle there. It copies those in and signs only the app, in under half a second. Otherwise it bundles in full, as it does without the flag. The result is the same file for file as a fresh bundle's.

```sh
swift build && wren bundle --incremental \
  --executable .build/debug/MyBrowser \
  --manifest wren-app.json --output MyBrowser.app
```

A second line says what it did:

- `Signed only the app`: the executable or a resource changed.
- `Nothing changed`.
- `Bundled in full:` and why. It bundles in full when there is no app at `--output`, or when the last bundle there was made without the flag. It also does when the framework, its template, the manifest, the icon, the app's own frameworks or `wren` itself changed. And it does when the app was signed again or changed since, or when a resource that shares a name with Chromium's own, such as an `.lproj` folder, changed.

It keeps a small record of each bundle in `~/Library/Caches/dev.multibox.wren-runtime.cli/bundles`, never in the app. The framework is compared by each file's size and time, not its content, as `make` does, and so are the app's frameworks. `--incremental` refuses any `--identity` but `-`, with exit status 2: a bundle signed for distribution is always made fresh, so the bundle you notarise is the one you checked.

### The manifest

`wren-app.json` holds what belongs to the app. Relative paths start from the manifest's folder.

```json
{
  "name": "MyBrowser",
  "displayName": "My Browser",
  "bundleId": "com.example.mybrowser",
  "version": "1.2.0",
  "build": "42",
  "icon": "AppIcon.png",
  "copyright": "Copyright 2026 Example Ltd.",
  "urlSchemes": [
    "mybrowser",
    { "name": "Web site URL", "schemes": ["http", "https"] }
  ],
  "documentTypes": [
    { "name": "HTML document", "types": ["public.html"] },
    { "name": "WebM video", "types": ["org.webmproject.webm"] }
  ],
  "importedTypes": [
    {
      "identifier": "org.webmproject.webm",
      "conformsTo": ["public.movie"],
      "description": "WebM video",
      "extensions": ["webm"],
      "mimeTypes": ["video/webm"]
    }
  ],
  "usageDescriptions": {
    "NSCameraUsageDescription": "My Browser lets sites you allow use the camera."
  },
  "resources": ["Extras"],
  "frameworks": ["Sparkle.framework"],
  "infoPlist": {
    "SUFeedURL": "https://example.com/appcast.xml"
  }
}
```

| Field | Required | Meaning |
| --- | --- | --- |
| `name` | yes | The executable's name in the bundle, `CFBundleName`, and the base of the helper apps' names. A file name; it may not end in " Helper" or ")". |
| `bundleId` | yes | The app's bundle ID, reverse-DNS. The helpers' IDs are derived from it. |
| `displayName` | no | The name macOS shows. Defaults to `name`. |
| `version` | no | `CFBundleShortVersionString`. Defaults to `1.0`. |
| `build` | no | `CFBundleVersion`. Defaults to `version`. |
| `icon` | no | The app's icon, for the Dock, Finder and notifications: an Icon Composer `.icon`, or a square PNG of 1024 pixels. Without it the app has macOS's generic icon. See below. |
| `copyright` | no | `NSHumanReadableCopyright`. |
| `urlSchemes` | no | URL schemes macOS offers the app for. None by default, so the app is not offered for web links. See below. |
| `documentTypes` | no | Uniform type identifiers the app opens. None by default. See below. |
| `importedTypes` | no | Uniform types the app opens that macOS may not know, such as WebM. See below. |
| `exportedTypes` | no | Uniform types the app owns. See below. |
| `usageDescriptions` | no | Text for macOS's permission prompts, by `Info.plist` key, in place of Chromium's. Each key ends in `UsageDescription`. |
| `entitlements` | no | The app's entitlements, in place of Chrome's: a list of keys, each set to true, or an object of keys and their values. See [Signing](#signing). |
| `provisioningProfile` | no | A provisioning profile, for entitlements Apple grants. See [Restricted entitlements and a provisioning profile](#restricted-entitlements-and-a-provisioning-profile). |
| `resources` | no | Files and folders copied into `Contents/Resources`. |
| `frameworks` | no | Frameworks the executable links, such as Sparkle's, copied into `Contents/Frameworks` and signed with the app (below). |
| `infoPlist` | no | Keys added to the app's `Info.plist`, such as Sparkle's feed and public key. It may not set a key `wren bundle` writes from another field, or a usage description. |
| `wren` | no | The Wren release the app is built with, such as `"0.3.0"`: what `wren install` links and `wren bundle` takes the framework from. |

A field of the wrong type, or a `name` or `bundleId` that cannot be used, stops the command with a message that names it.

### URL types, document types and type declarations

- **`urlSchemes`** holds schemes and URL types. The schemes given as strings go in one URL type, named `<bundleId>.url`. An object `{ "name", "schemes" }` is a URL type of its own, such as Chromium's "Web site URL" for `http` and `https`. A scheme may be in one URL type only.
- **`documentTypes`** holds uniform type identifiers and document types. A string is a type the app opens as a viewer. An object `{ "types", "name", "role" }` is a document type of one or more types, with `name` and `role` optional. `role` is `Viewer`, the default, `Editor`, `Shell` or `None`.
- **`importedTypes` and `exportedTypes`** declare uniform types, imported when another app owns the type and exported when the app does. Each is `{ "identifier", "conformsTo", "description", "extensions", "mimeTypes" }`, where `conformsTo` lists at least one type and the last three are optional. A type is either imported or exported, not both. A document type that macOS may not know, such as `org.webmproject.webm`, opens in the app only once it is declared.

They become `CFBundleURLTypes`, `CFBundleDocumentTypes`, `UTImportedTypeDeclarations` and `UTExportedTypeDeclarations`. `infoPlist` may not set these keys.

### The icon

- **An Icon Composer `.icon`** is what macOS 26 and later draw in their own style. It is layered, with the light, dark, clear and tinted looks Icon Composer shows. `wren bundle` compiles it with Xcode's `actool` into an asset catalog, `Assets.car`, and an `.icns` that earlier releases of macOS use. The icon takes the file's name, so `MyBrowser.icon` becomes `MyBrowser.icns`, named by `CFBundleIconName` and `CFBundleIconFile`. It needs Xcode 26 or later on the Mac that bundles, not only the Command Line Tools.
- **A PNG** is scaled to every size of an `.icns` with tools every Mac has. macOS 26 shows it as it is, on its standard backing shape, without the layered looks.

Either way, the icon also goes to the Alerts helper. macOS takes the icon of an alert-style notification from there.

Compiling an icon takes about a second, so `wren bundle` keeps each compiled icon in `~/Library/Caches/dev.multibox.wren-runtime.cli/icons`. It uses the kept one while the icon file, the app's minimum macOS version and the tools that compile it stay the same. It keeps the 16 icons used last. You can delete the folder at any time, and the next bundle compiles the icon again.

### What the bundle contains

- **The framework**, copied to `Contents/Frameworks/Wren Framework.framework`.
- **Helper apps named after the app**, inside the framework: `<name> Helper`, `<name> Helper (Renderer)`, `<name> Helper (GPU)` and `<name> Helper (Alerts)`. Their bundle IDs are `<bundleId>.helper`, `<bundleId>.helper.renderer` and `<bundleId>.framework.AlertNotificationService`. The Alerts helper takes the app's display name and icon, which macOS shows on alert-style notifications. The framework's Aperitif helpers are left out, because Chromium does not use them: their feature, `AperitifHelpers`, is off by default.
- **`Info.plist`**, from Chromium's, with the app's identity. Chromium's URL types, document types and type declarations are removed, so the app claims only what the manifest lists, and `infoPlist`'s keys are added. The key `WrenHelperName` tells the framework where the helpers are. Do not remove it.
- **Localised usage descriptions.** For every language Chromium has, its permission-prompt texts with the app's display name in place of "Chromium". Chromium picks its UI language from these folders, so they are all kept. A description given in the manifest replaces the localised one for that key, in every language.
- **The icon**, from the manifest's. That is `app.icns` from a PNG, or `Assets.car` and an `.icns` from an Icon Composer icon.
- **The app's own frameworks**, from `frameworks`, beside Wren's in `Contents/Frameworks`, as they are.

### Signing

Everything is signed from the inside out, as Chrome's own signer does it. The libraries and helpers come first, all at once, then the framework, then the app.

The app's own frameworks are signed as Sparkle documents for its framework. Each bundle and executable nested in one, such as an XPC service, a helper app or a tool, is signed before what holds it, with the hardened runtime, the entitlements it already has, and its own bundle ID. Then the framework is signed, before the app.

|  | Ad hoc (`--identity -`, the default) | Developer ID |
| --- | --- | --- |
| Hardened runtime | on | on |
| Library validation | off (no team to compare) | on, except the Renderer and GPU helpers |
| Renderer and GPU helpers | `allow-jit` | `allow-jit` |
| Added for development | `disable-library-validation`, `get-task-allow` (for debuggers) | nothing |
| Timestamp | none | Apple's timestamp server (needs a network) |
| Runs on | the Mac that made it | any Mac, once notarised |

The app's entitlements default to Chrome's: audio input, Bluetooth, camera, printing, USB, location and the photo library. They let Chromium ask macOS for those when a site asks the user. They grant nothing by themselves.

The manifest's `entitlements` replace them, on the app alone. The helpers keep the entitlements in the table above, and the app's own frameworks keep their own. A list sets each key to true. An object gives each key its value, which may be a string, a number, a list or an object:

```json
"entitlements": {
  "com.apple.security.device.camera": true,
  "com.apple.security.application-groups": ["group.com.example.mybrowser"]
}
```

### Restricted entitlements and a provisioning profile

macOS launches an app with some entitlements only if a provisioning profile embedded in it grants them. Passkeys in a browser, `com.apple.developer.web-browser.public-key-credential`, are one, and keychain access groups another. `wren bundle` treats every entitlement outside `com.apple.security.` as one of these. Make the profile in your Apple Developer account, for the app's bundle ID and your certificate, and name it in the manifest:

```json
"provisioningProfile": "MyBrowser.provisionprofile"
```

- **Signed with an identity,** the bundle holds the profile at `Contents/embedded.provisionprofile`. The app also gets `com.apple.application-identifier` and `com.apple.developer.team-identifier` from the profile, as Xcode adds them. If the manifest gives either, it must be the profile's.
- **Signed ad hoc,** the bundle leaves out the profile and every entitlement that needs one, and `wren bundle` prints what it left out. The app launches without them, so one manifest serves development and release.

Before it writes anything, `wren bundle` refuses a bundle that macOS would not launch, and says why: restricted entitlements with no profile, or a profile that does not grant an entitlement with the value the manifest gives, is for another bundle ID, is not for the signing identity's certificate, has expired, or is not for macOS. A profile's value ending in `*` grants every value that starts with what comes before it, and a list is granted when each item is.

It finds the identity's certificate as `codesign` does, among the valid code-signing identities that `security find-identity -v -p codesigning` lists: by its SHA-1, or by part of its name. If no identity matches, or more than one, it stops and says so. Give the SHA-1 to choose one.

An app with channels keeps a manifest for each, so each names its own profile.

An ad hoc signature changes with every build. So macOS asks again for anything it remembers per signature, the Keychain above all. Use the mock keychain in development: `.mock` in Swift, `Keychain::Mock` in Rust, `WREN_KEYCHAIN_MOCK` in C. Use the system keychain in a build signed with a Developer ID.

## `wren notarize`

`wren notarize` sends an app or a zip signed with a Developer ID to Apple, and waits for the answer.

```sh
# Once: store credentials in the keychain under a name of your choosing.
xcrun notarytool store-credentials my-notary

wren bundle --executable … --manifest wren-app.json --output MyBrowser.app \
  --identity "Developer ID Application: Example Ltd (TEAMID)"
wren notarize MyBrowser.app --keychain-profile my-notary
```

| Argument | Meaning |
| --- | --- |
| `PATH` | An `.app`, or a `.zip`. |
| `--keychain-profile NAME` | The profile `notarytool store-credentials` saved. The Apple ID and password stay in the keychain; `wren` never sees them. |
| `--key FILE --key-id ID` | In place of a profile: an App Store Connect API key, as its `.p8` file and key ID. `notarytool` reads the file; `wren` only passes its path. |
| `--issuer UUID` | The key's issuer ID. A team key has one; an individual key has none. |

Sign in one way or the other. An API key suits a build machine, which has no keychain profile. Make one at App Store Connect, under Users and Access, Integrations:

```sh
wren notarize MyBrowser.app --key AuthKey_ABC123DEFG.p8 \
  --key-id ABC123DEFG --issuer 69a6de70-0000-0000-0000-5bf3fb4eb1fc
```

A key can also be stored as a profile once, and used by name afterwards, with `xcrun notarytool store-credentials my-notary --key … --key-id … --issuer …`.

For an app, `wren notarize` zips a copy for the upload. Once Apple accepts it, it staples the ticket to the app, validates the staple, and asks Gatekeeper with `spctl --assess`. It ends by printing `Stapled; Gatekeeper accepts <path>`. A zip cannot carry a ticket. Gatekeeper asks Apple about a zip's contents when they are first opened.

An app that no Developer ID signed is refused at once, before anything is uploaded, because Apple would refuse every file in it. That includes an ad hoc signature, a development certificate, and no signature. A zip is not checked.

If Apple refuses the upload, the command prints Apple's log, which names each file and why, and exits 1. Submissions usually take a few minutes.

## `wren pack-extension`

`wren pack-extension` packs an extension's folder as a `.crx` file, for an app to list among the extensions it ships. See [Extensions your app ships](extensions.md).

```sh
# The first version: make the key.
wren pack-extension Blocker --key ~/keys/blocker.pem --new-key --output Blocker.crx
# Every later one: the same key.
wren pack-extension Blocker --key ~/keys/blocker.pem --output Blocker.crx
```

It prints the extension's ID and version. The key is an RSA private key in a PEM file, and it fixes the ID. So `wren` makes one only with `--new-key`, never replaces a file that exists, and refuses a key inside the extension's folder, which would be packed with it. It makes the key readable only by you. A key from Chrome's Pack extension works too. Hidden files, such as `.DS_Store`, are left out, as Chrome leaves them.

It checks only that `manifest.json` has a `version`. Chromium checks the rest when it installs the file.

## `wren extension-id`

`wren extension-id` prints an extension's ID, and nothing else, so a build can compare it with the ID the app expects:

```sh
test "$(wren extension-id Blocker.crx)" = fkgkibajhfbepljeaefdnfnegdcjomkh
```

For a `.crx`, the ID is the one its signed header declares. `wren` checks the file as Chromium does when it installs it: one of its keys must give that ID, and every signature must verify over the header and the archive. So a file that passes is one Chromium will install under that ID. A file that is not a CRX3 file, a declared ID that none of its keys gives, and a signature that fails each exit 1.

For a folder, the ID comes from the `key` in its `manifest.json`, as Wren reads an [internal extension's](extensions.md#internal-extensions). A folder with no `key` that Chromium can read exits 1, since Chromium would give that extension an ID from its path.

## Exit status

| Status | Meaning |
| --- | --- |
| 0 | Done. |
| 1 | It could not be done: a missing file, a bad manifest, `codesign` or Apple refused. The message says which. |
| 2 | The command line was wrong; usage is printed. |

## Things to know

- **The first launch of a new bundle is slow.** macOS scans a bundle it has not seen before when it first starts, before the framework loads. With a development build's framework the scan took about 4 seconds, and later launches a fraction of a second. A bundle made again from scratch at the same path is scanned again. `--incremental` keeps the bundle, so only its first launch waits.
- **Started from a terminal, the terminal is named.** macOS attributes permission prompts and notifications to the app responsible for the process. Run the executable directly from a shell, and prompts say the terminal's name. Start the app with `open`, or from Finder, and they say the app's.
- **An empty folder in Application Support.** A bundled app creates `~/Library/Application Support/<name>`, Chromium's default data folder, even when its data is elsewhere. Crash reports go to `Crashpad` inside the app's user-data directory.
- **One framework per app.** Each app carries its own copy, about 340 MB in release 0.3.0, as Electron apps do. It must be bundled with the release it was compiled against, because the framework accepts only its own API version.
- **Chromium's notices go with the app.** The framework is Chromium, whose licence and third-party licences ask that their notices go with every copy. A release has them in `LICENSES`: `Chromium.txt`, `third-party.txt`, and `credits.html`, the page an app's `chrome://credits` shows. Ship them with your app, for example in its About window or beside its own licence.

## Not done yet

- A disk image. `wren` makes the app, not a `.dmg`.
- Passkeys in the system keychain. `wren bundle` signs an app with the entitlement Apple grants to browsers on request, and with its profile, but passkeys have not been tried in a Wren app signed that way.
- Windows and Linux.
