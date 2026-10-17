# Wren runtime

Wren puts Chromium inside a native Mac app. Your app keeps its own windows and
draws its own UI; Chromium supplies real tabs and everything a browser keeps
behind them: Profiles, history, sessions, extensions, saved passwords, DevTools,
split tabs and side panels. Use it to build a browser, or to add web pages to an
app you already have, in Swift, Rust or C.

This repository holds Wren's releases, for app developers. Wren's source is not
here.

## Get Wren

Install the `wren` command once, with the GitHub CLI signed in to an account
with access to this repository, into a folder on your `PATH`:

```sh
gh release download --repo MultiboxLabs/wren-runtime --pattern wren \
  --output ~/.local/bin/wren
chmod +x ~/.local/bin/wren
```

Then, in a project whose `wren-app.json` names a release (`"wren": "0.1.0"`),
`wren install` fetches that release, checks it against its `SHA256SUMS`, keeps
it on the Mac, and links it into the project as `.wren/runtime`, which the
project's build files point at.

Each release on this repository's Releases page holds
`wren-runtime-<version>-mac-arm64.zip`, the release; a zip of its debug symbols,
for crash reports; `SHA256SUMS`; and `wren`. The release is a folder holding
everything an app needs: the framework, the C headers (`include`), a Swift
package (`wren-swift`), a Rust crate (`wren-rust`), the types for an app's pages
(`page`), the `wren` command (`bin/wren`), and these guides (`docs`). A zip
downloaded in a browser is quarantined; Getting started says how to use one.

## Start here

- [Getting started](docs/getting-started.md): a first app, from an empty folder
  to a window showing a page.
- [The guides](docs/README.md), by what you are building.
- [This release](docs/this-release.md): its versions, its Chromium, what it does
  not do yet, and its known issues.
- The headers are the reference for every function: [`Wren.h`](include/Wren.h)
  and the headers it includes.

## Licence

Wren may be used only with the written permission of Multibox Labs; see
`LICENSE` here, which a release carries as `LICENSES/Wren.txt`. A release's
`LICENSES` folder also holds Chromium's notices, which go with every app that
ships the framework.
