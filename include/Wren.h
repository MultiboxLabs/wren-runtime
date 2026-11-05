// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: Chromium's browser layer in your app's process (C API).
//
// Wren runs Chromium's browser layer, the part of Chrome under its windows,
// inside the app's own process. That layer is //chrome, not only //content. It
// has Chrome's Profiles, tabs, history, extensions, passwords, downloads,
// DevTools and permission prompts. The app owns its windows and draws its own
// browser UI. Wren draws no window of its own, and opens no Browser by itself.
//
// This header has startup, the changes Wren reports, the app's callbacks, and
// the types every other header uses. It includes the other headers, listed at
// its end.
//
// Threads and timing:
//
// - Call every function on the main thread, after on_ready. The exceptions
//   are Wren_Main, Wren_GetAPIVersion, Wren_GetVersionInfo, the ID
//   formatting functions below, and Wren_PostToMainThread, which hands the
//   main thread work from any thread.
// - Completion callbacks, such as those of Wren_OpenProfile and
//   Wren_CreateBrowser, always run later from the event loop. They never run
//   inside the call that started them.
// - on_changed, on_reopen and on_open_urls run from the event loop, never
//   inside a Chromium notification. So these callbacks may call any function.
// - on_popover, on_fullscreen, on_downloads_prompt, on_quit_prompt,
//   on_quit_committed and on_navigation are the exceptions. They run inside
//   Chromium's call. WrenUI.h says what they may do.
// - on_profile_extensions runs inside Chromium's call too, and may make no
//   Wren calls.
// - Closing, through Wren_CloseTab or Wren_CloseBrowsers, and Wren_Quit are
//   requests. They take effect after the caller returns, and a page may
//   cancel them.
// - Every other call takes effect before it returns. The matching on_changed
//   record follows later.
//
// Handles and identity:
//
// - WrenProfile and WrenBrowser handles are reference-counted. A callback that
//   creates or opens one gives the app one reference. Handles passed to other
//   callbacks, or returned by Get functions, are borrowed.
// - A handle stays valid until its last reference is released, even after
//   what it names has closed. Calls on it then return WREN_ERROR_CLOSED, or do
//   nothing.
// - A Chromium Profile or Browser always has the same handle.
// - A tab is a WrenTabID value. An ID is unique for the life of the process,
//   and never reused. It stays with the tab when the tab moves, including to
//   another Browser. Calls with the ID of a closed tab return
//   WREN_ERROR_TAB_UNAVAILABLE.

#ifndef WREN_PUBLIC_WREN_H_
#define WREN_PUBLIC_WREN_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WREN_API __attribute__((visibility("default")))

// The version of the API this header and the ones it includes describe.
// WrenUnstableUI.h has its own version, WREN_UNSTABLE_UI_VERSION. Set
// WrenStartup.api_version to this. Wren_Main refuses any other version. The
// Compatibility guide has the details.
#define WREN_API_VERSION 71

typedef struct WrenProfile WrenProfile;
typedef struct WrenBrowser WrenBrowser;

// A Profile's kind (Wren_GetProfileKind, WrenBrowser.h).
typedef enum WrenProfileKind {
  // Stored in user_data_dir, and listed.
  WREN_PROFILE_PERSISTENT = 0,
  // Chromium's incognito Profile of a persistent one. Its session ends when
  // Chromium closes its last Browser. A later request starts a new session.
  WREN_PROFILE_OFF_THE_RECORD = 1,
  // Stored in user_data_dir, but not listed. Chromium deletes it once its last
  // handle and its last Browser are gone, or at the next launch after a crash.
  WREN_PROFILE_EPHEMERAL = 2,
} WrenProfileKind;

// A page the app places that is not a tab. See WrenWebView.h.
typedef struct WrenWebView WrenWebView;

typedef int32_t WrenTabID;
#define WREN_NO_TAB 0
// A Browser's ID. A web view's page names a Browser by this ID, because a
// handle cannot reach a page (WrenWebView.h). Unique for the life of the
// process.
typedef int32_t WrenBrowserID;
#define WREN_NO_BROWSER 0

// A tab group's ID. It is unique, and stays the same while the group exists,
// including when the group moves. All zero means no group.
typedef struct WrenTabGroupID {
  uint64_t high;
  uint64_t low;
} WrenTabGroupID;

// A split's ID. A split is two tabs shown side by side as one. The ID is
// unique while the split exists. All zero means no split.
typedef struct WrenSplitID {
  uint64_t high;
  uint64_t low;
} WrenSplitID;

// A group's or a split's ID as text, for a page or a file. The text is
// Chromium's own form: 32 upper-case hexadecimal digits, `high` first. An ID of
// all zeros, meaning none, is 32 zeros. The size includes the NUL. These four
// functions may be called on any thread at any time.
#define WREN_ID_TEXT_SIZE 33
WREN_API void Wren_FormatTabGroupID(WrenTabGroupID id,
                                    char text[WREN_ID_TEXT_SIZE]);
// Reads what Wren_FormatTabGroupID writes. For any other text, it returns
// false and sets `id` to all zeros. Other text includes another length, lower
// case, and anything but hexadecimal digits.
WREN_API bool Wren_ParseTabGroupID(const char* text, WrenTabGroupID* id);
WREN_API void Wren_FormatSplitID(WrenSplitID id, char text[WREN_ID_TEXT_SIZE]);
WREN_API bool Wren_ParseSplitID(const char* text, WrenSplitID* id);

typedef enum WrenResult {
  WREN_OK = 0,
  // The Profile or Browser has closed.
  WREN_ERROR_CLOSED = 1,
  // No open tab has this ID.
  WREN_ERROR_TAB_UNAVAILABLE = 2,
  // No such Profile, or Chromium could not load it.
  WREN_ERROR_PROFILE_UNAVAILABLE = 3,
  // The call is not allowed here. Chromium's policy may forbid it, such as
  // off-the-record browsing, or the call is not allowed in this state.
  WREN_ERROR_NOT_ALLOWED = 4,
  // An argument is missing or malformed. Examples are an invalid URL, and
  // tabs from different Browsers in one call.
  WREN_ERROR_INVALID_ARGUMENT = 5,
  // No open tab group has this ID.
  WREN_ERROR_GROUP_UNAVAILABLE = 6,
  // The tab cannot go back or forward now, or has no page yet.
  WREN_ERROR_NAVIGATION_UNAVAILABLE = 7,
  // No open split has this ID.
  WREN_ERROR_SPLIT_UNAVAILABLE = 8,
  // No side panel with this ID is available in the Browser now.
  WREN_ERROR_PANEL_UNAVAILABLE = 9,
  // The popover request has closed. See WrenUI.h.
  WREN_ERROR_POPOVER_CLOSED = 10,
  // The page token is not that of the Browser's current page (WrenUI.h). The
  // tab navigated, or another tab became active, since it was read.
  WREN_ERROR_STALE_PAGE = 11,
  // The suggestions have changed since this WrenOmniboxResult was read. See
  // WrenOmnibox.h.
  WREN_ERROR_STALE_RESULT = 12,
  // Chromium does not have this download. See WrenDownloads.h.
  WREN_ERROR_DOWNLOAD_UNAVAILABLE = 13,
  // No bookmark or folder of the Profile has this ID. See WrenBookmarks.h.
  WREN_ERROR_BOOKMARK_UNAVAILABLE = 14,
  // A page cancelled the call with beforeunload, or the downloads warning
  // did, so nothing was done. See Wren_CloseBrowsers and Wren_DeleteProfile.
  WREN_ERROR_CANCELLED = 15,
  // The tab's page gave no picture: it is discarded, crashed or of no size,
  // or drew nothing in time. See Wren_CaptureTab.
  WREN_ERROR_NO_IMAGE = 16,
} WrenResult;

// Receives the result of a call that answers later, from the event loop.
typedef void (*WrenResultCallback)(void* context, WrenResult result);
// Receives a Browser a call made, from the event loop, as Wren_CreateBrowser
// does (WrenBrowser.h). On success it receives one reference to the Browser.
// On failure it receives NULL and the reason.
typedef void (*WrenBrowserCallback)(void* context,
                                    WrenBrowser* browser,
                                    WrenResult result);

// Why a page is gone, for a tab (WrenTabs.h) or a web view (WrenWebView.h).
typedef enum WrenTabCrash {
  WREN_TAB_CRASH_NONE = 0,
  // The page's renderer crashed.
  WREN_TAB_CRASH_CRASHED = 1,
  // The system or chrome://kill killed the page's renderer.
  WREN_TAB_CRASH_KILLED = 2,
  // The renderer ran out of memory.
  WREN_TAB_CRASH_OUT_OF_MEMORY = 3,
} WrenTabCrash;

// Where a page opens, for a suggestion (WrenOmnibox.h) or a bookmark
// (WrenBookmarks.h).
typedef enum WrenOpenDisposition {
  WREN_OPEN_CURRENT_TAB = 0,
  // A new tab, shown. In Chrome, Alt-Enter or a middle click.
  WREN_OPEN_NEW_TAB = 1,
  // A new tab, behind the current one.
  WREN_OPEN_BACKGROUND_TAB = 2,
  // A new window, as Shift-Enter opens in Chrome. That is a new normal
  // Browser, which on_browser_opened gives the app.
  WREN_OPEN_NEW_WINDOW = 3,
  // The tab that already has the suggestion's page (has_tab), in whichever
  // Browser it is.
  WREN_OPEN_SWITCH_TO_TAB = 4,
} WrenOpenDisposition;

// A content setting. Each has an icon in the address field (WrenToolbar.h),
// and a bubble whose content is in WrenUI.h.
typedef enum WrenContentSetting {
  WREN_CONTENT_SETTING_COOKIES = 0,
  WREN_CONTENT_SETTING_IMAGES = 1,
  WREN_CONTENT_SETTING_JAVASCRIPT = 2,
  WREN_CONTENT_SETTING_POPUPS = 3,
  WREN_CONTENT_SETTING_GEOLOCATION = 4,
  // Insecure content on a secure page.
  WREN_CONTENT_SETTING_MIXED_SCRIPT = 5,

  WREN_CONTENT_SETTING_PROTOCOL_HANDLERS = 6,
  // The camera or microphone.
  WREN_CONTENT_SETTING_MEDIA = 7,

  WREN_CONTENT_SETTING_ADS = 8,
  WREN_CONTENT_SETTING_AUTOMATIC_DOWNLOADS = 9,
  WREN_CONTENT_SETTING_MIDI = 10,
  WREN_CONTENT_SETTING_SOUND = 11,
  // A redirect by a frame was blocked.
  WREN_CONTENT_SETTING_REDIRECT = 12,

  WREN_CONTENT_SETTING_SENSORS = 13,
  WREN_CONTENT_SETTING_CLIPBOARD = 14,
  WREN_CONTENT_SETTING_STORAGE_ACCESS = 15,
  WREN_CONTENT_SETTING_NOTIFICATIONS = 16,
} WrenContentSetting;

// --- Startup ----------------------------------------------------------------

// Where Chromium keeps its encryption key for cookies and passwords.
typedef enum WrenKeychain {
  // The login Keychain, as the item "<keychain_name> Safe Storage". It needs
  // an app signed with a stable identity. An ad hoc build asks the user for
  // access after every rebuild.
  WREN_KEYCHAIN_SYSTEM = 1,
  // A fixed in-memory key. For development and tests only.
  WREN_KEYCHAIN_MOCK = 2,
} WrenKeychain;

// The main menu Wren installs before on_ready. After that, the app may set its
// own from data (Wren_SetMainMenu, WrenMenus.h), or edit or replace
// NSApp.mainMenu itself.
typedef enum WrenMenu {
  // App, File, Edit, View and Window menus, like Electron's default. The View
  // menu's Reload, Force Reload, Toggle Developer Tools and zoom items act on
  // the key window's Browser, the one attached to a view in it. Its Quit item
  // is WREN_MENU_ROLE_QUIT (WrenMenus.h).
  WREN_MENU_DEFAULT = 0,
  // Chrome's main menu, without the lists Chrome fills in as it runs. Those
  // are profiles, bookmarks, history, open tabs and saved groups.
  //
  // Its commands act on the key window's Browser. With none, they act as in
  // Chrome with no window open. Find opens Chromium's find bar. Commands whose
  // UI Wren lacks, such as Name Window, are disabled. Edit it like any NSMenu.
  // An item copied from it keeps its target.
  WREN_MENU_CHROMIUM = 1,
} WrenMenu;

// What one of the app's URL schemes may do (WrenScheme.privileges).
typedef enum WrenSchemePrivileges {
  WREN_SCHEME_PLAIN = 0,
  // URLs have a host and a path, as https URLs do, such as
  // "app://ui/index.html". So relative URLs resolve, each host is an origin of
  // its own, and pages have web storage. Without this, a URL is opaque, as a
  // data: URL is.
  WREN_SCHEME_STANDARD = 1u << 0,
  // Pages are secure contexts. Loading them from an https page is not mixed
  // content.
  WREN_SCHEME_SECURE = 1u << 1,
  // The scheme's own pages may fetch() its URLs. Pages of other origins
  // cannot read its responses.
  WREN_SCHEME_FETCH = 1u << 2,
  // Other pages may reach the scheme. A website, an extension or another of
  // the app's schemes may link to its pages, frame them, and load its scripts
  // and images, as they may from an https URL. They still cannot read its
  // responses.
  //
  // Without this, the scheme is closed. Its handler gets requests only for
  // what the app or the user opens, such as a tab's or a web view's URL or a
  // typed address, and for what the scheme's own pages ask for. Every other
  // request fails without reaching the handler. Leave it out for pages that
  // are the browser's own, such as its UI and its settings.
  WREN_SCHEME_WEB_ACCESSIBLE = 1u << 3,
} WrenSchemePrivileges;

// A URL scheme the app answers itself. See WrenSchemes.h.
typedef struct WrenScheme {
  // The scheme's name, such as "app". A lower-case letter, then lower-case
  // letters, digits, "+", "-" and ".", up to 64 characters. It may not be one
  // of Chromium's own, such as https, file, chrome, chrome-extension, about,
  // data or mailto.
  const char* name;
  // WrenSchemePrivileges, combined.
  uint32_t privileges;
} WrenScheme;

// An extension the app ships, which Wren installs in the Profiles the app
// chooses. "App extensions" in WrenExtensions.h says how.
typedef enum WrenAppExtensionKind {
  // An ordinary extension, as a .crx file. The user sees it on Chrome's
  // extensions page, and may turn it off.
  WREN_APP_EXTENSION_BUNDLED = 0,
  // A hidden component extension, as a folder whose manifest.json has a key.
  // The key gives its ID. Chrome's extensions page and Wren_GetExtensionCount
  // leave it out, and it is always on. WrenExtensions.h says what it may do.
  WREN_APP_EXTENSION_INTERNAL = 1,
} WrenAppExtensionKind;

typedef struct WrenAppExtension {
  WrenAppExtensionKind kind;
  // Required. An absolute path, in UTF-8: the .crx file, or the internal
  // extension's folder.
  const char* path;
  // For a bundled extension, whether it is on when Wren installs it in a
  // Profile. Off, it stays off until the user or the app turns it on
  // (Wren_SetExtensionEnabled). After the install, the Profile keeps whatever
  // the user or the app chose. An internal extension ignores it.
  // on_profile_extensions may choose otherwise for one Profile.
  bool enabled_on_install;
  // For a bundled extension, whether it is allowed in incognito when Wren
  // installs it in a Profile, as with Allow in Incognito on Chrome's
  // extensions page. False, as in Chrome, leaves that to the user or the app
  // (Wren_SetExtensionAllowedInIncognito). After the install, the Profile
  // keeps whatever they chose. An internal extension always runs in
  // incognito. on_profile_extensions may choose otherwise for one Profile.
  bool allowed_in_incognito_on_install;
} WrenAppExtension;

// A native messaging host the app answers in its own process. "Native
// messaging" in WrenExtensions.h says how.
typedef struct WrenNativeHost {
  // Required. The name extensions pass to chrome.runtime.connectNative and
  // sendNativeMessage, by Chromium's rule: lower-case letters, digits, "."
  // and "_", with no "." at either end or beside another.
  const char* name;
  // Required, at least one. The extensions that may reach it, by ID, each 32
  // letters from a to p.
  const char* const* extension_ids;
  size_t extension_id_count;
} WrenNativeHost;

// Which Manifest V2 extensions may run. Chromium refuses to install any other,
// and turns off one that is installed. The Chrome Web Store refuses them all,
// whatever this says. Component extensions are not affected.
typedef enum WrenManifestV2 {
  // None, as in Chrome.
  WREN_MANIFEST_V2_NONE = 0,
  // The extensions WrenStartup.manifest_v2_ids names, the app's or the
  // user's.
  WREN_MANIFEST_V2_LISTED = 1,
  // All of them, an unpacked one the user loads among them.
  WREN_MANIFEST_V2_ALL = 2,
} WrenManifestV2;

// How Chromium starts. Wren reads every field before Wren_Main hands control
// to Chromium. It copies the strings and the icons.
typedef struct WrenStartup {
  // Required: WREN_API_VERSION.
  uint32_t api_version;
  // Required: WREN_UNSTABLE_UI_VERSION (WrenUnstableUI.h).
  uint32_t unstable_ui_version;
  // Required. An absolute path. Only one process at a time can run with a
  // directory.
  const char* user_data_dir;
  // Optional. The startup Profile's name, in UTF-8, if Chromium creates the
  // Profile in this launch, as it does at the first launch in a new
  // user_data_dir. It never renames a Profile that exists. Wren trims
  // whitespace at either end. NULL lets Chromium choose the name, as
  // Wren_CreateProfile does without one. A name that is empty after trimming
  // ends Wren_Main with WREN_EXIT_INVALID_STARTUP.
  const char* startup_profile_name;
  // Required.
  WrenKeychain keychain;
  // Required for WREN_KEYCHAIN_SYSTEM, such as "Wren Sample". Keep it the same
  // across versions of the app. A new name means a new key, and Chromium can
  // no longer decrypt the cookies and passwords it already has.
  const char* keychain_name;
  // Optional. Extra Chromium switches for development, each starting with
  // "--". Wren rejects switches that would override the settings in this
  // struct.
  const char* const* development_switches;
  size_t development_switch_count;
  WrenMenu menu;
  // Optional. The app's name in Chromium's own UI, in place of "Chromium",
  // "Chrome" and "Google Chrome". Chromium's own UI is its WebUI pages, such as
  // Settings and the New Tab page, and its dialogs, prompts and toasts.
  // Without it, they say "Chromium", so a shipped app sets it.
  //
  // Google's products, such as the Chrome Web Store, keep their names, and so
  // do strings about the Chromium project, such as copyright and credits.
  // NULL keeps Chromium's name. 1 to 64 characters, with no whitespace at
  // either end, no control characters, and none of $ { } # < > & ".
  const char* product_name;
  // Optional. A square PNG, 256 to 4096 pixels wide, shown as Chromium's
  // product logo in Settings > About, chrome://version and dialogs. NULL keeps
  // Chromium's logo. On its own, it serves every variant below.
  const void* product_icon_png;
  size_t product_icon_png_size;
  // Optional variants, each shown where Chromium shows that variant of its
  // logo. They need product_icon_png. Where each one appears, and what a NULL
  // one falls back to:
  //
  // - product_icon_png: Settings > About, the WebUI toolbars of Settings,
  //   Extensions and History in light mode, chrome://version in light mode,
  //   and dialogs. Wren renders exact sizes from it, so they stay sharp.
  // - product_icon_dark_png: chrome://version in dark mode. Falls back to
  //   product_icon_png.
  // - product_icon_svg: the profile picker, the first-run and search engine
  //   choice pages, and the default-browser dialog. Falls back to
  //   product_icon_png.
  // - product_icon_dark_svg: WebUI toolbars in dark mode. Falls back to the
  //   dark PNG, then the SVG, then product_icon_png.
  //
  // Chromium's light toolbars always use a PNG, never an SVG. The dark PNG
  // follows product_icon_png's rules. An SVG is UTF-8 text that starts with
  // <svg or <?xml, up to 1 MiB.
  const void* product_icon_dark_png;
  size_t product_icon_dark_png_size;
  const void* product_icon_svg;
  size_t product_icon_svg_size;
  const void* product_icon_dark_svg;
  size_t product_icon_dark_svg_size;
  // Optional. Shown as given, in every language, in place of Chromium's
  // copyright line and company name, in Settings > About and chrome://version.
  // NULL keeps Chromium's, "The Chromium Authors". The same rules as
  // product_name apply, but the copyright line may have up to 128
  // characters.
  const char*
      copyright;  // e.g. "Copyright 2026 Wren Authors. All rights reserved."
  const char* company_name;  // e.g. "Wren Authors"
  // Optional. An https URL that "Get help" in Settings > About opens in a new
  // tab. NULL hides the row, because Chromium's own help is Google's.
  const char* help_url;
  // Optional. The app's version, such as "1.2.0", shown in Settings > About as
  // "Version 1.2.0 (Official Build, Chromium 156.0.8078.48)". NULL shows
  // Chromium's version alone. The same rules as product_name apply.
  const char* app_version;
  // Optional. The app's own URL schemes. Chromium fixes its list of schemes
  // as it starts, so the app cannot add one later. The handlers the app sets
  // after on_ready answer their requests (Wren_SetSchemeHandler).
  const WrenScheme* schemes;
  size_t scheme_count;
  // Optional. The extensions the app ships, for this launch only. Wren reads
  // each one's ID and version before Chromium starts. These end Wren_Main with
  // WREN_EXIT_INVALID_STARTUP:
  //
  // - a missing file or folder
  // - a bundled extension's file that is not a CRX3 file
  // - an internal extension's manifest.json without a key
  // - a manifest without a valid version
  // - two extensions with one ID
  //
  // Chromium checks a .crx file's signature later, when it installs the file.
  // "App extensions" in WrenExtensions.h says what Wren does with them.
  const WrenAppExtension* extensions;
  size_t extension_count;
  // Optional. Which Manifest V2 extensions may run. 0 is
  // WREN_MANIFEST_V2_NONE. manifest_v2_ids are extension IDs, each 32 letters
  // from a to p, and are only for WREN_MANIFEST_V2_LISTED.
  WrenManifestV2 manifest_v2;
  const char* const* manifest_v2_ids;
  size_t manifest_v2_id_count;
  // Optional. The native messaging hosts the app answers. A bad name, a bad
  // or missing ID, or two hosts with one name end Wren_Main with
  // WREN_EXIT_INVALID_STARTUP. The handlers the app sets after on_ready
  // answer their ports (Wren_SetNativeHostHandler).
  const WrenNativeHost* native_hosts;
  size_t native_host_count;
} WrenStartup;

// --- Changes ----------------------------------------------------------------

// What changed. Each kind names what to read again. Wren reports a kind that
// says "Reported once the app has read it" only after the app's first read of
// what it names. From then on, it reports every change.
typedef enum WrenChangeKind {
  // The list of persistent Profiles changed. See Wren_GetProfileListCount.
  WREN_CHANGE_PROFILES = 0,
  // `profile` closed.
  WREN_CHANGE_PROFILE_CLOSED = 1,
  // Tabs were added to `browser`, removed or moved, or its active tab
  // changed. Read its tabs again with Wren_GetTabCount and Wren_GetTabAt.
  WREN_CHANGE_TABS = 2,
  // The fields in `tab_fields` changed for `tab`, which is now in `browser`.
  WREN_CHANGE_TAB = 3,
  // `group`'s title, colour, collapsed state or tabs changed, or it closed.
  // `browser` is its Browser, or NULL once it has closed.
  WREN_CHANGE_GROUP = 4,
  // `split`'s tabs, order, layout or ratio changed, or it ended. `browser` is
  // its Browser, or NULL once it has ended.
  WREN_CHANGE_SPLIT = 5,
  // `browser`'s side panel opened, closed or changed to another panel, or the
  // open panel's width changed. See Wren_GetSidePanelState.
  WREN_CHANGE_SIDE_PANEL = 6,
  // A browser-modal dialog opened or closed over `browser`. See
  // Wren_HasModalDialog.
  WREN_CHANGE_MODAL_DIALOG = 7,
  // `browser`'s extension actions changed. An action was added, removed,
  // pinned or reordered, or its icon, name, tooltip, badge, enabled state,
  // popup or pop-out changed, including on a tab switch. Read them again.
  // WREN_CHANGE_EXTENSIONS reports the installed extensions.
  WREN_CHANGE_EXTENSION_ACTIONS = 8,
  // A page action of `browser`'s active tab changed, or the active tab did.
  // An action may have been shown or hidden, or its label, tooltip, icon or
  // bubble changed. Read them again.
  WREN_CHANGE_PAGE_ACTIONS = 9,
  // `browser`'s content setting icons changed, for its active tab. Read them
  // again.
  WREN_CHANGE_CONTENT_SETTINGS = 10,
  // A popover request of `browser`'s, `popover`, changed or closed. Only
  // requests the app answered with WREN_POPOVER_APP report this. See
  // WrenUI.h.
  WREN_CHANGE_POPOVER = 11,
  // What `browser`'s active tab's site shows changed: its security,
  // permissions, devices or cookies. Or the active tab, or its page, changed.
  // See Wren_GetSiteState. Reported once the app has read it.
  WREN_CHANGE_SITE = 12,
  // What `browser`'s extensions may do on its active tab's site changed: the
  // site setting, their access or their requests. Or the page changed. See
  // Wren_GetExtensionsSiteState. Reported once the app has read it.
  WREN_CHANGE_EXTENSIONS_SITE = 13,
  // The zoom of `browser`'s active tab changed, or the active tab did. See
  // Wren_GetZoomState. Reported once the app has read it.
  WREN_CHANGE_ZOOM = 14,
  // Whether the page of `browser`'s active tab is bookmarked changed, or its
  // bookmark did. Or the page, or the active tab, changed. See
  // Wren_GetBookmarkState. Reported once the app has read it.
  WREN_CHANGE_BOOKMARK = 15,
  // A page in `browser` entered or left full screen. See
  // Wren_GetFullscreenTab.
  WREN_CHANGE_FULLSCREEN = 16,
  // What `browser`'s address field shows changed: the active tab's address,
  // the active tab, or the default search engine. See Wren_GetLocationState
  // (WrenOmnibox.h). Reported once the app has read it.
  WREN_CHANGE_LOCATION = 17,
  // `browser`'s suggestions changed, or a suggestion's icon arrived. See
  // Wren_GetOmniboxState (WrenOmnibox.h).
  WREN_CHANGE_OMNIBOX = 18,
  // Whether `browser` has a closed tab to reopen changed. See
  // Wren_CanReopenClosedTab (WrenSession.h). Reported once the app has read
  // it.
  WREN_CHANGE_CLOSED_TABS = 19,
  // Chromium asked for `browser`, which is in the background, to come
  // forward. See Wren_HasForegroundRequest. By the time the app hears it,
  // the app may have forgotten it (Wren_ClearForegroundRequest): the getter
  // says what stands.
  WREN_CHANGE_FOREGROUND_REQUEST = 20,
  // `web_view`'s page began or stopped loading, failed to load, drew, or its
  // DevTools opened or closed. Or its process ended or started again. See
  // Wren_GetWebViewState (WrenWebView.h).
  WREN_CHANGE_WEB_VIEW = 21,
  // The state of one or more of `browser`'s anchors changed. An anchor was
  // named or placed, or one of Chromium's bubbles opened or closed at it. See
  // Wren_GetAnchorState (WrenToolbar.h).
  WREN_CHANGE_ANCHORS = 22,
  // `browser` became active or inactive. See Wren_IsBrowserActive
  // (WrenBrowser.h).
  WREN_CHANGE_ACTIVE = 23,
  // `browser`'s find changed. It opened or ended, its text or its matches
  // changed, or the active tab did. See Wren_GetFindState (WrenUI.h).
  WREN_CHANGE_FIND = 24,
  // `profile`'s lists of downloads changed: one was added or went, one
  // entered, left or moved in a list, or the history loaded. See
  // Wren_GetDownloadCount and Wren_GetDownloadHistory (WrenDownloads.h).
  // Reported once the app has read either. A change to one download that
  // leaves the lists as they were is WREN_CHANGE_DOWNLOAD.
  WREN_CHANGE_DOWNLOADS = 25,
  // `browser`'s download button changed. See Wren_GetDownloadButtonState
  // (WrenDownloads.h). Reported once the app has read it.
  WREN_CHANGE_DOWNLOAD_BUTTON = 26,
  // `profile`'s bookmarks changed. An item was added, removed, moved, renamed
  // or given another URL, an item's icon loaded, or the bookmarks loaded. See
  // Wren_GetBookmarksState (WrenBookmarks.h). Reported once the app has read
  // them.
  WREN_CHANGE_BOOKMARKS = 27,
  // Warn Before Quitting was turned on or off, by the app, Chrome's menu
  // item, chrome://settings or policy, or policy began or stopped setting it.
  // See Wren_GetQuitWarningState.
  WREN_CHANGE_QUIT_WARNING = 28,
  // `profile`'s installed extensions changed. One was installed, removed,
  // turned on or off, allowed in incognito or not, pinned, unpinned or
  // moved, or allowed to show its requests or not, or its icon loaded.
  // See Wren_GetExtensionCount (WrenExtensions.h). An off-the-record
  // Profile's extensions are its original's, and each of the two the app has
  // read hears the change. Reported once the app has read them.
  WREN_CHANGE_EXTENSIONS = 29,
  // `download`'s progress or state changed. See Wren_GetDownloadState
  // (WrenDownloads.h). `profile` is the download's own. Reported once the app
  // has read a list that has it, that of its Profile or of an off-the-record
  // Profile of its Profile's.
  WREN_CHANGE_DOWNLOAD = 30,
  // Whether the app is the default browser changed, as Wren_SetAsDefaultBrowser
  // or Wren_GetDefaultBrowserState found, or as Wren found when the app became
  // active. Reported once the app has asked.
  WREN_CHANGE_DEFAULT_BROWSER = 31,
  // Chromium loaded `profile` in this launch: persistent, ephemeral or off
  // the record, whoever asked, the ones Wren_OpenProfile and
  // Wren_CreateProfile open included. Each is reported once, and its
  // PROFILE_CLOSED follows when it closes. Profiles loaded before on_ready
  // come in the first on_changed. A Browser Chromium shows for a Profile it
  // has just loaded, as a restore may, reaches on_browser_opened first.
  WREN_CHANGE_PROFILE_OPENED = 32,
} WrenChangeKind;

// Bits of WrenChange.tab_fields.
typedef enum WrenTabField {
  WREN_TAB_FIELD_TITLE = 1u << 0,
  WREN_TAB_FIELD_URL = 1u << 1,
  // Loading started or stopped.
  WREN_TAB_FIELD_LOADING = 1u << 2,
  // The session history changed, and with it whether the tab can go back or
  // forward.
  WREN_TAB_FIELD_HISTORY = 1u << 3,

  WREN_TAB_FIELD_FAVICON = 1u << 4,
  WREN_TAB_FIELD_PINNED = 1u << 5,
  WREN_TAB_FIELD_GROUP = 1u << 6,
  // Chromium discarded the page to save memory, or loaded it again after.
  WREN_TAB_FIELD_DISCARDED = 1u << 7,
  // 1u << 8 was WREN_TAB_FIELD_VIEW. A tab's view now stays the same for the
  // whole life of the tab.
  //
  // The page's renderer crashed or was killed, or the tab loaded again after
  // it. See WrenTabState.crash.
  WREN_TAB_FIELD_CRASHED = 1u << 9,
  // DevTools for the tab opened, closed, docked to another side or undocked,
  // through Wren_OpenDevTools or DevTools' own dock menu. See
  // WrenTabState.devtools.
  WREN_TAB_FIELD_DEVTOOLS = 1u << 10,
  // The tab joined or left a split. See WrenTabState.split.
  WREN_TAB_FIELD_SPLIT = 1u << 11,
  // What Chrome's status bubble would show for the tab changed, such as the
  // link under the pointer or the loading line. See WrenTabState.status and
  // Wren_CopyTabStatusText.
  WREN_TAB_FIELD_STATUS = 1u << 12,
  // The tab's page went into a <wren-view> element of a web view's page, or
  // left it. See WrenTabState.in_element.
  WREN_TAB_FIELD_IN_ELEMENT = 1u << 13,
  // WrenTabState.alerts changed: any of its bits, not only the one Chrome
  // shows.
  WREN_TAB_FIELD_ALERTS = 1u << 14,
  // The tab was muted or unmuted. See WrenTabState.muted.
  WREN_TAB_FIELD_MUTED = 1u << 15,
  // The tab was kept out of the session or put back. See
  // WrenTabState.session_excluded.
  WREN_TAB_FIELD_SESSION_EXCLUDED = 1u << 16,
  // The app set the tab to ignore input or take it again. See
  // WrenTabState.ignores_input.
  WREN_TAB_FIELD_IGNORES_INPUT = 1u << 17,
  // The app set the tab to watch its navigations or stop. See
  // WrenTabState.watches_navigations.
  WREN_TAB_FIELD_WATCHES_NAVIGATIONS = 1u << 18,
  // WrenTabState.media changed. See "A tab's media" in WrenTabs.h.
  WREN_TAB_FIELD_MEDIA = 1u << 19,
  // The media's title, artist, album, source or artwork changed.
  WREN_TAB_FIELD_MEDIA_METADATA = 1u << 20,
} WrenTabField;

// A download (WrenDownloads.h). Unique for the life of the process, and never
// reused. WREN_NO_DOWNLOAD means none.
typedef uint64_t WrenDownloadID;
#define WREN_NO_DOWNLOAD 0
// A prompt before closing or quitting with downloads in progress
// (WrenDownloads.h). Unique for the life of the process.
typedef uint64_t WrenDownloadsPromptID;

// A popover request (WrenUI.h). Unique for the life of the process, and never
// reused. WREN_NO_POPOVER means none.
typedef uint64_t WrenPopoverID;
#define WREN_NO_POPOVER 0

// One change. Wren coalesces changes. Each on_changed call has at most one
// record per Profile, Browser, tab, group, split, popover request, web view
// and download, in the order of each one's first change. A tab's fields are
// combined into one record. Handles are borrowed.
//
// Wren keeps no copy of Chromium's state for the app. When a change arrives,
// read again what it names.
typedef struct WrenChange {
  WrenChangeKind kind;
  // PROFILE_CLOSED, PROFILE_OPENED, DOWNLOADS, BOOKMARKS, EXTENSIONS,
  // DOWNLOAD
  WrenProfile* profile;
  // Every kind but PROFILES, PROFILE_CLOSED, PROFILE_OPENED, WEB_VIEW,
  // DOWNLOADS, BOOKMARKS, QUIT_WARNING, EXTENSIONS, DOWNLOAD and
  // DEFAULT_BROWSER.
  WrenBrowser* browser;
  WrenTabID tab;            // TAB
  uint32_t tab_fields;      // TAB
  WrenTabGroupID group;     // GROUP
  WrenSplitID split;        // SPLIT
  WrenPopoverID popover;    // POPOVER
  WrenWebView* web_view;    // WEB_VIEW
  WrenDownloadID download;  // DOWNLOAD
} WrenChange;

// --- App delegate -----------------------------------------------------------

// What the app wants for a popover Chromium is about to open (WrenUI.h).
typedef enum WrenPopoverAnswer {
  // Chromium's own popover, as if there were no handler.
  WREN_POPOVER_CHROMIUM = 0,
  // The app shows its own popover. The request stays open until the app
  // closes it or Wren withdraws it.
  WREN_POPOVER_APP = 1,
  // Nothing is shown. A prompt counts as dismissed without a decision.
  WREN_POPOVER_NONE = 2,
} WrenPopoverAnswer;

typedef struct WrenPopoverRequest WrenPopoverRequest;
// What the app wants for a watched tab's navigation (WrenTabs.h,
// "Navigations").
typedef enum WrenNavigationAnswer {
  // The navigation goes ahead.
  WREN_NAVIGATION_ALLOW = 0,
  // The navigation stops. The tab stays on its page, with no error page.
  WREN_NAVIGATION_CANCEL = 1,
  // The navigation stops, and its URL opens in a new tab in front, as a link
  // with target=_blank opens. The tab stays on its page.
  WREN_NAVIGATION_OPEN_IN_NEW_TAB = 2,
} WrenNavigationAnswer;
typedef struct WrenNavigationRequest WrenNavigationRequest;
typedef struct WrenDownloadsPrompt WrenDownloadsPrompt;
typedef struct WrenQuitPrompt WrenQuitPrompt;
typedef struct WrenWindowRequest WrenWindowRequest;
// Defined in WrenExtensions.h, "App extensions".
typedef struct WrenProfileExtensionsRequest WrenProfileExtensionsRequest;

// Chromium's kinds of Browser, one for one. What a Browser's window shows
// comes from Wren_GetWindowFeatures, not from its type, because two popups
// can differ.
typedef enum WrenBrowserType {
  // A tabbed window.
  WREN_BROWSER_NORMAL = 0,
  // A page's popup, opened by window.open with features, or a window
  // Chromium opened.
  WREN_BROWSER_POPUP = 1,
  // Undocked DevTools.
  WREN_BROWSER_DEVTOOLS = 2,
  // An installed web app's window.
  WREN_BROWSER_APP = 3,
  // An extension's popup window, from chrome.windows.create with type
  // "popup", or a web app's popup.
  WREN_BROWSER_APP_POPUP = 4,
  // A Document Picture-in-Picture window, when Chromium gives it a Browser.
  WREN_BROWSER_PICTURE_IN_PICTURE = 5,
} WrenBrowserType;

// The parts of a window Chrome gives a Browser. Bits of
// Wren_GetWindowFeatures.
typedef enum WrenWindowFeature {
  WREN_WINDOW_FEATURE_TITLE_BAR = 1u << 0,
  WREN_WINDOW_FEATURE_TAB_STRIP = 1u << 1,
  // Navigation buttons, extensions and the rest of Chrome's toolbar.
  WREN_WINDOW_FEATURE_TOOLBAR = 1u << 2,
  // The address. Without a toolbar, Chrome shows it read-only, as in a popup.
  WREN_WINDOW_FEATURE_LOCATION_BAR = 1u << 3,

  WREN_WINDOW_FEATURE_BOOKMARK_BAR = 1u << 4,
} WrenWindowFeature;

// The app's callbacks. They run on the main thread. Only on_browser_opened is
// required.
typedef struct WrenAppDelegate {
  void* context;
  // Chromium's browser process is running. Create windows now. Wren opens no
  // Browser and no window by itself.
  //
  // It runs before AppKit's event loop starts. So an accessory app sets its
  // activation policy here, because a Wren app cannot declare LSUIElement.
  void (*on_ready)(void* context);
  // Required. Chromium opened a Browser the app did not create, and wants it
  // shown. It may be a popup, a window.open, an extension's window, undocked
  // DevTools, or a restored window.
  //
  // The app receives one reference. It shows the Browser in a window of its
  // own, or closes it with Wren_CloseBrowsers. `opener` is the Browser of the
  // tab that opened the Browser's first tab (WrenTabState.opened_by, set
  // already), or NULL when no tab opened it or the app does not know that
  // tab's Browser.
  //
  // Wren_IsBrowserActive tells how Chromium wants it shown. If true, show the
  // window and make it key. If false, Chromium showed it without activating
  // it, as for a window an extension opens unfocused. Order the window in
  // behind the key window, without making it key.
  void (*on_browser_opened)(void* context,
                            WrenBrowser* browser,
                            WrenBrowserType type,
                            WrenBrowser* opener);
  // Something Wren reports changed. See WrenChange. `changes` is valid only
  // during the call.
  void (*on_changed)(void* context, const WrenChange* changes, size_t count);
  // The Browser closed, for any reason. Wren reports no more changes to it.
  void (*on_browser_closed)(void* context, WrenBrowser* browser);
  // The user clicked the Dock icon while the app was running. AppKit's default
  // handling also runs.
  void (*on_reopen)(void* context, bool has_visible_windows);
  // The system asked the app to open URLs, through an open-URL Apple Event, a
  // file or Handoff. Wren opens nothing itself. The app declares its own URL
  // schemes and document types in Info.plist. The strings are UTF-8, and valid
  // only during the call.
  void (*on_open_urls)(void* context, const char* const* urls, size_t count);
  // The app is quitting. Runs once, after on_quit_committed and every
  // on_browser_closed.
  void (*on_will_quit)(void* context);
  // Chromium wants the app's address field focused for the Browser's active
  // tab, such as when Wren_FocusTab finds the New Tab page, or for Search the
  // Web in Chrome's menu. Wren_GetLocationFocus says which (WrenOmnibox.h).
  // Runs from the event loop.
  void (*on_focus_location_bar)(void* context, WrenBrowser* browser);
  // Chromium is about to open one of its popovers. The app answers whether the
  // popover is Chromium's, the app's, or none. NULL leaves every popover to
  // Chromium. Runs inside Chromium's call. WrenUI.h says what the app may do
  // from it.
  WrenPopoverAnswer (*on_popover)(void* context,
                                  const WrenPopoverRequest* request);
  // Chromium wants `browser`'s window in full screen, if `fullscreen`, or out
  // of it. A page asked, through requestFullscreen or exitFullscreen, the
  // user pressed Escape, or Chromium's command ran.
  //
  // The app returns whether it will make the change. If it will, it starts
  // the change with NSWindow's toggleFullScreen:, and reports it with
  // Wren_SetFullscreen. If it will not, or this callback is NULL, the page
  // leaves full screen at once.
  //
  // Runs inside Chromium's call, with on_popover's rules (WrenUI.h).
  // Wren_SetFullscreen is among the calls refused until it returns.
  bool (*on_fullscreen)(void* context, WrenBrowser* browser, bool fullscreen);
  // The user chose an APP item of the main menu the app set (Wren_SetMainMenu,
  // WrenMenus.h), by a click or by its key, or of the Dock menu
  // (Wren_SetDockMenu). `id` is the item's ID, valid only
  // during the call. `browser` is the Browser in the key window, the one
  // Chromium's commands in the menu act on, or NULL. Runs from the event
  // loop.
  void (*on_menu_command)(void* context, const char* id, WrenBrowser* browser);
  // Downloads in progress would be cancelled, because the app is quitting or
  // an off-the-record Profile's last Browser is closing (WrenDownloads.h).
  // The app answers whose prompt it is, as for a popover:
  //
  // - WREN_POPOVER_CHROMIUM shows Chrome's alert or dialog, as if there were
  //   no handler.
  // - WREN_POPOVER_APP means the app asks the user, then answers with
  //   Wren_AnswerDownloadsPrompt. The quit or close waits until it does.
  // - WREN_POPOVER_NONE cancels the downloads without asking.
  //
  // Runs inside Chromium's call, with on_popover's rules (WrenUI.h).
  WrenPopoverAnswer (*on_downloads_prompt)(void* context,
                                           const WrenDownloadsPrompt* prompt);
  // A persistent or ephemeral Profile is loading, and Wren asks which of
  // WrenStartup.extensions it gets, and how each starts there. The request
  // names the Profile and its kind, and has one entry for each extension.
  // The app changes the entries before it returns. The request is valid only
  // during the call. WrenExtensions.h, "App extensions", has the details.
  //
  // Wren asks once for each Profile in a launch, before Chromium loads the
  // Profile's extensions, and so before Wren has a WrenProfile for it. It may
  // run before on_ready. It runs inside Chromium's call. Make no Wren calls
  // from it. An off-the-record Profile has what its original has, and is
  // never asked about.
  //
  // NULL gives every Profile every extension, each as WrenAppExtension says.
  // Wren never asks without extensions.
  void (*on_profile_extensions)(void* context,
                                WrenProfileExtensionsRequest* request);
  // The user pressed the Quit item's key, ⌘Q, while Warn Before Quitting is
  // on (Wren_GetQuitWarningState), and Chromium would warn. The app answers
  // whose warning it is, as for a popover:
  //
  // - WREN_POPOVER_CHROMIUM shows Chrome's "Hold ⌘Q to Quit", as if there
  //   were no handler.
  // - WREN_POPOVER_APP means the app asks the user, then answers with
  //   Wren_AnswerQuitPrompt. The quit waits until it does.
  // - WREN_POPOVER_NONE quits without a warning.
  //
  // Chromium warns only for the key, as Chrome does. A click on Quit, the
  // Dock's Quit, logging out and Wren_Quit never ask.
  //
  // The key pressed again while the app's prompt is open asks again, with
  // `again` set and the open prompt's ID, as Chrome's warning quits on a
  // second press. WREN_POPOVER_NONE quits, and the prompt is closed. Any other
  // answer leaves the prompt open and the app running.
  //
  // Runs inside Chromium's call, with on_popover's rules (WrenUI.h).
  WrenPopoverAnswer (*on_quit_prompt)(void* context,
                                      const WrenQuitPrompt* prompt);
  // The app is quitting, and the quit can no longer be cancelled. Every
  // page's beforeunload and the downloads prompt have passed. Runs once, before
  // the quit's first on_browser_closed. Then each Browser closes and
  // on_will_quit runs.
  //
  // It runs for ⌘Q, Quit in a menu or the Dock, logging out and Wren_Quit.
  // With no Browser open, it runs just before on_will_quit. When the system
  // ends the process with SIGTERM, as launchd may at logout or shutdown, it
  // runs and then the process ends, with no on_browser_closed and no
  // on_will_quit.
  //
  // An app that shows several Browsers in one window hides its windows here,
  // so they do not empty one Browser at a time. It may also save its own
  // records.
  //
  // Runs inside Chromium's call, with on_popover's rules (WrenUI.h).
  void (*on_quit_committed)(void* context);
  // Chromium asks for `browser`'s window to move, resize, minimise, zoom or
  // restore (WrenWindowRequest, WrenBrowser.h). An extension's
  // chrome.windows.update, a debugging tool, or a popup's resizeTo or moveTo
  // asked. Wren does nothing itself. The app does what was asked, adapts it,
  // or ignores it, and Chromium then reads the window as it is. NULL ignores
  // every request, as before 0.3.0.
  //
  // A frame, or the normal state, for a window in full screen means leaving
  // full screen first, as Chrome does. A Browser in the background shares its
  // window, so ignore its requests, or bring it forward first.
  //
  // Wren asks only for a Browser attached to a view, and only for what its
  // window is not already. Runs from the event loop. `request` is valid only
  // during the call.
  void (*on_window_request)(void* context,
                            WrenBrowser* browser,
                            const WrenWindowRequest* request);
  // Chromium's About page asks for an update check: a page opened, or asked
  // again. Never while the update state is WREN_UPDATE_NONE (WrenUpdates.h).
  // The app decides whether to check, and tells the page what its updater
  // does with Wren_SetUpdateState. Runs from the event loop.
  void (*on_update_check)(void* context);
  // A tab that watches its navigations (Wren_SetTabWatchesNavigations) is
  // about to go to another page, or a server redirected it. The app answers
  // whether it goes ahead, stops, or opens in a new tab. WrenTabs.h,
  // "Navigations", says which navigations are asked. `request` is valid only
  // during the call. NULL lets every navigation go ahead.
  //
  // Runs inside Chromium's call, with on_popover's rules (WrenUI.h). Calls
  // that navigate, such as Wren_Navigate, are among those refused until it
  // returns. Work that follows, such as closing a tab left blank, goes in
  // Wren_PostToMainThread.
  WrenNavigationAnswer (*on_navigation)(void* context,
                                        const WrenNavigationRequest* request);
} WrenAppDelegate;

// --- Lifecycle and version --------------------------------------------------

typedef enum WrenExit {
  WREN_EXIT_NORMAL = 0,
  // The startup settings or the delegate were invalid, and Chromium never
  // started. Wren prints the reason to stderr.
  WREN_EXIT_INVALID_STARTUP = 1,
  // Another process holds user_data_dir. Wren told that process about this
  // launch, and this one opens nothing.
  WREN_EXIT_ALREADY_RUNNING = 2,
  // Chromium exited with an error.
  WREN_EXIT_CHROMIUM_ERROR = 3,
  // WrenStartup.api_version is not this framework's WREN_API_VERSION, or
  // unstable_ui_version is not its WREN_UNSTABLE_UI_VERSION.
  WREN_EXIT_API_MISMATCH = 4,
} WrenExit;

// Runs Chromium's browser process in this process. It creates NSApplication
// and runs its event loop, in place of NSApplicationMain. It returns after
// quit, once every Profile and Browser handle reports closed.
//
// Handles the app still holds stay valid after it returns. They report
// closed. The app may release them then, as an object that owns one does when
// it is destroyed, or not at all.
WREN_API WrenExit Wren_Main(const WrenStartup* startup,
                            const WrenAppDelegate* delegate);

// Asks the app to quit. A page may cancel the quit with beforeunload. With
// downloads in progress, the user or the app may cancel it
// (on_downloads_prompt). Otherwise on_quit_committed runs, every Browser gets
// on_browser_closed, then on_will_quit runs, and Wren_Main returns
// WREN_EXIT_NORMAL.
//
// It never warns, whether or not Warn Before Quitting is on: only the Quit
// item's key does (on_quit_prompt). An app that wants the user to confirm
// asks first, then calls this.
WREN_API void Wren_Quit(void);

// Warn Before Quitting, Chrome's setting. While it is on, the Quit item's key
// warns before quitting (on_quit_prompt). It is on unless the user or policy
// turned it off. Chrome's menu item and chrome://settings change it too, and
// WREN_CHANGE_QUIT_WARNING reports every change.
typedef struct WrenQuitWarningState {
  // ⌘Q warns before quitting.
  bool on;
  // Policy sets it, and Wren_SetQuitWarning refuses to change it.
  bool managed;
} WrenQuitWarningState;

// Fills `state`. Before on_ready and after quit, it is off.
WREN_API void Wren_GetQuitWarningState(WrenQuitWarningState* state);

// Turns Warn Before Quitting on or off for every Profile, and for later
// launches. Returns WREN_ERROR_NOT_ALLOWED when policy sets it, as Chrome's
// menu item is disabled then, or inside a callback that runs inside
// Chromium's call.
WREN_API WrenResult Wren_SetQuitWarning(bool on);

// The ID of a quit prompt, unique for the life of the process.
typedef uint64_t WrenQuitPromptID;

// Chromium is about to warn before quitting (on_quit_prompt).
struct WrenQuitPrompt {
  WrenQuitPromptID prompt;
  // The key was pressed again while this prompt, which the app answered
  // WREN_POPOVER_APP, is open.
  bool again;
};

// Answers a prompt for which the app answered WREN_POPOVER_APP. With `quit`,
// the app quits, as Wren_Quit does, without a warning. Without it, the app
// keeps running.
//
// One prompt is open at a time, and Wren never withdraws it. A quit from
// elsewhere goes on while it is open. If a page's beforeunload cancels that
// quit, the prompt is still open.
//
// Returns WREN_ERROR_POPOVER_CLOSED for a prompt already answered, or once the
// app has quit, and WREN_ERROR_NOT_ALLOWED inside a callback that runs inside
// Chromium's call.
WREN_API WrenResult Wren_AnswerQuitPrompt(WrenQuitPromptID prompt, bool quit);

// Returns the framework's WREN_API_VERSION. May be called at any time.
WREN_API uint32_t Wren_GetAPIVersion(void);

typedef struct WrenVersionInfo {
  const char* wren_version;      // "0.1.0" in a release, "0.1.0-dev" otherwise
  const char* chromium_version;  // for example "155.0.8059.12"
  const char* chromium_commit;
  uint32_t patch_revision;  // Wren's Chromium patch series
} WrenVersionInfo;

// Fills `info` with static strings. May be called at any time.
WREN_API void Wren_GetVersionInfo(WrenVersionInfo* info);

// --- The default browser ----------------------------------------------------
//
// Whether the app is the browser macOS opens web links with, and asking the
// user to make it so, as Chrome's Settings do. The app is the default browser
// when it opens http links. For macOS to offer it, its Info.plist declares the
// http and https schemes and the HTML type (`wren bundle`'s `urlSchemes` and
// `documentTypes`).

typedef enum WrenDefaultBrowser {
  // macOS did not say, or the app has no bundle ID.
  WREN_DEFAULT_BROWSER_UNKNOWN = 0,
  WREN_DEFAULT_BROWSER_NO = 1,
  WREN_DEFAULT_BROWSER_YES = 2,
} WrenDefaultBrowser;

typedef struct WrenDefaultBrowserState {
  WrenDefaultBrowser state;
  // False where the DefaultBrowserSettingEnabled policy is off. Chrome's
  // Settings disable their button then, and Wren_SetAsDefaultBrowser refuses.
  bool can_set;
} WrenDefaultBrowserState;

// Receives the answer of Wren_GetDefaultBrowserState or
// Wren_SetAsDefaultBrowser, from the event loop. `state` is NULL unless
// `result` is WREN_OK, and borrowed until the callback returns.
typedef void (*WrenDefaultBrowserCallback)(void* context,
                                           const WrenDefaultBrowserState* state,
                                           WrenResult result);

// Asks macOS whether the app is the default browser, off the main thread, and
// gives the answer to `callback` from the event loop. The call returns at once.
// `callback` is required. It is not called before on_ready, or once the app
// quits.
//
// From the first call, or the first Wren_SetAsDefaultBrowser, Wren asks again
// each time the app becomes active, and reports WREN_CHANGE_DEFAULT_BROWSER
// whenever an answer differs from the one before. Someone who changes the
// default browser in System Settings leaves the app to do so. A change made
// while the app stays active is seen the next time it becomes active.
WREN_API void Wren_GetDefaultBrowserState(WrenDefaultBrowserCallback callback,
                                          void* context);

// Asks macOS to make the app the default browser, as Chrome's Settings do.
// macOS asks the user, and `callback` gets the answer from the event loop
// about when its prompt goes away: WREN_DEFAULT_BROWSER_YES if the user
// accepted, else what the default browser is. The call returns at once.
// `callback` is required. It is not called before on_ready, or once the app
// quits. There is no time limit: the user may leave the prompt open.
//
// - While a prompt is open, another call does not ask macOS again. It gets
//   the open prompt's answer.
// - The app's windows stay where they are. macOS's prompt comes in front.
//
// `callback` gets WREN_ERROR_NOT_ALLOWED where the DefaultBrowserSettingEnabled
// policy is off, or for a call made inside a callback that runs inside
// Chromium's call.
WREN_API void Wren_SetAsDefaultBrowser(WrenDefaultBrowserCallback callback,
                                       void* context);

// --- Work from other threads -------------------------------------------------
//
// For an app that runs another event loop beside Wren's, such as an embedded
// runtime's: its thread waits for its own work, then hands the work to the
// main thread, where Wren's calls are made.

// Runs or discards work an app posted. `context` is the app's.
typedef void (*WrenPostedWork)(void* context);

// Asks Wren to run `run` on the main thread, from the event loop. It may be
// called on any thread, the main thread included, and wakes the event loop if
// it is waiting. It makes no other function callable off the main thread.
//
// - `run` never runs inside this call, or inside a Chromium notification. So
//   it may call any function, as on_changed may.
// - Work posted from one thread runs in the order it was posted. There is no
//   order between threads, or against Wren's own callbacks.
// - It also runs inside a nested loop where Chromium lets tasks run, such as
//   while a context menu is open.
// - Work that has not run when the event loop ends is discarded: `discard`
//   runs instead, on the main thread, before Wren_Main returns. `discard` may
//   be NULL. On SIGTERM nothing more runs, as on_will_quit does not.
//
// Returns WREN_OK once Wren has the work. Then exactly one of `run` and
// `discard` runs. Otherwise neither runs, and `context` is still the app's:
//
// - WREN_ERROR_NOT_ALLOWED before on_ready
// - WREN_ERROR_CLOSED once the event loop has ended
// - WREN_ERROR_INVALID_ARGUMENT for a NULL `run`
WREN_API WrenResult Wren_PostToMainThread(WrenPostedWork run,
                                          WrenPostedWork discard,
                                          void* context);

// As Wren_PostToMainThread, but `run` runs once `delay_ms` milliseconds have
// passed, by a clock that stops while the Mac sleeps, as GCD's does. Work
// due at the same time runs in the order it was posted. There is no way to
// cancel it: an app that may not want it keeps a flag in `context`. A delay
// of 0 is Wren_PostToMainThread's. Work still waiting when the event loop
// ends is discarded, as there. It also returns WREN_ERROR_INVALID_ARGUMENT
// for a negative delay.
WREN_API WrenResult Wren_PostToMainThreadAfter(int64_t delay_ms,
                                               WrenPostedWork run,
                                               WrenPostedWork discard,
                                               void* context);

#ifdef __cplusplus
}  // extern "C"
#endif

// Profiles, Browsers, full screen and side panels.
#include "WrenBrowser.h"
// Tabs, navigation, groups, splits and DevTools.
#include "WrenTabs.h"
// Extensions' actions and the installed list.
#include "WrenExtensions.h"
// Anchors, page actions and content setting icons.
#include "WrenToolbar.h"
// Downloads, the download button, and the prompts before closing or quitting.
#include "WrenDownloads.h"
// Chromium's popovers as requests and data. It includes WrenUnstableUI.h,
// which has Chrome's sentences and the layout of its bubbles, and is
// unstable.
#include "WrenUI.h"
// Chromium's omnibox as data.
#include "WrenOmnibox.h"
// A Profile's bookmarks as state.
#include "WrenBookmarks.h"
// Chromium's session restore and closed tabs.
#include "WrenSession.h"
// The app's own URL schemes.
#include "WrenSchemes.h"
// Pages the app places that are not tabs.
#include "WrenWebView.h"
// Menus the app describes as data.
#include "WrenMenus.h"
// The system clipboard.
#include "WrenClipboard.h"
// The app's updates, and relaunching.
#include "WrenUpdates.h"

#endif  // WREN_PUBLIC_WREN_H_
