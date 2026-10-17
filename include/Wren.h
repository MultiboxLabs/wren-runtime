// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: Chromium's browser layer in your app's process (C API).
//
// Threading and timing:
// - Every function except Wren_Main, Wren_GetAPIVersion and
//   Wren_GetVersionInfo must be called on the main thread after on_ready.
// - Completion callbacks (Wren_OpenProfile, Wren_CreateBrowser, ...) always
//   run later from the event loop, never inside the call that started them.
// - on_changed, on_reopen and on_open_urls run from the event loop, never
//   inside a Chromium notification, so a callback may call any function.
//   on_popover, on_fullscreen and on_downloads_prompt are the exceptions:
//   they run inside Chromium's call (see WrenUI.h).
// - Closing (Wren_CloseTab, Wren_CloseBrowser) and Wren_Quit are requests that
//   take effect after the caller returns. Other calls take effect before they
//   return; the matching on_changed record follows.
//
// Handles and identity:
// - WrenProfile and WrenBrowser handles are reference-counted. A callback that
//   creates or opens one hands the app one reference; handles passed to other
//   callbacks or returned by Get functions are borrowed. A handle stays valid
//   until its last reference is released, even after what it names closes;
//   calls on it then return WREN_ERROR_CLOSED or do nothing.
// - One Chromium Profile or Browser always has the same handle.
// - Tabs are WrenTabID values. An ID is unique for the life of the process and
//   never reused; it follows the tab through moves, including to another
//   Browser. Calls with the ID of a closed tab return
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

// The API this header and the ones it includes describe, but for
// WrenUnstableUI.h, which has its own (WREN_UNSTABLE_UI_VERSION). Set
// WrenStartup.api_version to it; Wren_Main refuses any other version. See
// docs/apps/compatibility.md.
#define WREN_API_VERSION 36

typedef struct WrenProfile WrenProfile;
typedef struct WrenBrowser WrenBrowser;
// A page the app places that is not a tab (WrenWebView.h).
typedef struct WrenWebView WrenWebView;

typedef int32_t WrenTabID;
#define WREN_NO_TAB 0
// A Browser's ID: the name a web view's page has for it (WrenWebView.h),
// where a handle cannot go. Unique for the life of the process.
typedef int32_t WrenBrowserID;
#define WREN_NO_BROWSER 0

// A tab group, unique and stable while the group exists, including when it
// moves. All zero means no group.
typedef struct WrenTabGroupID {
  uint64_t high;
  uint64_t low;
} WrenTabGroupID;

// A split: two tabs shown side by side as one. Unique while the split exists.
// All zero means no split.
typedef struct WrenSplitID {
  uint64_t high;
  uint64_t low;
} WrenSplitID;

// A group's or a split's ID as text, for a page or a file: Chromium's own
// form, 32 upper-case hexadecimal digits, `high` first. All zero, none, is
// 32 zeros. The size includes the NUL. These four may be called on any
// thread at any time.
#define WREN_ID_TEXT_SIZE 33
WREN_API void Wren_FormatTabGroupID(WrenTabGroupID id,
                                    char text[WREN_ID_TEXT_SIZE]);
// Reads what Wren_FormatTabGroupID writes. False, and `id` all zero, for any
// other text: another length, lower case, anything but hexadecimal digits.
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
  // Chromium's policy forbids it, for example off-the-record browsing.
  WREN_ERROR_NOT_ALLOWED = 4,
  // An argument is missing or malformed, for example an invalid URL, or tabs
  // from different Browsers in one call.
  WREN_ERROR_INVALID_ARGUMENT = 5,
  // No open tab group has this ID.
  WREN_ERROR_GROUP_UNAVAILABLE = 6,
  // The tab cannot go back (or forward) now.
  WREN_ERROR_NAVIGATION_UNAVAILABLE = 7,
  // No open split has this ID.
  WREN_ERROR_SPLIT_UNAVAILABLE = 8,
  // No side panel with this ID is available in the Browser now.
  WREN_ERROR_PANEL_UNAVAILABLE = 9,
  // The popover request has closed (WrenUI.h).
  WREN_ERROR_POPOVER_CLOSED = 10,
  // The page token is not the Browser's current page's (WrenUI.h): the tab
  // navigated or another tab became active since it was read.
  WREN_ERROR_STALE_PAGE = 11,
  // The suggestions have changed since this WrenOmniboxResult was read
  // (WrenOmnibox.h).
  WREN_ERROR_STALE_RESULT = 12,
  // The download is not one Chromium has (WrenDownloads.h).
  WREN_ERROR_DOWNLOAD_UNAVAILABLE = 13,
} WrenResult;

// Why a page is gone: a tab's (WrenTabs.h) or a web view's (WrenWebView.h).
typedef enum WrenTabCrash {
  WREN_TAB_CRASH_NONE = 0,
  // The page's renderer crashed.
  WREN_TAB_CRASH_CRASHED = 1,
  // The renderer was killed, by the system or chrome://kill.
  WREN_TAB_CRASH_KILLED = 2,
  // The renderer ran out of memory.
  WREN_TAB_CRASH_OUT_OF_MEMORY = 3,
} WrenTabCrash;

// A content setting: an icon in the address field (WrenToolbar.h) and what
// its bubble shows (WrenUI.h).
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

// Where Chromium's encryption key (cookies, passwords) is kept.
typedef enum WrenKeychain {
  // The login Keychain, as the item "<keychain_name> Safe Storage". Needs a
  // stably signed app; ad hoc builds prompt for access after every rebuild.
  WREN_KEYCHAIN_SYSTEM = 1,
  // A fixed in-memory key. For development and tests only.
  WREN_KEYCHAIN_MOCK = 2,
} WrenKeychain;

// The main menu Wren installs before on_ready. Afterwards the app may set
// its own from data (Wren_SetMainMenu, WrenMenus.h), or edit or replace
// NSApp.mainMenu itself.
typedef enum WrenMenu {
  // App, File, Edit, View and Window menus, like Electron's default. View's
  // Reload, Force Reload, Toggle Developer Tools and zoom items act on the
  // key window's Browser (the one attached to a view in it).
  WREN_MENU_DEFAULT = 0,
  // Chrome's main menu, without the lists Chrome fills in as it runs
  // (profiles, bookmarks, history, open tabs, saved groups). Its commands act
  // on the key window's Browser, or, with none, as in Chrome with no window
  // open; Find opens Chromium's find bar, and commands that need UI Wren does
  // not have (Print) are disabled. Edit it like any NSMenu; an item copied
  // from it keeps its target.
  WREN_MENU_CHROMIUM = 1,
} WrenMenu;

// What a URL scheme of the app's may do (WrenScheme.privileges).
typedef enum WrenSchemePrivileges {
  WREN_SCHEME_PLAIN = 0,
  // URLs have a host and a path, as https URLs do ("app://ui/index.html"),
  // so relative URLs resolve, each host is an origin of its own, and pages
  // have web storage. Without it a URL is opaque, as a data: URL is.
  WREN_SCHEME_STANDARD = 1u << 0,
  // Pages are secure contexts, and loading them from an https page is not
  // mixed content.
  WREN_SCHEME_SECURE = 1u << 1,
  // The scheme's own pages may fetch() its URLs. Pages of other origins
  // cannot read its responses.
  WREN_SCHEME_FETCH = 1u << 2,
  // Other pages may reach the scheme: a website, an extension or another of
  // the app's schemes may link to its pages, frame them, and load its
  // scripts and images, as they may an https URL's (they still cannot read
  // its responses). Without it a scheme is closed: its handler is asked only
  // for what the app or the user opens (a tab's or a web view's URL, a typed
  // address) and for what the scheme's own pages ask, and every other
  // request fails unanswered. Leave it out for pages that are the browser's
  // own (its UI, its settings).
  WREN_SCHEME_WEB_ACCESSIBLE = 1u << 3,
} WrenSchemePrivileges;

// A URL scheme the app answers itself (WrenSchemes.h).
typedef struct WrenScheme {
  // A lower-case letter, then lower-case letters, digits, "+", "-" and ".",
  // up to 64 characters, for example "app". Not one of Chromium's own (https,
  // file, chrome, chrome-extension, about, data, mailto and the like).
  const char* name;
  // WrenSchemePrivileges, combined.
  uint32_t privileges;
} WrenScheme;

// How Chromium starts. Every field is read before Wren_Main returns control
// to Chromium; strings and the icon are copied.
typedef struct WrenStartup {
  // Required: WREN_API_VERSION.
  uint32_t api_version;
  // Required: WREN_UNSTABLE_UI_VERSION (WrenUnstableUI.h).
  uint32_t unstable_ui_version;
  // Required. Absolute path; one running process per directory.
  const char* user_data_dir;
  // Required. The startup Profile's directory inside user_data_dir, one path
  // component, for example "Default".
  const char* profile_directory;
  // Required.
  WrenKeychain keychain;
  // Required for WREN_KEYCHAIN_SYSTEM, for example "Wren Sample". Keep it the
  // same across app versions: a new name means a new key, and existing
  // cookies and passwords can no longer be decrypted.
  const char* keychain_name;
  // Optional extra Chromium switches for development, each starting with
  // "--". Switches that would override this policy are rejected.
  const char* const* development_switches;
  size_t development_switch_count;
  WrenMenu menu;
  // Optional. The app's name in Chromium's own UI (WebUI pages such as
  // Settings and the New Tab page, dialogs and prompts), in place of
  // "Chromium", "Chrome" and "Google Chrome". Google's products (the Chrome
  // Web Store) and strings about the Chromium project (copyright, credits)
  // keep their names. NULL keeps Chromium's. 1 to 64 characters, with no
  // surrounding whitespace, no control characters and none of $ { } # < > & ".
  const char* product_name;
  // Optional. A square PNG, 256 to 4096 pixels wide, shown as Chromium's
  // product logo (Settings > About, chrome://version, dialogs). NULL keeps
  // Chromium's. On its own it serves every variant below.
  const void* product_icon_png;
  size_t product_icon_png_size;
  // Optional variants, each shown where Chromium shows that variant of its
  // logo; they need product_icon_png. Where each appears, and what a NULL one
  // falls back to:
  // - product_icon_png: Settings > About, WebUI toolbars (Settings,
  //   Extensions, History) in light mode, chrome://version in light mode,
  //   dialogs. Wren renders exact sizes from it, so they stay sharp.
  // - product_icon_dark_png: chrome://version in dark mode. Falls back to
  //   product_icon_png.
  // - product_icon_svg: the profile picker, first-run and search engine
  //   choice pages, the default-browser dialog. Falls back to
  //   product_icon_png.
  // - product_icon_dark_svg: WebUI toolbars in dark mode. Falls back to the
  //   dark PNG, then the SVG, then product_icon_png.
  // Chromium's light toolbars always use a PNG, never an SVG.
  // The dark PNG follows product_icon_png's rules. An SVG is UTF-8 text that
  // starts with <svg or <?xml, up to 1 MiB.
  const void* product_icon_dark_png;
  size_t product_icon_dark_png_size;
  const void* product_icon_svg;
  size_t product_icon_svg_size;
  const void* product_icon_dark_svg;
  size_t product_icon_dark_svg_size;
  // Optional, shown as given (in every language) in place of Chromium's
  // copyright line and company name: Settings > About and chrome://version.
  // NULL keeps Chromium's ("The Chromium Authors"). The same rules as
  // product_name, up to 128 characters for the copyright line.
  const char*
      copyright;  // e.g. "Copyright 2026 Wren Authors. All rights reserved."
  const char* company_name;  // e.g. "Wren Authors"
  // Optional. An https URL that Settings > About's "Get help" opens in a new
  // tab. NULL hides the row; Chromium's own help is Google's.
  const char* help_url;
  // Optional. The app's own URL schemes. Chromium fixes its list of schemes
  // as it starts, so they cannot be added later; their requests are answered
  // by the handlers the app sets after on_ready (Wren_SetSchemeHandler).
  const WrenScheme* schemes;
  size_t scheme_count;
} WrenStartup;

// --- Changes ----------------------------------------------------------------

typedef enum WrenChangeKind {
  // The list of persistent Profiles changed; see Wren_GetProfileListCount.
  WREN_CHANGE_PROFILES = 0,
  // `profile` closed.
  WREN_CHANGE_PROFILE_CLOSED = 1,
  // `browser`'s tabs were added, removed or moved, or its active tab changed.
  // Read its tabs again with Wren_GetTabCount and Wren_GetTabAt.
  WREN_CHANGE_TABS = 2,
  // `tab`, now in `browser`, changed the fields in `tab_fields`.
  WREN_CHANGE_TAB = 3,
  // `group`'s title, colour, collapsed state or tabs changed, or it closed.
  // `browser` is its Browser, or NULL once it has closed.
  WREN_CHANGE_GROUP = 4,
  // `split`'s tabs, order, layout or ratio changed, or it ended. `browser` is
  // its Browser, or NULL once it has ended.
  WREN_CHANGE_SPLIT = 5,
  // `browser`'s side panel opened, closed or changed to another panel, or
  // the open panel's width changed; see Wren_GetSidePanelState.
  WREN_CHANGE_SIDE_PANEL = 6,
  // A browser-modal dialog of `browser`'s opened or closed; see
  // Wren_HasModalDialog.
  WREN_CHANGE_MODAL_DIALOG = 7,
  // `browser`'s extension actions were added, removed, pinned or reordered,
  // or one's icon, name, tooltip, badge, enabled state, popup or pop-out
  // changed, including on a tab switch; or its Profile's installed
  // extensions changed (installed, removed, turned on or off, an icon
  // loaded). Read them again.
  WREN_CHANGE_EXTENSION_ACTIONS = 8,
  // A page action of `browser`'s active tab changed (shown, hidden, its
  // label, tooltip, icon or bubble), or the active tab did. Read them again.
  WREN_CHANGE_PAGE_ACTIONS = 9,
  // `browser`'s content setting icons changed, for its active tab. Read them
  // again.
  WREN_CHANGE_CONTENT_SETTINGS = 10,
  // A popover request of `browser`'s that the app answered with
  // WREN_POPOVER_APP (`popover`) changed or closed; see WrenUI.h.
  WREN_CHANGE_POPOVER = 11,
  // What `browser`'s active tab's site shows changed (its security,
  // permissions, devices or cookies), or the active tab or its page did; see
  // Wren_GetSiteState. Reported once the app has read it.
  WREN_CHANGE_SITE = 12,
  // What `browser`'s extensions may do on its active tab's site changed
  // (site setting, access, requests), or the page did; see
  // Wren_GetExtensionsSiteState. Reported once the app has read it.
  WREN_CHANGE_EXTENSIONS_SITE = 13,
  // `browser`'s active tab's zoom changed, or the active tab did; see
  // Wren_GetZoomState. Reported once the app has read it.
  WREN_CHANGE_ZOOM = 14,
  // Whether `browser`'s active tab's page is bookmarked, or its bookmark,
  // changed, or the page or active tab did; see Wren_GetBookmarkState.
  // Reported once the app has read it.
  WREN_CHANGE_BOOKMARK = 15,
  // A page of `browser`'s entered or left full screen; see
  // Wren_GetFullscreenTab.
  WREN_CHANGE_FULLSCREEN = 16,
  // What `browser`'s address field shows changed: the active tab's address,
  // the active tab, or the default search engine; see Wren_GetLocationState
  // (WrenOmnibox.h). Reported once the app has read it.
  WREN_CHANGE_LOCATION = 17,
  // `browser`'s suggestions changed, or a suggestion's icon arrived; see
  // Wren_GetOmniboxState (WrenOmnibox.h).
  WREN_CHANGE_OMNIBOX = 18,
  // Whether `browser` has a closed tab to reopen changed; see
  // Wren_CanReopenClosedTab (WrenSession.h). Reported once the app has read
  // it.
  WREN_CHANGE_CLOSED_TABS = 19,
  // Chromium asked for `browser`, which is in the background, to come
  // forward; see Wren_HasForegroundRequest.
  WREN_CHANGE_FOREGROUND_REQUEST = 20,
  // `web_view`'s page began or stopped loading, failed to load, or its
  // process ended or was started again; see Wren_GetWebViewState
  // (WrenWebView.h).
  WREN_CHANGE_WEB_VIEW = 21,
  // The state of one or more of `browser`'s anchors changed: named, placed,
  // or a bubble of Chromium's opened or closed at one; see
  // Wren_GetAnchorState (WrenToolbar.h).
  WREN_CHANGE_ANCHORS = 22,
  // `browser` became active or inactive; see Wren_IsBrowserActive
  // (WrenBrowser.h).
  WREN_CHANGE_ACTIVE = 23,
  // `browser`'s find changed: it opened or ended, its string or its matches
  // changed, or the active tab did; see Wren_GetFindState (WrenUI.h).
  WREN_CHANGE_FIND = 24,
  // `profile`'s downloads changed: one was added or went, or one's progress
  // or state changed; see Wren_GetDownloadCount (WrenDownloads.h). Reported
  // once the app has read them.
  WREN_CHANGE_DOWNLOADS = 25,
  // `browser`'s download button changed; see Wren_GetDownloadButtonState
  // (WrenDownloads.h). Reported once the app has read it.
  WREN_CHANGE_DOWNLOAD_BUTTON = 26,
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
  // The page was discarded to save memory, or reloaded after it.
  WREN_TAB_FIELD_DISCARDED = 1u << 7,
  // 1u << 8 was WREN_TAB_FIELD_VIEW: a tab's view now stays the same while
  // the tab lives.
  // The page's renderer crashed or was killed, or the tab was loaded again
  // after it; see WrenTabState.crash.
  WREN_TAB_FIELD_CRASHED = 1u << 9,
  // DevTools for the tab opened, closed, docked to another side or undocked,
  // from Wren_OpenDevTools or DevTools' own dock menu; see
  // WrenTabState.devtools.
  WREN_TAB_FIELD_DEVTOOLS = 1u << 10,
  // The tab joined or left a split; see WrenTabState.split.
  WREN_TAB_FIELD_SPLIT = 1u << 11,
  // What Chrome's status bubble would show for the tab changed: the link
  // under the pointer, the load state's line; see WrenTabState.status and
  // Wren_CopyTabStatusText.
  WREN_TAB_FIELD_STATUS = 1u << 12,
  // The tab's page went into a <wren-view> element of a web view's page, or
  // left it; see WrenTabState.in_element.
  WREN_TAB_FIELD_IN_ELEMENT = 1u << 13,
} WrenTabField;

// A download (WrenDownloads.h). Unique for the life of the process and never
// reused; WREN_NO_DOWNLOAD is none.
typedef uint64_t WrenDownloadID;
#define WREN_NO_DOWNLOAD 0
// A prompt before closing or quitting with downloads in progress
// (WrenDownloads.h). Unique for the life of the process.
typedef uint64_t WrenDownloadsPromptID;

// A popover request (WrenUI.h). Unique for the life of the process and never
// reused; WREN_NO_POPOVER is none.
typedef uint64_t WrenPopoverID;
#define WREN_NO_POPOVER 0

// One change. Changes are coalesced: one record per Profile, Browser, tab,
// group, split, popover request and web view in each on_changed call, in the
// order of its first change, with a tab's fields combined. Handles are
// borrowed.
typedef struct WrenChange {
  WrenChangeKind kind;
  WrenProfile* profile;  // PROFILE_CLOSED, DOWNLOADS
  // Every kind but PROFILES, PROFILE_CLOSED, WEB_VIEW and DOWNLOADS.
  WrenBrowser* browser;
  WrenTabID tab;          // TAB
  uint32_t tab_fields;    // TAB
  WrenTabGroupID group;   // GROUP
  WrenSplitID split;      // SPLIT
  WrenPopoverID popover;  // POPOVER
  WrenWebView* web_view;  // WEB_VIEW
} WrenChange;

// --- App delegate -----------------------------------------------------------

// What the app wants for a popover Chromium is about to open (WrenUI.h).
typedef enum WrenPopoverAnswer {
  // Chromium's own popover, as without a handler.
  WREN_POPOVER_CHROMIUM = 0,
  // The app shows its own; the request stays open until the app closes it
  // or Wren withdraws it.
  WREN_POPOVER_APP = 1,
  // Nothing is shown. A prompt counts as dismissed without a decision.
  WREN_POPOVER_NONE = 2,
} WrenPopoverAnswer;

typedef struct WrenPopoverRequest WrenPopoverRequest;
typedef struct WrenDownloadsPrompt WrenDownloadsPrompt;

// Chromium's kinds of Browser, one for one. What a kind's window shows is
// Wren_GetWindowFeatures, not the type: two popups can differ.
typedef enum WrenBrowserType {
  // A tabbed window.
  WREN_BROWSER_NORMAL = 0,
  // A page's popup (window.open with features), or one Chromium opened.
  WREN_BROWSER_POPUP = 1,
  // Undocked DevTools.
  WREN_BROWSER_DEVTOOLS = 2,
  // An installed web app's window.
  WREN_BROWSER_APP = 3,
  // An extension's popup window (chrome.windows.create with type "popup"),
  // or a web app's popup.
  WREN_BROWSER_APP_POPUP = 4,
  // A Document Picture-in-Picture window, when Chromium gives it a Browser.
  WREN_BROWSER_PICTURE_IN_PICTURE = 5,
} WrenBrowserType;

// The parts of a window Chrome gives a Browser of this kind. Bits of
// Wren_GetWindowFeatures.
typedef enum WrenWindowFeature {
  WREN_WINDOW_FEATURE_TITLE_BAR = 1u << 0,
  WREN_WINDOW_FEATURE_TAB_STRIP = 1u << 1,
  // Navigation buttons, extensions and the rest of Chrome's toolbar.
  WREN_WINDOW_FEATURE_TOOLBAR = 1u << 2,
  // The address. Without a toolbar, Chrome shows it read-only (a popup's).
  WREN_WINDOW_FEATURE_LOCATION_BAR = 1u << 3,
  WREN_WINDOW_FEATURE_BOOKMARK_BAR = 1u << 4,
} WrenWindowFeature;

// Callbacks run on the main thread. Only on_browser_opened is required.
typedef struct WrenAppDelegate {
  void* context;
  // Chromium's browser process is running; create windows now. Runs before
  // AppKit's event loop starts, so an accessory app sets its activation policy
  // here (a Wren app cannot declare LSUIElement).
  void (*on_ready)(void* context);
  // Required. Chromium opened a Browser the app did not create (a popup,
  // window.open, an extension window, DevTools) and wants it shown. The app
  // receives one reference, hosts the Browser in a window of its own, or
  // closes it with Wren_CloseBrowser. `opener` may be NULL.
  void (*on_browser_opened)(void* context,
                            WrenBrowser* browser,
                            WrenBrowserType type,
                            WrenBrowser* opener);
  // Profiles, Browsers or tabs changed. `changes` is valid during the call.
  void (*on_changed)(void* context, const WrenChange* changes, size_t count);
  // The Browser closed, for any reason. Changes to it are no longer reported.
  void (*on_browser_closed)(void* context, WrenBrowser* browser);
  // The Dock icon was clicked while the app was running. AppKit's default
  // handling also runs.
  void (*on_reopen)(void* context, bool has_visible_windows);
  // The system asked the app to open URLs (an open-URL Apple Event, a file,
  // Handoff). Wren opens nothing itself; the app declares its own URL schemes
  // and document types in Info.plist. The strings are UTF-8 and valid during
  // the call.
  void (*on_open_urls)(void* context, const char* const* urls, size_t count);
  // Quit can no longer be cancelled. Runs once, after every on_browser_closed.
  void (*on_will_quit)(void* context);
  // Chromium wants the app's address field focused for the Browser's active
  // tab, for example when Wren_FocusTab finds the New Tab page. Runs from the
  // event loop.
  void (*on_focus_location_bar)(void* context, WrenBrowser* browser);
  // Chromium is about to open one of its popovers; the app answers whose it
  // is. NULL leaves every popover to Chromium. Runs inside Chromium's call:
  // see WrenUI.h for what the app may do from it.
  WrenPopoverAnswer (*on_popover)(void* context,
                                  const WrenPopoverRequest* request);
  // Chromium wants `browser`'s window in full screen (`fullscreen`) or out
  // of it: a page asked (requestFullscreen, exitFullscreen), the user
  // pressed Escape, or Chromium's command ran. The app returns whether it
  // will; if it will, it starts the change (NSWindow's toggleFullScreen:)
  // and reports it with Wren_SetFullscreen. If it will not, or this is NULL,
  // the page leaves full screen at once. Runs inside Chromium's call, with
  // on_popover's rules (WrenUI.h): Wren_SetFullscreen is among the calls
  // refused until it returns.
  bool (*on_fullscreen)(void* context, WrenBrowser* browser, bool fullscreen);
  // An APP item of the main menu the app set (Wren_SetMainMenu, WrenMenus.h)
  // was chosen, by a click or its key. `id` is the item's, valid during the
  // call. `browser` is the Browser in the key window, as Chromium's
  // commands in the menu use, or NULL. Runs from the event loop.
  void (*on_menu_command)(void* context, const char* id, WrenBrowser* browser);
  // Downloads in progress would be cancelled: the app is quitting, or an
  // off-the-record Profile's last Browser is closing (WrenDownloads.h). The
  // app answers whose prompt it is, as for a popover: Chromium's (as
  // without a handler: Chrome's alert or dialog); the app's, which then asks
  // and answers with Wren_AnswerDownloadsPrompt, the quit or close waiting
  // until it does; or none, which cancels them without asking. Runs inside
  // Chromium's call, with on_popover's rules (WrenUI.h).
  WrenPopoverAnswer (*on_downloads_prompt)(void* context,
                                           const WrenDownloadsPrompt* prompt);
} WrenAppDelegate;

// --- Lifecycle and version --------------------------------------------------

typedef enum WrenExit {
  WREN_EXIT_NORMAL = 0,
  // The startup policy or delegate was invalid; Chromium never started. The
  // reason is printed to stderr.
  WREN_EXIT_INVALID_STARTUP = 1,
  // Another process holds user_data_dir. It was told about this launch and
  // opens nothing.
  WREN_EXIT_ALREADY_RUNNING = 2,
  // Chromium exited with an error.
  WREN_EXIT_CHROMIUM_ERROR = 3,
  // WrenStartup.api_version is not this framework's WREN_API_VERSION, or
  // unstable_ui_version not its WREN_UNSTABLE_UI_VERSION.
  WREN_EXIT_API_MISMATCH = 4,
} WrenExit;

// Runs Chromium's browser process in this process. Returns after quit, when
// every Profile and Browser handle reports closed. Handles the app still
// holds stay valid after it returns: they report closed, and may be released
// then (as an object that owns one does when it is destroyed) or not at all.
WREN_API WrenExit Wren_Main(const WrenStartup* startup,
                            const WrenAppDelegate* delegate);

// Requests quit. Pages may cancel it with beforeunload, and the user or the
// app with downloads in progress (on_downloads_prompt). Otherwise every
// Browser gets on_browser_closed, then on_will_quit runs, and Wren_Main
// returns WREN_EXIT_NORMAL.
WREN_API void Wren_Quit(void);

// The framework's WREN_API_VERSION. May be called at any time.
WREN_API uint32_t Wren_GetAPIVersion(void);

typedef struct WrenVersionInfo {
  const char* wren_version;      // "0.1.0" in a release, "0.1.0-dev" otherwise
  const char* chromium_version;  // for example "155.0.8059.12"
  const char* chromium_commit;
  uint32_t patch_revision;  // Wren's Chromium patch series
} WrenVersionInfo;

// Fills `info` with static strings. May be called at any time.
WREN_API void Wren_GetVersionInfo(WrenVersionInfo* info);

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
// Chromium's popovers as requests and data; it includes WrenUnstableUI.h,
// Chrome's sentences and the layout of its bubbles, which is unstable.
#include "WrenUI.h"
// Chromium's omnibox as data.
#include "WrenOmnibox.h"
// Chromium's session restore and closed tabs.
#include "WrenSession.h"
// The app's own URL schemes.
#include "WrenSchemes.h"
// Pages the app places that are not tabs.
#include "WrenWebView.h"
// Menus the app describes as data.
#include "WrenMenus.h"

#endif  // WREN_PUBLIC_WREN_H_
