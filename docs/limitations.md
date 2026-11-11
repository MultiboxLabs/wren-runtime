# What Wren Runtime can't do yet

Wren Runtime gives your app everything Chrome has behind its UI, but a few parts of Chrome aren't available yet. This page lists them, so you know before you plan a feature around one. Each release's notes say when one becomes available.

## Platforms

Wren Runtime runs only on macOS 13 or later, on Apple silicon. Windows and Linux aren't supported yet.

## Google's services

A release has no Google API keys, so Chrome's features that need them don't work, such as Translate and Safe Browsing's checks of downloads.

## Passkeys

Wren Runtime doesn't save passkeys in the system keychain.

## What Chrome draws in its own window

Most of what Chrome draws around a page comes to your app as state, so your UI can show it. These parts of Chrome's own window don't, and aren't available:

- infobars, including page info's offer to reload. A site's state has `reload_needed`, so you can show your own.
- the cookie controls, file system access and memory saver icons in the address field
- Lens results, About this site and Merchant trust, which Chrome draws as panels of its own
- reading mode's overlay. Its side panel works.
- Lens's overlay. Searching with an image opens Lens in a tab, and Wren Runtime doesn't offer "Search This Tab".
- in-product help
- moving the keyboard between a window's parts with F6
- the Touch Bar's items for a card field

## Installed web apps

Wren Runtime can't install or open web apps yet. Installing is off in every Profile, as it is in Chrome when an administrator turns off its `WebAppInstallByUserEnabled` policy. A site's own Install button gets no install prompt, so most sites hide it, and Chromium leaves out its own install icon and menu item. A web app installed with an earlier version of Wren Runtime stays installed, but its app in `~/Applications` opens no window.

## Chrome's menu

Name Window is turned off. The People menu and File's Share are left out.

## Your own URL schemes

A scheme of your own can't take a request body, stream a response, redirect, or use CORS yet.

## Web views

A web view can't tell you its page's address or title yet.

## The `wren` command

`wren` makes an app, not a disk image.
