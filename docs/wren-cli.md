# The `wren` command

`wren` is the tool an app developer runs to get the Wren release an app is built
with, and to turn the compiled executable into an app that carries Wren. It is
one executable that needs nothing installed: each release ships it as
`bin/wren`, and publishes it on its own as `wren`.

```text
wren install [--manifest PATH]
wren bundle --executable PATH --manifest PATH --output PATH [--framework PATH] [--identity NAME] [--template PATH]
wren notarize PATH --keychain-profile NAME
wren notarize PATH --key FILE --key-id ID [--issuer UUID]
wren help
```

It is the only command a release ships; Wren's own build tools are not part of
it.

## Installing `wren`

Releases are published in the private repository `MultiboxLabs/wren-runtime`.
With access to it, and the GitHub CLI signed in, put `wren` in a folder on your
`PATH`:

```sh
gh release download --repo MultiboxLabs/wren-runtime --pattern wren \
  --output ~/.local/bin/wren
chmod +x ~/.local/bin/wren
```

That is the newest release's; any release's `wren` works with the others.

## `wren install`

Run in a project's folder, beside its `wren-app.json`:

1. It reads the release the manifest names (`"wren": "0.1.0"`).
2. When that release is not on this Mac, it fetches its zip and `SHA256SUMS`
   from `MultiboxLabs/wren-runtime`, signed in with `GH_TOKEN` or
   `GITHUB_TOKEN`, else the GitHub CLI's sign-in; refuses a zip that does not
   match `SHA256SUMS`; and unpacks it into
   `~/Library/Application Support/Wren/runtimes/<version>`, or
   `$WREN_RUNTIMES/<version>` when that is set (a CI cache, say). Nothing it
   fetches is quarantined, so macOS runs it as it is.
3. It links `.wren/runtime` in the project to that release, and keeps `.wren`
   out of git.

The project's build files name `.wren/runtime`: `Package.swift` depends on
`.wren/runtime/wren-swift`, `Cargo.toml` on `.wren/runtime/wren-rust`, and a C
build compiles with `-I .wren/runtime/include -F .wren/runtime`. To move to
another release, change `"wren"` and run `wren install` again.

## From source to app

`swift build` makes an executable, not an app. A Wren app also needs the
framework inside its bundle, helper apps for Chromium's child processes, and a
signature macOS accepts for all of it. `wren bundle` does that.

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

The app's executable must be at `MyBrowser.app/Contents/MacOS`: it loads the
framework from `../Frameworks` by a fixed path, so a bare executable outside a
bundle does not start.

## `wren bundle`

| Option              | Meaning                                                                                                                                                                       |
| ------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `--executable PATH` | The app's compiled executable. Required.                                                                                                                                      |
| `--manifest PATH`   | The app's manifest, `wren-app.json` (below). Required.                                                                                                                        |
| `--output PATH`     | The `.app` to make. Replaced if it exists. Required.                                                                                                                          |
| `--framework PATH`  | `Wren Framework.framework`. Defaults to `.wren/runtime`'s when the manifest names a release (which must be the one linked), else to the one in the release the command is in. |
| `--identity NAME`   | A `codesign` identity. Defaults to `-`, an ad hoc signature.                                                                                                                  |
| `--template PATH`   | The folder with Chromium's app `Info.plist` and localised strings. Found beside the framework (`app-template` in a release) unless given.                                     |

It prints `Bundled <path>` and exits 0. The bundle it leaves has already passed
`codesign --verify --deep --strict`.

### The manifest

`wren-app.json` holds what belongs to the app. Relative paths start at the
manifest's folder.

```json
{
  "name": "MyBrowser",
  "displayName": "My Browser",
  "bundleId": "com.example.mybrowser",
  "version": "1.2.0",
  "build": "42",
  "icon": "AppIcon.png",
  "copyright": "Copyright 2026 Example Ltd.",
  "urlSchemes": ["http", "https"],
  "documentTypes": ["public.html"],
  "usageDescriptions": {
    "NSCameraUsageDescription": "My Browser lets sites you allow use the camera."
  },
  "resources": ["Extras"]
}
```

| Field               | Required | Meaning                                                                                                                                                                   |
| ------------------- | -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `name`              | yes      | The executable's name in the bundle, `CFBundleName`, and the base of the helper apps' names. A file name; it may not end in " Helper" or ")".                             |
| `bundleId`          | yes      | The app's bundle ID, reverse-DNS. The helpers' IDs are derived from it.                                                                                                   |
| `displayName`       | no       | The name macOS shows. Defaults to `name`.                                                                                                                                 |
| `version`           | no       | `CFBundleShortVersionString`. Defaults to `1.0`.                                                                                                                          |
| `build`             | no       | `CFBundleVersion`. Defaults to `version`.                                                                                                                                 |
| `icon`              | no       | The app's icon, for the Dock, Finder and notifications: an Icon Composer `.icon`, or a square PNG of 1024 pixels. Without it the app has macOS's generic icon. See below. |
| `copyright`         | no       | `NSHumanReadableCopyright`.                                                                                                                                               |
| `urlSchemes`        | no       | URL schemes macOS offers the app for. None by default, so the app is not offered for web links.                                                                           |
| `documentTypes`     | no       | Uniform type identifiers the app opens, as a viewer. None by default.                                                                                                     |
| `usageDescriptions` | no       | Text for macOS's permission prompts, by `Info.plist` key, in place of Chromium's.                                                                                         |
| `entitlements`      | no       | The app's entitlements, as a list of keys, in place of Chrome's (below).                                                                                                  |
| `resources`         | no       | Files and folders copied into `Contents/Resources`.                                                                                                                       |
| `wren`              | no       | The Wren release the app is built with, such as `"0.1.0"`: what `wren install` links and `wren bundle` takes the framework from.                                          |

A field of the wrong type, or a `name` or `bundleId` that cannot be used, stops
the command with a message that names it.

### The icon

- **An Icon Composer `.icon`** is what macOS 26 and later draw in their own
  style: layered, with the light, dark, clear and tinted looks Icon Composer
  shows. `wren bundle` compiles it with Xcode's `actool` into an asset catalog
  (`Assets.car`) and an `.icns` that earlier releases of macOS use. The icon
  takes the file's name: `MyBrowser.icon` becomes `MyBrowser.icns`, named by
  `CFBundleIconName` and `CFBundleIconFile`. It needs Xcode 26 or later on the
  Mac that bundles, not only the Command Line Tools.
- **A PNG** is scaled to every size of an `.icns` with tools every Mac has.
  macOS 26 shows it as it is, on its standard backing shape, without the layered
  looks.

Either way the icon is also given to the Alerts helper, which is where macOS
takes the icon of an alert-style notification from.

### What the bundle contains

- **The framework**, copied to `Contents/Frameworks/Wren Framework.framework`.
- **Helper apps named after the app**, inside the framework: `<name> Helper`,
  `<name> Helper (Renderer)`, `<name> Helper (GPU)` and
  `<name> Helper (Alerts)`. Their bundle IDs are `<bundleId>.helper`,
  `<bundleId>.helper.renderer` and
  `<bundleId>.framework.AlertNotificationService`. The Alerts helper takes the
  app's display name and icon, which macOS shows on alert-style notifications.
  The framework's Aperitif helpers are left out; Chromium 155 does not use them.
- **`Info.plist`**, from Chromium's with the app's identity. Chromium's file and
  URL types are removed, so the app claims only what the manifest lists. The key
  `WrenHelperName` tells the framework where the helpers are; do not remove it.
- **Localised usage descriptions.** For every language Chromium has, its
  permission-prompt texts with the app's display name in place of "Chromium".
  Chromium picks its UI language from these folders, so they are all kept. A
  description given in the manifest replaces the localised one for that key, in
  every language.
- **The icon**, from the manifest's: `app.icns` from a PNG, or `Assets.car` and
  an `.icns` from an Icon Composer icon.

### Signing

Everything is signed inside out, as Chrome's own signer does it: the libraries
and helpers first, then the framework, then the app.

|                          | Ad hoc (`--identity -`, the default)                           | Developer ID                               |
| ------------------------ | -------------------------------------------------------------- | ------------------------------------------ |
| Hardened runtime         | on                                                             | on                                         |
| Library validation       | off (no team to compare)                                       | on, except the Renderer and GPU helpers    |
| Renderer and GPU helpers | `allow-jit`                                                    | `allow-jit`                                |
| Added for development    | `disable-library-validation`, `get-task-allow` (for debuggers) | nothing                                    |
| Timestamp                | none                                                           | Apple's timestamp server (needs a network) |
| Runs on                  | the Mac that made it                                           | any Mac, once notarised                    |

The app's entitlements default to Chrome's: audio input, Bluetooth, camera,
printing, USB, location and the photo library. They let Chromium ask macOS for
those when a site asks the user; they grant nothing by themselves.

An ad hoc signature changes with every build, so macOS asks again for anything
it remembers per signature, the Keychain above all. Use `WREN_KEYCHAIN_MOCK`
(Swift: `keychain: .mock`) in development, and the system keychain in a build
signed with a Developer ID.

## `wren notarize`

Sends an app or a zip signed with a Developer ID to Apple, and waits for the
answer.

```sh
# Once: store credentials in the keychain under a name of your choosing.
xcrun notarytool store-credentials my-notary

wren bundle --executable … --manifest wren-app.json --output MyBrowser.app \
  --identity "Developer ID Application: Example Ltd (TEAMID)"
wren notarize MyBrowser.app --keychain-profile my-notary
```

| Argument                  | Meaning                                                                                                                                      |
| ------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------- |
| `PATH`                    | An `.app`, or a `.zip`.                                                                                                                      |
| `--keychain-profile NAME` | The profile `notarytool store-credentials` saved. The Apple ID and password stay in the keychain; `wren` never sees them.                    |
| `--key FILE --key-id ID`  | In place of a profile: an App Store Connect API key, as its `.p8` file and key ID. `notarytool` reads the file; `wren` only passes its path. |
| `--issuer UUID`           | The key's issuer ID. A team key has one; an individual key has none.                                                                         |

Sign in one way or the other. An API key suits a build machine, which has no
keychain profile; make one at App Store Connect, under Users and Access,
Integrations:

```sh
wren notarize MyBrowser.app --key AuthKey_ABC123DEFG.p8 \
  --key-id ABC123DEFG --issuer 69a6de70-0000-0000-0000-5bf3fb4eb1fc
```

A key can also be stored as a profile once, and used by name afterwards:
`xcrun notarytool store-credentials my-notary --key … --key-id … --issuer …`.

For an app it zips a copy for the upload, and once Apple accepts it, staples the
ticket to the app, validates the staple and asks Gatekeeper (`spctl --assess`),
ending with `Stapled; Gatekeeper accepts <path>`. A zip cannot carry a ticket:
Gatekeeper asks Apple about its contents when they are first opened.

An app that no Developer ID signed (ad hoc, a development certificate, or
unsigned) is refused at once, before anything is uploaded: Apple would refuse
every file in it. A zip is not checked.

If Apple refuses the upload, the command prints Apple's log, which names each
file and why, and exits 1. Submissions usually take a few minutes.

## Exit status

| Status | Meaning                                                                                                    |
| ------ | ---------------------------------------------------------------------------------------------------------- |
| 0      | Done.                                                                                                      |
| 1      | It could not be done: a missing file, a bad manifest, `codesign` or Apple refused. The message says which. |
| 2      | The command line was wrong; usage is printed.                                                              |

## Things to know

- **Started from a terminal, the terminal is named.** macOS attributes
  permission prompts and notifications to the app responsible for the process.
  Run the executable directly from a shell and prompts say the terminal's name;
  start the app with `open`, or from Finder, and they say the app's.
- **An empty folder in Application Support.** A bundled app creates
  `~/Library/Application Support/<name>`, Chromium's default data folder, even
  when its data is elsewhere. Crash reports go to `Crashpad` inside the app's
  user-data directory.
- **One framework per app.** Each app carries its own copy (342 MB in release
  0.1.0), as Electron apps do, and must be bundled with the release it was
  compiled against: the framework accepts only its own API version.
- **Chromium's notices go with the app.** The framework is Chromium, whose
  licence and third-party licences ask that their notices go with every copy. A
  release has them in `LICENSES` (`Chromium.txt`, `third-party.txt`, and
  `credits.html`, the page an app's `chrome://credits` shows); ship them with
  your app, for example in its About window or beside its own licence.

## Not done yet

- A disk image: `wren` makes the app, not a `.dmg`.
- Passkeys in the system keychain, which need a provisioning profile and an
  entitlement Apple grants to browsers on request.
- Windows and Linux.
