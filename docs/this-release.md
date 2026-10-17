# Wren 0.1.0

The first release of Wren, for apps on macOS 13 or later on Apple silicon. Wren
is pre-1.0: an app is built and bundled with one release, and moves to the next
by building again (the Compatibility guide says what may change).

| Part                       | Version                                          |
| -------------------------- | ------------------------------------------------ |
| Wren                       | `0.1.0`                                          |
| Chromium                   | `155.0.8059.12`, with Wren's patch revision `55` |
| `WREN_API_VERSION`         | `36`                                             |
| `WREN_UNSTABLE_UI_VERSION` | `4`                                              |
| Rust, for the `wren` crate | `1.85` or later                                  |

`release.json` in the release names the build exactly; `Wren_GetVersionInfo`
reports it at runtime.

## Chromium, and security

`155.0.8059.12` is Chrome's early-stable release of 2026-09-23. Later Chrome
releases, the rest of 155 and 156 (stable from 2026-10-20), carry security fixes
it lacks. An app that shows pages from the open web should move to the next Wren
release when there is one.

The build has no Google API keys, so what needs them does not work: Translate,
and Safe Browsing's checks of downloads among them.

## Getting it

Install the `wren` command (published with each release), name `0.1.0` in an
app's `wren-app.json` (`"wren": "0.1.0"`), and run `wren install`: it fetches
the release, checks it against `SHA256SUMS`, and links it into the project. A
zip downloaded in a browser instead is quarantined until the mark is cleared
(`xattr -dr com.apple.quarantine <folder>`). The release is not signed: an app
is signed when `wren bundle` makes it.

## Licence

Wren may be used only with the written permission of Multibox Labs
(`LICENSES/Wren.txt`). The framework is Chromium, whose notices
(`LICENSES/Chromium.txt`, `third-party.txt`, `credits.html`) go with every app
that ships it. It decodes H.264, AAC and HEVC; any patent licensing an app that
ships them needs is the app's.

## What it does not do yet

- **Platforms:** macOS on Apple silicon only.
- **Profiles** cannot be deleted or renamed. Passkeys are not saved in the
  system keychain.
- **What Chrome draws in its own window:** infobars (page info's offer to reload
  among them); the cookie controls, file-system access and memory saver icons in
  the address field; the panels drawn in Views (Lens results, About this site,
  Merchant trust); reading mode's overlay (its panel works); Lens's overlay
  (image search opens Lens in a tab, and "Search This Tab" is not offered);
  in-product help; moving the keyboard between a window's panes (F6); the Touch
  Bar's items for a card field. A page's `EyeDropper` is refused.
- **Chrome's menu:** Print, Save Page, Open File and Name Window are disabled;
  the People menu and File's Share are left out.
- **App schemes:** no request bodies, streaming, redirects or CORS.
- **Bundling:** `wren` makes an app, not a disk image.

## Known issues

- A second launch in the instant before the first has started can take its
  place.
- A few places keep Chromium's name or artwork: three error-page strings, icons
  compiled into Chromium (the WebAuthn dialog, QR codes, the New Tab search
  box), and languages that inflect or transliterate the name.
- The crash handler outlives the app for over a minute.
- A bundled app makes an empty `~/Library/Application Support/<name>`,
  Chromium's default data folder, even when its data is elsewhere.
