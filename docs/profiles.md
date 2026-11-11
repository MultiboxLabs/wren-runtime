# Profiles and their data

As in Chrome, your app can keep several Profiles, such as one for work and one for home, each with its own history, cookies, saved passwords and extensions. This page shows how to open and make Profiles, change them the way Chrome's settings page does, clear their data, and bring someone's data over from the browser they used before.

The code here is in Swift. The [table at the end](#the-names-in-swift-rust-and-c) gives each name in Rust and in C.

## Opening and making Profiles

- **The one Chromium starts with.** `WrenApp.openProfile()` opens the Profile whose Browser was active last, as Chrome does. At the first launch, Chromium creates a new one, and `startupProfileName` in [Startup](startup.md#where-your-data-lives) can name it.
- **Any other.** `WrenApp.profileList` lists the persistent Profiles, with their IDs, names and avatars. `WrenApp.openProfile(id:)` opens one of them.
- **A new one.** `WrenApp.createProfile(displayName:)` makes a persistent Profile. Without a name, Chromium chooses one.
- **One for a single session.** `WrenApp.createEphemeralProfile` makes a Profile that nothing lists, which Chromium deletes once nothing uses it.
- **Incognito.** `profile.offTheRecord()` gives a persistent Profile's incognito side, which ends when its last Browser closes.

```swift
WrenApp.createProfile(displayName: "Work") { result in
    guard case .success(let work) = result else { return }
    work.createBrowser(url: nil) { result in
        if case .success(let browser) = result { show(browser) }
    }
}
```

Whenever the list changes, a `.profiles` change follows. That includes a Profile added, renamed, deleted, or given a new avatar or colour, whether your app did it or Chromium's own pages did.

## Changing a Profile

You can change a persistent Profile the same ways Chrome's settings page can.

- **Its name.** `profile.rename(_:)` renames it, without spaces at either end.
- **Its avatar.** `WrenApp.avatars` lists the avatars Chrome offers, and `profile.setAvatar(_:)` sets one. `WrenApp.avatarPNG` draws any of them, and `WrenApp.profileAvatarPNG(id:)` draws a listed Profile's avatar, even one that isn't loaded. The first avatar is a silhouette in the Profile's colours. Chromium downloads each avatar's large image from `www.gstatic.com` once, and draws a small built-in one until then. Chromium can't use an image of your own as an avatar, so keep any icons of your own by Profile ID.
- **Its colour.** `profile.setColor(_:)` sets the theme's colour, an opaque `0xFFRRGGBB`, and `profile.resetColor()` goes back to Chromium's default. `profile.color` reads it.

## Deleting a Profile

`profile.delete` closes the Profile's Browsers, and those of its incognito side, then deletes it. Each page gets its `beforeunload` first, as when quitting, and if any page cancels, nothing is deleted and the completion gets `.cancelled`.

Deleting works as it does in Chrome:

- Downloads in progress are cancelled without asking.
- The Profile's data goes at once, and its folder when the app quits.
- Its web views end, and every handle to it or its incognito side closes.
- The last persistent Profile can't be deleted.

The same happens when someone deletes a Profile from `chrome://profile-picker`. If the Profile Chromium started with is deleted, `openProfile()` opens the Profile Chromium moves to, as Chrome does. That's the one whose Browser was active last, or another Profile in the list. The next launch starts on it too, unless the Browser active last was an ephemeral Profile's.

## Clearing browsing data

`profile.clearBrowsingData` clears what Chrome's Clear browsing data dialog clears, for one of its time periods, in any kind of Profile. Its completion gets the kinds of data Chromium couldn't clear.

```swift
profile.clearBrowsingData([.history, .siteData, .cache], period: .lastHour) { result in
    if case .success(let failed) = result, !failed.isEmpty { showFailure(failed) }
}
```

Where policy forbids deleting history, a request that includes history or downloads fails with `.notAllowed`, and nothing is cleared. Pages of your own URL schemes keep their storage, as `chrome://` pages do.

To clear the list of downloads as chrome://downloads's Clear All does, use `profile.clearDownloadHistory()`. It clears every period and keeps downloads in progress. For an incognito Profile, it clears its original Profile's list too, as the page does.

## Turning off Chromium's password manager

Chromium's password manager is on in every Profile, as in Chrome. It offers to save passwords, fills them in, and shows its key icon and prompts. If your app has a password manager of its own, or relies on an extension's, `profile.setPasswordManager(false)` turns Chromium's off.

While it's off, Chromium saves, fills and suggests nothing, and shows no prompts. The Profile remembers the choice at later launches, and its incognito side follows it. Passwords already saved stay, and chrome://password-manager still lists them.

Where policy decides which sites the password manager is off for, `passwordManagerState().managed` is true, and `setPasswordManager` fails with `.notAllowed`.

## Bringing data over from another browser

When someone switches to your browser from Chrome, Arc or Safari, you can bring their history, cookies, extensions and bookmarks with them. Your app reads the other browser's files, and Wren Runtime writes the data into a Profile the way Chrome's own importer does. Chromium shows no UI for this, so ask first, and say what you'll bring.

```swift
let entries = [
    HistoryEntry(
        url: "https://example.com/", title: "Example Domain",
        lastVisitMs: 1_762_000_000_000, visitCount: 6, typedCount: 1)
]
profile.addHistory(entries) { result in
    if case .success(let added) = result { print("Added \(added) pages") }
}
```

- **History.** `profile.addHistory` adds pages with their titles, last visits, and how often each was visited and typed. Give the counts, because the address field suggests a page typed once, visited 4 times, or visited in the past 3 days. A page already in history gains a visit and keeps its counts. Pages history doesn't keep, such as `chrome://` pages, and visits older than 90 days are left out. The completion gets how many pages were added.
- **Cookies.** `profile.addCookies` writes cookies as a site setting them would, including HttpOnly and SameSite, and saves them to disk before the completion runs. The completion gets a result for each cookie, such as written, invalid, or expired. An incognito Profile's cookies are its own.
- **Extensions.** `profile.installFromWebStore` installs Chrome Web Store extensions by ID, as Chrome installs someone's synced extensions on a new Mac. Each one comes in turned on, with the permissions it asks for. See [Extensions your app ships](extensions.md#installing-someones-extensions-from-the-chrome-web-store).
- **Bookmarks.** `profile.addBookmark` adds each bookmark and folder. [`WrenBookmarks.h`](../include/WrenBookmarks.h) has the details.

Wren Runtime refuses the calls for history, cookies and extensions with `.notAllowed` inside `popover` and the other callbacks that run inside Chromium's call.

### Where Chromium browsers keep the data

Browsers built on Chromium, such as Chrome and Arc, keep each Profile's data in files in its folder. macOS asks the person before your app reads another app's Keychain item, which you need for the cookies.

| Data | Where it is |
| --- | --- |
| History | `History`, a SQLite database. Copy it before reading it. Pages are in the `urls` table, and `last_visit_time` is in microseconds since 1601. |
| Cookies | `Cookies`, a SQLite database. Copy it before reading it. Values starting `v10` are AES-128-CBC, with a key made by PBKDF2-SHA1 from the "`<Browser>` Safe Storage" Keychain password, the salt "saltysalt" and 1003 rounds. From the database's meta version 24, each value starts with the SHA-256 of its domain. `host_key` is the cookie's domain as `WrenCookie` takes it, with a leading dot for a domain cookie and none for a host-only one. `samesite` counts from -1, so add 1 to get `WrenCookieSameSite`: -1 is unspecified, 0 none, 1 lax and 2 strict. |
| Extensions | `extensions.settings` in `Secure Preferences` and `Preferences`. Bring over those from the store, and leave out those installed by default and the browser's own. |

## The names in Swift, Rust and C

| Swift | Rust | C |
| --- | --- | --- |
| `WrenApp.openProfile` | `MainThread::open_profile` | `Wren_OpenProfile` |
| `WrenApp.createProfile` | `MainThread::create_profile` | `Wren_CreateProfile` |
| `WrenApp.createEphemeralProfile` | `MainThread::create_ephemeral_profile` | `Wren_CreateEphemeralProfile` |
| `WrenApp.profileList` | `MainThread::profile_list` | `Wren_GetProfileListCount` |
| `profile.offTheRecord()` | `Profile::off_the_record` | `Wren_GetOffTheRecordProfile` |
| `profile.rename` | `Profile::rename` | `Wren_RenameProfile` |
| `WrenApp.avatars` | `MainThread::avatars` | `Wren_GetAvatarCount` |
| `profile.setAvatar` | `Profile::set_avatar` | `Wren_SetProfileAvatar` |
| `profile.setColor` | `Profile::set_color` | `Wren_SetProfileColor` |
| `profile.delete` | `Profile::delete` | `Wren_DeleteProfile` |
| `profile.clearBrowsingData` | `Profile::clear_browsing_data` | `Wren_ClearBrowsingData` |
| `profile.clearDownloadHistory()` | `Profile::clear_download_history` | `Wren_ClearDownloadHistory` |
| `profile.setPasswordManager` | `Profile::set_password_manager` | `Wren_SetPasswordManager` |
| `profile.addHistory`, `HistoryEntry` | `Profile::add_history`, `HistoryEntry` | `Wren_AddHistory`, `WrenHistoryEntry` |
| `profile.addCookies`, `Cookie`, `CookieResult` | `Profile::add_cookies`, `Cookie`, `CookieResult` | `Wren_AddCookies`, `WrenCookie`, `WrenCookieResult` |
| `profile.installFromWebStore` | `Profile::install_from_web_store` | `Wren_InstallFromWebStore` |
| `profile.addBookmark` | `Profile::add_bookmark` | `Wren_AddBookmarkItem` |
