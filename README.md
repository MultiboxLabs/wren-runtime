# Wren runtime

Wren puts Chromium's browser layer inside a native Mac app. Your app keeps its own windows and draws its own UI. Chromium supplies real tabs, and everything a browser keeps behind them: Profiles, history, sessions, extensions, saved passwords, DevTools, split tabs and side panels. Use it to build a browser, or to add web pages to an app you already have, in Swift, Rust or C.

This repository holds Wren's releases, for app developers. Wren's source is not here.

## Get Wren

Install the `wren` command once, into a folder on your `PATH`. The GitHub CLI must be signed in to an account with access to this repository.

```sh
gh release download --repo MultiboxLabs/wren-runtime --pattern wren \
  --output ~/.local/bin/wren
chmod +x ~/.local/bin/wren
```

Then, in a project whose `wren-app.json` names a release, such as `"wren": "0.3.0"`, run `wren install`. It fetches that release, checks it against its `SHA256SUMS`, and keeps it on the Mac. It links the release into the project as `.wren/runtime`, which the project's build files point at.

Each release on this repository's Releases page holds four files:

- `wren-runtime-<version>-mac-arm64.zip`, the release
- a zip of its debug symbols, for crash reports
- `SHA256SUMS`
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

Fetch the files with `wren install` or `gh`, as Getting started says, not in a web browser. macOS quarantines what a web browser downloads, and then refuses to run `bin/wren`.

## Start here

- [Getting started](docs/getting-started.md) makes a first app, from an empty folder to a window showing a page.
- [The guides](docs/README.md) are sorted by what you are building.
- [This release](docs/this-release.md) gives its versions and its Chromium, and says what it does not do yet and its known issues.
- The headers are the reference for every function: [`Wren.h`](include/Wren.h) and the headers it includes.

## Licence

Wren may be used only with the written permission of Multibox Labs. See `LICENSE` here, which a release carries as `LICENSES/Wren.txt`. A release's `LICENSES` folder also holds Chromium's notices, which go with every app that ships the framework.
