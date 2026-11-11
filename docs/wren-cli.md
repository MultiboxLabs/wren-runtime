# The `wren` command

`wren` gets the Wren Runtime release your app is built with, and turns your compiled executable into a Mac app with Chromium inside it, signed and ready to notarise. It's a single executable with nothing else to install. Each release comes with it as `bin/wren`, and any release's `wren` works with the others.

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

This page is the reference for each command. [Getting started](getting-started.md) shows them in use with a first app.

## Installing `wren`

```sh
curl -fsSL https://wrenbrowser.com/runtime/cli/install.sh | sh
```

The script installs the newest release's `wren` into `~/.local/bin`, after checking it against the release's `SHA256SUMS`. It tells you if that folder isn't on your `PATH`. Releases come from the public repository `MultiboxLabs/wren-runtime`.

| Variable | What it changes |
| --- | --- |
| `WREN_VERSION` | Installs that release's `wren`, such as `0.5.0`, instead of the newest |
| `WREN_INSTALL_DIR` | Installs into that folder instead of `~/.local/bin` |
| `WREN_RELEASES_URL` | Downloads from a mirror of the releases page instead of GitHub |

## What's in a release

| Path | What it is |
| --- | --- |
| `Wren Framework.framework` | Chromium, with Wren Runtime inside. `wren bundle` copies it into your app. |
| `wren-swift` | The Swift SDK, the package `Wren`, with `CWren`, the C API underneath it |
| `wren-rust` | The Rust SDK, the crate `wren`, with `wren::sys`, the C API underneath it |
| `include` | The C headers, with a module map |
| `page` | `wren.d.ts`, the TypeScript types for a web view's page |
| `bin/wren` | This command |
| `docs`, `README.md`, `AGENTS.md` | These guides, and the release's introductions for people and for coding agents |
| `samples` | The two sample browsers' source |
| `app-template` | What `wren bundle` builds each app from |
| `LICENSES` | The licences and notices to ship with your app |
| `release.json` | Which build this is |

`Wren_GetVersionInfo` reports the same build as `release.json` while your app runs. The release itself isn't signed, and `wren bundle` signs your app when it makes it.

## `wren install`

Run it in your project's folder, beside its `wren-app.json`:

1. It reads the release the manifest names, such as `"wren": "0.5.0"`.
2. If that release isn't on this Mac yet, it downloads its zip and `SHA256SUMS` from `MultiboxLabs/wren-runtime`, and refuses a zip that doesn't match.
3. It unpacks the release into `~/.wren/runtimes/<version>`, or `$WREN_RUNTIMES/<version>` when that's set, such as for a CI cache.
4. It links `.wren/runtime` in your project to that release, and keeps `.wren` out of git.

It needs no sign-in. When there's a GitHub token, in `GH_TOKEN` or `GITHUB_TOKEN`, or from the GitHub CLI's sign-in, it uses it. Without one, GitHub allows 60 requests an hour from an address, and `wren install` makes three for each release.

Your build files point at `.wren/runtime`. `Package.swift` depends on `.wren/runtime/wren-swift`, `Cargo.toml` on `.wren/runtime/wren-rust`, and a C build compiles with `-I .wren/runtime/include -F .wren/runtime`. To move to another release, change `"wren"` in the manifest and run `wren install` again.

Because git ignores `.wren`, search tools that skip ignored files won't find the release. Its guides are in `.wren/runtime/docs`, and its headers in `.wren/runtime/include`.

## `wren bundle`

`swift build` or `cargo build` makes an executable, not an app. A Wren Runtime app also needs Chromium's framework inside its bundle, helper apps for Chromium's child processes, and a signature that macOS accepts for all of it. `wren bundle` makes all of that:

```sh
wren bundle \
  --executable .build/release/MyBrowser \
  --manifest wren-app.json \
  --output MyBrowser.app
```

| Option | What it does |
| --- | --- |
| `--executable PATH` | Your app's compiled executable. Required. |
| `--manifest PATH` | Your app's manifest, `wren-app.json`, described below. Required. |
| `--output PATH` | The `.app` to make. If it exists, it's replaced. Required. |
| `--framework PATH` | Which `Wren Framework.framework` to use. By default, the release linked at `.wren/runtime`, which must be the one the manifest names. Without a release in the manifest, the release `wren` came from. |
| `--identity NAME` | A `codesign` identity to sign with. By default, `-`, an ad hoc signature. |
| `--template PATH` | The folder with Chromium's app `Info.plist` and localised strings. By default, the release's `app-template`. |
| `--incremental` | Keep the bundle when only the executable or resources changed, and sign only the app. Ad hoc only. See [While you develop](#while-you-develop). |

It prints `Bundled <path>` and exits 0. The bundle has already passed `codesign --verify --deep --strict`.

Your executable has to stay at `MyBrowser.app/Contents/MacOS`, because it loads the framework from `../Frameworks`. On its own, outside the bundle, it won't start.

### While you develop

A full bundle takes 2 to 3 seconds. With `--incremental`, `wren bundle` keeps the bundle at `--output` when only the executable or the resources changed since the last `--incremental` bundle there. It copies them in and signs only the app, in under half a second, and the result is the same, file for file, as a fresh bundle.

```sh
swift build && wren bundle --incremental \
  --executable .build/debug/MyBrowser \
  --manifest wren-app.json --output MyBrowser.app
```

A second line says what it did:

- `Signed only the app`, when the executable or a resource changed.
- `Nothing changed`.
- `Bundled in full:`, with the reason. It bundles in full when there's no app at `--output`, or the last bundle there was made without the flag. It also bundles in full when the framework, its template, the manifest, the icon, your app's own frameworks or `wren` itself changed, when the app was signed again or changed since, or when a resource with the same name as one of Chromium's, such as an `.lproj` folder, changed.

It keeps a small record of each bundle in `~/Library/Caches/dev.multibox.wren-runtime.cli/bundles`, never in the app. It compares frameworks by each file's size and time, as `make` does, not by content. `--incremental` with any `--identity` other than `-` exits with status 2, because a bundle signed for distribution is always made fresh, so the bundle you notarise is the one you checked.

An incremental bundle also starts faster. macOS scans a bundle it hasn't seen before when it first starts, which took about 4 seconds with a development build's framework. A bundle made again from scratch at the same path gets scanned again, and an incremental one doesn't.

### The manifest

`wren-app.json` describes your app. Relative paths start from the manifest's folder.

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
  },
  "wren": "0.5.0"
}
```

| Field | Required | What it is |
| --- | --- | --- |
| `name` | Yes | The executable's name in the bundle, and `CFBundleName`. The helper apps' names start with it. It's a file name, and can't end in " Helper" or ")". |
| `bundleId` | Yes | Your app's bundle ID, in reverse-DNS form. The helpers' IDs are made from it. |
| `displayName` | No | The name macOS shows. By default, `name`. |
| `version` | No | `CFBundleShortVersionString`. By default, `1.0`. |
| `build` | No | `CFBundleVersion`. By default, `version`. |
| `icon` | No | Your app's icon, for the Dock, Finder and notifications. An Icon Composer `.icon`, or a square PNG of 1024 pixels. Without it, the app has macOS's generic icon. See [The icon](#the-icon). |
| `copyright` | No | `NSHumanReadableCopyright`. |
| `urlSchemes` | No | URL schemes macOS can send your app. None by default, so macOS doesn't offer your app for web links. See below. |
| `documentTypes` | No | File types your app opens. None by default. See below. |
| `importedTypes`, `exportedTypes` | No | File types your app opens that macOS may not know, such as WebM, and types your app owns. See below. |
| `usageDescriptions` | No | The text of macOS's permission prompts, by `Info.plist` key, instead of Chromium's. Each key ends in `UsageDescription`. |
| `entitlements` | No | Your app's entitlements, instead of Chrome's. See [Signing](#signing). |
| `provisioningProfile` | No | A provisioning profile, for entitlements Apple grants. See [Restricted entitlements](#restricted-entitlements-and-a-provisioning-profile). |
| `resources` | No | Files and folders to copy into `Contents/Resources`. |
| `frameworks` | No | Frameworks your executable links, such as Sparkle, copied into `Contents/Frameworks` and signed with the app. |
| `infoPlist` | No | Keys to add to the app's `Info.plist`, such as Sparkle's feed. It can't set a key that another field writes, or a usage description. |
| `wren` | No | The Wren Runtime release your app is built with, which `wren install` links and `wren bundle` takes the framework from. |

A field of the wrong type, or a `name` or `bundleId` that can't be used, stops the command with a message that names it.

### URL schemes and file types

- **`urlSchemes`** lists schemes and URL types. Schemes given as strings go in one URL type, named `<bundleId>.url`. An object, `{ "name", "schemes" }`, is a URL type of its own, such as "Web site URL" for `http` and `https`, which a browser needs. A scheme can be in only one URL type.
- **`documentTypes`** lists uniform type identifiers and document types. A string is a type your app opens as a viewer. An object, `{ "types", "name", "role" }`, is a document type of one or more types, where `name` and `role` are optional. `role` is `Viewer`, the default, `Editor`, `Shell` or `None`.
- **`importedTypes` and `exportedTypes`** declare uniform types, imported when another app owns the type, and exported when yours does. Each is `{ "identifier", "conformsTo", "description", "extensions", "mimeTypes" }`, where `conformsTo` lists at least one type and the last three are optional. A type is either imported or exported, not both. A document type macOS may not know, such as `org.webmproject.webm`, only opens in your app once it's declared.

These become `CFBundleURLTypes`, `CFBundleDocumentTypes`, `UTImportedTypeDeclarations` and `UTExportedTypeDeclarations`, which `infoPlist` can't set.

### The icon

- **An Icon Composer `.icon`** is what macOS 26 and later draw in their own style, with the light, dark, clear and tinted looks Icon Composer shows. `wren bundle` compiles it with Xcode's `actool` into an asset catalog and an `.icns` for older versions of macOS. The `.icns` takes the icon's file name, so `MyBrowser.icon` becomes `MyBrowser.icns`. Compiling it needs Xcode 26 or later, not only the Command Line Tools.
- **A PNG** is scaled to every size of an `.icns`, with tools every Mac has. macOS 26 shows it as it is, on its standard backing shape, without the layered looks.

Either way, the icon also goes to the Alerts helper, which is where macOS takes the icon of an alert-style notification from.

Compiling an icon takes about a second, so `wren bundle` keeps the 16 most recently used compiled icons in `~/Library/Caches/dev.multibox.wren-runtime.cli/icons`. It reuses one while the icon file, your app's minimum macOS version and the compiling tools stay the same. You can delete the folder at any time.

### What's in the bundle

- **The framework,** copied to `Contents/Frameworks/Wren Framework.framework`.
- **Helper apps named after your app,** inside the framework, which Chromium starts its processes from: `<name> Helper`, `<name> Helper (Renderer)`, `<name> Helper (GPU)` and `<name> Helper (Alerts)`, and the same four with `Aperitif` in the parentheses. Their bundle IDs start with your `bundleId`. The Alerts helpers take your display name and icon, which macOS shows on alert-style notifications.
- **`Info.plist`,** from Chromium's, with your app's identity and your `infoPlist` keys. Chromium's URL types, document types and type declarations are removed, so your app claims only what the manifest lists. Don't remove the key `WrenHelperName`, which tells the framework where the helpers are.
- **Permission prompt text in every language Chromium has,** with your display name in place of "Chromium". Chromium picks its UI language from these folders, so they're all kept. A description in the manifest replaces that key's text in every language.
- **Your icon,** as `app.icns` from a PNG, or an asset catalog and an `.icns` from an Icon Composer icon.
- **Your own frameworks,** from `frameworks`, beside Wren Runtime's in `Contents/Frameworks`.

### Signing

`wren bundle` signs everything from the inside out, as Chrome's own signing does: the libraries and helpers first, then the framework, then the app. Your own frameworks are signed the way Sparkle's documentation asks. Each bundle or executable nested in one is signed before what holds it, with the hardened runtime, its existing entitlements and its own bundle ID.

|  | Ad hoc, the default | Developer ID |
| --- | --- | --- |
| Hardened runtime | On | On |
| Library validation | Off, because there's no team to compare | On, except for the Renderer and GPU helpers |
| Renderer and GPU helpers | `allow-jit` | `allow-jit` |
| Added for development | `disable-library-validation`, and `get-task-allow` for debuggers | Nothing |
| Timestamp | None | From Apple's timestamp server, which needs a network |
| Runs on | The Mac that made it | Any Mac, once notarised |

Your app's entitlements are Chrome's unless you give your own: audio input, Bluetooth, camera, printing, USB, location and the photo library. They let Chromium ask macOS for those when a site asks the person, and grant nothing by themselves.

The manifest's `entitlements` replace Chrome's, on the app only. The helpers and your own frameworks keep theirs. Give a list of keys, each set to true, or an object of keys and their values:

```json
"entitlements": {
  "com.apple.security.device.camera": true,
  "com.apple.security.application-groups": ["group.com.example.mybrowser"]
}
```

An ad hoc signature changes with every build, so macOS asks again for anything it remembers by signature, the Keychain above all. Use the mock keychain while you develop, and the system keychain in a build signed with a Developer ID. See [Startup](startup.md#how-cookies-and-passwords-are-encrypted).

### Restricted entitlements and a provisioning profile

macOS launches an app with some entitlements only if a provisioning profile in it grants them. Passkeys in a browser, `com.apple.developer.web-browser.public-key-credential`, need one, and so do keychain access groups. `wren bundle` treats every entitlement outside `com.apple.security.` this way. Make the profile in your Apple Developer account, for your bundle ID and your certificate, and name it in the manifest:

```json
"provisioningProfile": "MyBrowser.provisionprofile"
```

- **Signed with an identity,** the bundle holds the profile at `Contents/embedded.provisionprofile`. The app also gets `com.apple.application-identifier` and `com.apple.developer.team-identifier` from the profile, as Xcode adds them. If the manifest gives either, it must match the profile.
- **Signed ad hoc,** the bundle leaves out the profile and every entitlement that needs one, and `wren bundle` prints what it left out. The app launches without them, so one manifest works for development and release.

Before it writes anything, `wren bundle` refuses a bundle macOS wouldn't launch, and says why. That covers restricted entitlements without a profile, and a profile that doesn't grant an entitlement with the manifest's value, is for another bundle ID or another certificate, has expired, or isn't for macOS. A profile value ending in `*` grants every value that starts with what comes before it, and a list is granted when each item is.

It finds your identity's certificate the way `codesign` does, among the identities `security find-identity -v -p codesigning` lists, by its SHA-1 or part of its name. If none or several match, it stops and says so, and a SHA-1 picks one.

An app with several release channels keeps a manifest for each, so each names its own profile.

## `wren notarize`

`wren notarize` sends an app or a zip signed with a Developer ID to Apple, and waits for the answer:

```sh
# Once, store your credentials in the keychain under a name of your choosing.
xcrun notarytool store-credentials my-notary

wren bundle --executable .build/release/MyBrowser --manifest wren-app.json \
  --output MyBrowser.app --identity "Developer ID Application: Example Ltd (TEAMID)"
wren notarize MyBrowser.app --keychain-profile my-notary
```

| Argument | What it is |
| --- | --- |
| `PATH` | An `.app` or a `.zip` |
| `--keychain-profile NAME` | The profile `notarytool store-credentials` saved. Your Apple ID and password stay in the keychain, and `wren` never sees them. |
| `--key FILE --key-id ID` | Instead of a profile, an App Store Connect API key, as its `.p8` file and key ID. `notarytool` reads the file, and `wren` only passes its path. |
| `--issuer UUID` | The key's issuer ID, for a team key. An individual key has none. |

An API key suits a build machine, which has no keychain profile. Make one in App Store Connect, under Users and Access, then Integrations:

```sh
wren notarize MyBrowser.app --key AuthKey_ABC123DEFG.p8 \
  --key-id ABC123DEFG --issuer 69a6de70-0000-0000-0000-5bf3fb4eb1fc
```

You can also store a key as a profile once, with `xcrun notarytool store-credentials my-notary --key … --key-id … --issuer …`, and use the profile's name after that.

For an app, `wren notarize` uploads a zipped copy. Once Apple accepts it, it staples the ticket to the app, checks the staple, and asks Gatekeeper with `spctl --assess`, then prints `Stapled; Gatekeeper accepts <path>`. A zip can't carry a ticket, so Gatekeeper asks Apple about its contents when they're first opened.

An app that isn't signed with a Developer ID is refused before anything is uploaded, because Apple would refuse every file in it. That includes an ad hoc signature, a development certificate, and no signature at all. A zip isn't checked.

If Apple refuses the upload, `wren notarize` prints Apple's log, which names each file and why, and exits 1. Notarising usually takes a few minutes.

## `wren pack-extension`

`wren pack-extension` packs an extension's folder as a `.crx` file, for your app to ship. See [Extensions your app ships](extensions.md).

```sh
# The first version, which makes the key.
wren pack-extension Blocker --key ~/keys/blocker.pem --new-key --output Blocker.crx
# Every later version, with the same key.
wren pack-extension Blocker --key ~/keys/blocker.pem --output Blocker.crx
```

It prints the extension's ID and version. The key is an RSA private key in a PEM file, and it decides the extension's ID. So `wren` makes a key only with `--new-key`, never replaces an existing file, and refuses a key inside the extension's folder, which would be packed with it. The key it makes is readable only by you. A key from Chrome's Pack Extension works too.

Hidden files, such as `.DS_Store`, are left out, as Chrome leaves them out. `wren` checks only that `manifest.json` has a `version`, and Chromium checks the rest when it installs the extension.

## `wren extension-id`

`wren extension-id` prints an extension's ID and nothing else, so a build can compare it with the ID your app expects:

```sh
test "$(wren extension-id Blocker.crx)" = fkgkibajhfbepljeaefdnfnegdcjomkh
```

For a `.crx`, it checks the file as Chromium does when it installs one. One of its keys must give the ID its header declares, and every signature must verify. So a file that passes is one Chromium will install under that ID. A file that isn't a CRX3 file, or whose keys or signatures don't match, exits 1.

For a folder, the ID comes from the `key` in its `manifest.json`, as for an [internal extension](extensions.md#internal-extensions). A folder without a readable `key` exits 1, because Chromium would give that extension an ID based on its path.

## Exit status

| Status | What it means |
| --- | --- |
| 0 | Done. |
| 1 | It couldn't be done, such as for a missing file or a bad manifest, or because `codesign` or Apple refused. The message says which. |
| 2 | The command line was wrong, and `wren` prints its usage. |

## Things to know

- **Each app carries its own framework,** about 350 MB in release 0.5.0, as Electron apps do. It runs only with the release your app was built against, because the framework accepts only its own API version.
- **Started from a terminal, prompts name the terminal.** macOS names the app responsible for a process in permission prompts and notifications. Run the executable from a shell, and they name your terminal. Start the app with `open` or from Finder, and they name your app.
- **An empty folder appears in Application Support.** A bundled app creates `~/Library/Application Support/<name>`, Chromium's default data folder, even when its data is elsewhere.
- **Ship Chromium's notices with your app.** Chromium's licence and its third-party licences ask that their notices go with every copy. A release has them in `LICENSES`: `Chromium.txt`, `third-party.txt`, and `credits.html`, which your app's `chrome://credits` shows. `Source.txt` says where to get the source of the LGPL and MPL files Wren Runtime changes, which people who get your app may ask for. Ship all four with your app, such as in its Resources folder, linked from its About window.

## Not available yet

- Disk images. `wren` makes the app, not a `.dmg`.
- Passkeys in the system keychain haven't been tried. `wren bundle` signs an app with the entitlement Apple grants browsers on request, and its profile, but nobody has tried passkeys in an app signed that way yet.
- Windows and Linux.
