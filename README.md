# Wren runtime

Wren puts Chromium's browser layer inside a native Mac app. Your app keeps its own windows and draws its own UI. Chromium supplies real tabs, and everything a browser keeps behind them: Profiles, history, sessions, extensions, saved passwords, DevTools, split tabs and side panels. Use it to build a browser, or to add web pages to an app you already have, in Swift, Rust or C.

This repository holds Wren's releases, for app developers. Wren's source is not here.

## Get Wren

Install the `wren` command once:

```sh
curl -fsSL https://wrenbrowser.com/runtime/cli/install.sh | sh
```

`install.sh`, here, puts the newest release's `wren` in `~/.local/bin`, checked against the release's `SHA256SUMS`, and says if that folder is not on your `PATH`. `WREN_VERSION` and `WREN_INSTALL_DIR` pick another release or folder.

Then, in a project whose `wren-app.json` names a release, such as `"wren": "0.4.0"`, run `wren install`. It fetches that release, checks it against its `SHA256SUMS`, and keeps it on the Mac. It links the release into the project as `.wren/runtime`, which the project's build files point at.

Each release on this repository's Releases page holds four files:

- `wren-runtime-<version>-mac-arm64.zip`, the release
- `wren-runtime-<version>-mac-arm64-source.zip`, the source of the Chromium files Wren changes that are under the LGPL or the MPL
- `SHA256SUMS`, of the zips and `wren`
- `wren`, the command

The release is a folder that holds everything an app needs:

| Folder                     | What it is                                |
| -------------------------- | ----------------------------------------- |
| `Wren Framework.framework` | Chromium with Wren in it                  |
| `include`                  | The C headers                             |
| `wren-swift`               | The Swift package                         |
| `wren-rust`                | The Rust crate                            |
| `page`                     | The types for an app's pages, `wren.d.ts` |
| `bin/wren`                 | The `wren` command                        |
| `docs`                     | These guides                              |

Fetch the files with `install.sh`, `wren install`, `curl` or `gh`, as Getting started says, not in a web browser. macOS quarantines what a web browser downloads, and then refuses to run `bin/wren`.

## Start here

- [Getting started](docs/getting-started.md) makes a first app, from an empty folder to a window showing a page.
- [The guides](docs/README.md) are sorted by what you are building.
- [This release](docs/this-release.md) gives its versions and its Chromium, and says what it does not do yet and its known issues.
- The headers are the reference for every function: [`Wren.h`](include/Wren.h) and the headers it includes.

## Licence

Wren is proprietary software of Avocado and Bear Limited, which trades as Multibox Labs. `LICENSE` here is its licence, and a release carries it as `LICENSES/Wren.txt`. In short:

- You may download Wren and use it to develop and test your own apps, inside your organisation.
- You may use it to build, run, change and contribute to [Wren Browser](https://github.com/MultiboxLabs/wren-browser), but not distribute builds of it.
- Shipping an app with Wren to anyone outside your organisation needs a written licence from Multibox Labs for that app. Ask for one at licensing@wrenbrowser.com.

A release's `LICENSES` folder also holds Chromium's notices, and `Source.txt`, which says where the source of the files Wren changes is. Both go with every app that ships the framework.

Report bugs and ask for features in this repository's issues. Report a security vulnerability privately, as `SECURITY.md` says.
