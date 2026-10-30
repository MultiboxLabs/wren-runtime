// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: extensions (C API). Wren.h includes this header. Wren.h also explains
// threads, handles and strings.
//
// This header has four parts. Extension actions are the buttons extensions
// add to a toolbar. Installed extensions are the list Chrome's extensions page
// shows. App extensions are the ones the app ships, which Wren installs.
// Native messaging is how an extension talks to the app.

#ifndef WREN_PUBLIC_WREN_EXTENSIONS_H_
#define WREN_PUBLIC_WREN_EXTENSIONS_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Extension actions ------------------------------------------------------
//
// An action is the button an extension adds to Chrome's toolbar through
// chrome.action. The app draws these buttons in its own toolbar.
//
// Only a normal Browser has actions, and they act on its active tab. Other
// Browsers have none, because Chrome shows no toolbar there. Calls on them
// return WREN_ERROR_NOT_ALLOWED.
//
// Calls name an action by its extension ID. An ID that is not there gives
// WREN_ERROR_INVALID_ARGUMENT.
//
// An action's popups, menus and dialogs are Chromium's. They hang off the
// views the app names with Wren_SetAnchor.

// The number of actions. Chrome's order is the pinned actions in the user's
// order, then the others by name. Returns 0 until Chromium has loaded the
// Profile's extensions.
WREN_API size_t Wren_GetExtensionActionCount(WrenBrowser* browser);
// Copies the extension ID of the action at `index`, or "" if there is none.
// The ID is UTF-8. Returns the length needed, including the NUL.
WREN_API size_t Wren_CopyExtensionActionID(WrenBrowser* browser,
                                           size_t index,
                                           char* buffer,
                                           size_t size);

typedef struct WrenExtensionActionState {
  // True if the action is enabled for the active tab. Clicking a disabled
  // action opens its context menu.
  bool enabled;
  // True if the action is shown in the toolbar. This is the Profile's
  // setting, as in Chrome.
  bool pinned;
  // True if policy pinned the action. The user cannot unpin it.
  bool force_pinned;
  // True while the action's popup is open.
  bool popup_open;
  // True while Chromium wants the button shown even though the action is not
  // pinned. Chrome does this when the popup opens, popping the button out next
  // to the extensions button.
  //
  // The popup opens after on_changed has reported this field. It hangs off
  // the "extension:<id>" anchor if the app has named one by then, and off
  // "extensions" otherwise.
  //
  // This field is only for the popup. Chromium does not pop the button out
  // for the action's dialogs, such as the one that asks before removing the
  // extension. Those hang off "extension:<id>" if the app has placed it, and
  // off "extensions" if not.
  bool popped_out;
  // The colours of the badge's text and background, as 0xAARRGGBB.
  uint32_t badge_text_argb;
  uint32_t badge_background_argb;
} WrenExtensionActionState;

WREN_API WrenResult
Wren_GetExtensionActionState(WrenBrowser* browser,
                             const char* extension_id,
                             WrenExtensionActionState* state);
// The next three calls copy the extension's name, its tooltip and its badge
// text, or "". The tooltip is the action's title and what the extension may
// do on the page. Each string is UTF-8, and each call returns the length
// needed, including the NUL.
WREN_API size_t Wren_CopyExtensionActionName(WrenBrowser* browser,
                                             const char* extension_id,
                                             char* buffer,
                                             size_t size);
WREN_API size_t Wren_CopyExtensionActionTooltip(WrenBrowser* browser,
                                                const char* extension_id,
                                                char* buffer,
                                                size_t size);
WREN_API size_t Wren_CopyExtensionActionBadge(WrenBrowser* browser,
                                              const char* extension_id,
                                              char* buffer,
                                              size_t size);
// Copies the action's icon as a PNG, `points` square at `scale`. `points` is
// 1 to 1024 and `scale` is at most 4.
//
// With `badge`, the PNG is Chrome's whole 28-point button face, scaled to
// `points`. That is the 16-point icon in the middle, greyed when the action is
// disabled, with its badge. Once the extension has been blocked on the page,
// it also has the circle Chrome draws behind the icon. Without `badge`, the
// PNG is the extension's icon alone.
//
// Returns the PNG's size, and copies it only if `size` is large enough.
// Returns 0 if there is no icon.
WREN_API size_t Wren_CopyExtensionActionIconPNG(WrenBrowser* browser,
                                                const char* extension_id,
                                                int points,
                                                double scale,
                                                bool badge,
                                                uint8_t* buffer,
                                                size_t size);

// Does what a click on Chrome's button does. That opens the action's popup
// below its anchor, fires chrome.action.onClicked, or opens its side panel. A
// disabled action opens its context menu instead. A click while the popup is
// open closes the popup.
WREN_API WrenResult Wren_ClickExtensionAction(WrenBrowser* browser,
                                              const char* extension_id);
// Opens Chromium's context menu for the action at its anchor. The menu has
// pin, options, remove and site access. The menu runs before the call
// returns.
WREN_API WrenResult Wren_ShowExtensionActionMenu(WrenBrowser* browser,
                                                 const char* extension_id);
// Pins or unpins the action for the Profile, as Chrome's menu does. Returns
// WREN_ERROR_NOT_ALLOWED in an off-the-record Browser, and for a pin set by
// policy.
WREN_API WrenResult Wren_SetExtensionActionPinned(WrenBrowser* browser,
                                                  const char* extension_id,
                                                  bool pinned);
// Moves a pinned action to `index` among the pinned actions. Returns
// WREN_ERROR_NOT_ALLOWED for an action that is not pinned. As with pinning,
// it also returns WREN_ERROR_NOT_ALLOWED in an off-the-record Browser and for
// a pin set by policy.
WREN_API WrenResult Wren_MoveExtensionAction(WrenBrowser* browser,
                                             const char* extension_id,
                                             size_t index);
// Opens Chromium's extensions menu below the "extensions" anchor, or closes it
// if it is open. The menu lists every action, with pinning and site access.
// Wren asks on_popover before opening it, as WREN_POPOVER_EXTENSIONS_MENU.
WREN_API WrenResult Wren_ShowExtensionsMenu(WrenBrowser* browser);

// --- Installed extensions ---------------------------------------------------
//
// The extensions installed in a Profile, whether turned on or off, with or
// without a window. The list is the one Chrome's extensions page shows. It
// leaves out component extensions, apps and themes. An extension that is
// turned off has no action. An off-the-record Profile gives its original's,
// as in Chrome.
//
// Calls name an extension by its ID. An ID that is not there gives
// WREN_ERROR_INVALID_ARGUMENT. Until Chromium has loaded the Profile's
// extensions, the list is empty. Once the app has read it,
// WREN_CHANGE_EXTENSIONS reports each change, including each extension as it
// loads. A Profile that has closed gives an empty list and
// WREN_ERROR_CLOSED.

// The number of installed extensions. They are in order of name.
WREN_API size_t Wren_GetExtensionCount(WrenProfile* profile);
// Copies the extension ID at `index`, or "" if there is none. The ID is
// UTF-8. Returns the length needed, including the NUL.
WREN_API size_t Wren_CopyExtensionID(WrenProfile* profile,
                                     size_t index,
                                     char* buffer,
                                     size_t size);

typedef struct WrenExtensionState {
  // True if the extension is turned on. It may have crashed since.
  bool enabled;
  // True if the user may turn the extension on or off here, as with the
  // switch on Chrome's extensions page. That needs policy to allow it. An
  // extension that is off must also have been turned off by the user. For
  // any other extension that is off, Chrome's extensions page asks first, for
  // new permissions, or repairs it.
  bool can_change;
  // True if the extension also runs in the Profile's off-the-record
  // Profiles, as with Allow in Incognito on Chrome's extensions page.
  bool allowed_in_incognito;
  // True if the user may allow it in incognito or not, here or with that
  // switch. Chrome's extensions page shows the switch only then: the
  // extension's manifest does not refuse incognito, and the Profile may go
  // off the record. Policy that keeps the user from turning the extension
  // off does not lock this switch.
  bool can_allow_in_incognito;
} WrenExtensionState;

WREN_API WrenResult Wren_GetExtensionState(WrenProfile* profile,
                                           const char* extension_id,
                                           WrenExtensionState* state);
// Copies the extension's name, in UTF-8. Returns the length needed, including
// the NUL.
WREN_API size_t Wren_CopyExtensionName(WrenProfile* profile,
                                       const char* extension_id,
                                       char* buffer,
                                       size_t size);
// Copies the extension's icon as a PNG, `points` square at `scale`. `points`
// is 1 to 1024 and `scale` is at most 4. The icon is the one in the
// extension's manifest, or Chrome's default extension icon. It is never
// greyed.
//
// Returns the PNG's size, and copies it only if `size` is large enough.
// Returns 0 until the icon has loaded. WREN_CHANGE_EXTENSIONS then reports
// it.
WREN_API size_t Wren_CopyExtensionIconPNG(WrenProfile* profile,
                                          const char* extension_id,
                                          int points,
                                          double scale,
                                          uint8_t* buffer,
                                          size_t size);
// Turns the extension on or off, as the switch on Chrome's extensions page
// does. Returns WREN_ERROR_NOT_ALLOWED unless its state's can_change is true.
WREN_API WrenResult Wren_SetExtensionEnabled(WrenProfile* profile,
                                             const char* extension_id,
                                             bool enabled);
// Allows the extension in incognito or not, as Allow in Incognito on Chrome's
// extensions page does. Chromium reloads an extension that is on, as there,
// so its pages and its worker start again. Returns WREN_ERROR_NOT_ALLOWED
// unless its state's can_allow_in_incognito is true.
WREN_API WrenResult
Wren_SetExtensionAllowedInIncognito(WrenProfile* profile,
                                    const char* extension_id,
                                    bool allowed);

// --- Installing from the Chrome Web Store ----------------------------------
//
// For an app that brings the user over from another browser. The app reads
// that browser's extensions itself, and asks the user which to install.

typedef enum WrenWebStoreInstallResult {
  WREN_WEB_STORE_INSTALLED = 0,
  // The Profile already had it. Nothing changed.
  WREN_WEB_STORE_ALREADY_INSTALLED = 1,
  // The store has no extension with the ID.
  WREN_WEB_STORE_NOT_FOUND = 2,
  // Policy or Chrome's blocklist refuses it.
  WREN_WEB_STORE_NOT_ALLOWED = 3,
  // The download or the install failed. The store may be out of reach.
  WREN_WEB_STORE_FAILED = 4,
} WrenWebStoreInstallResult;

// Receives the result of Wren_InstallFromWebStore. With WREN_OK, `results`
// has one result for each ID, in order, valid during the call.
typedef void (*WrenInstallFromWebStoreCallback)(
    void* context,
    WrenResult result,
    const WrenWebStoreInstallResult* results,
    size_t count);

// Installs the Chrome Web Store extensions with `extension_ids`, as the user's
// own installs, the way Chrome installs a user's synced extensions on a new
// Mac. Ask the user first: Chromium shows no dialog. Each comes in on, with
// the permissions it asks for. The user may turn it off or remove it.
// WREN_CHANGE_EXTENSIONS reports each install. `callback` gets one result for
// each ID, in order, once all are done, from the event loop. An ID given
// twice gets the same result in both places. An off-the-record Profile
// installs into its original. `callback` is not called if the app quits
// first.
//
// `callback` gets one of these errors:
//
// - WREN_ERROR_INVALID_ARGUMENT for no Profile, NULL `extension_ids` with a
//   count, or an ID that is not 32 letters from a to p. Nothing is installed.
// - WREN_ERROR_CLOSED once the Profile has closed. If it closes before all
//   are done, `callback` gets it too, and what was installed stays.
// - WREN_ERROR_NOT_ALLOWED inside on_popover. Nothing is installed.
WREN_API void Wren_InstallFromWebStore(WrenProfile* profile,
                                       const char* const* extension_ids,
                                       size_t count,
                                       WrenInstallFromWebStoreCallback callback,
                                       void* context);

// --- App extensions ---------------------------------------------------------
//
// The app lists the extensions it ships in WrenStartup.extensions, at every
// launch. No launch inherits the list of the one before. There are no calls
// here. The list and on_profile_extensions are in Wren.h.
//
// As each persistent or ephemeral Profile loads, Wren asks the app which of
// them the Profile gets, and how each starts there (on_profile_extensions,
// with the request below). Then, in that Profile:
//
// - Wren installs each extension the Profile gets that it does not have, or
//   has at a lower version. An update keeps the extension's data.
// - An extension the Profile has at the same or a higher version stays as it
//   is.
// - Wren uninstalls each extension it installed in an earlier launch that the
//   Profile no longer gets. Its data goes with it.
//
// Wren installs them as Chromium installs a .crx the user opens. They are
// among the installed extensions above, with their actions. Wren does this in
// the background once Chromium has loaded the Profile's extensions, and
// WREN_CHANGE_EXTENSIONS reports each change. Chromium logs a .crx it refuses
// to install, such as one whose manifest it rejects, and Wren leaves that
// extension out.
//
// Neither the user nor the extension can remove an app extension.
// Chrome's extensions page greys out Remove, and
// chrome.management.uninstallSelf fails. Only the app removes one, by no
// longer giving it to the Profile. The user may still turn it off, as with any
// extension, and the Profile keeps that choice across launches.
//
// Chrome's extensions page says each one is "Managed by" the app, by its
// product_name, with its product_icon_png as the badge. Safety Check does not
// ask the user to remove one for not coming from the Chrome Web Store. Its
// other warnings, such as for malware, still apply.
//
// An extension the user installed with the same ID becomes the app's. Chromium
// updates an app extension as it does any other, from the update_url in its
// manifest, or from the Chrome Web Store without one.
//
// Those are bundled extensions. An internal extension is a component
// extension, as Chrome's own PDF viewer is. Wren adds it, as each Profile
// loads, to the Profiles that get it, and Chromium loads it from its folder
// each launch. It is never installed, so Wren removes nothing: a launch that
// does not give it to a Profile does not load it there. Its data stays in the
// Profile.
//
// A component extension has more rights than any other:
//
// - Chrome's extensions page and Wren_GetExtensionCount leave it out. Its
//   action and its side panel work as any extension's do.
// - Neither the user nor the app can turn it off, remove it, or limit the
//   sites it may read. It runs in off-the-record Profiles too.
// - Every extension API's checks of allowlists, locations and manifest
//   versions pass for it. So it may use Chromium's private APIs, such as
//   settingsPrivate, activityLogPrivate, metricsPrivate, feedbackPrivate,
//   resourcesPrivate and safeBrowsingPrivate, and dns, processes,
//   audioCapture, videoCapture, hid and usbDevices. It may let every page
//   message it, through externally_connectable.
// - It may script pages other extensions may not, and use chrome://resources
//   and chrome://theme.
//
// So ship an internal extension only for code the app itself trusts.
// WrenStartup.manifest_v2 says which Manifest V2 extensions run. A component
// extension always does.

// What one Profile gets of one of WrenStartup.extensions.
typedef struct WrenProfileExtension {
  // Whether the Profile gets the extension. Starts true.
  bool gets;
  // Whether it is on, and whether it is allowed in incognito, if Wren
  // installs it in this Profile in this launch. They start as the
  // extension's enabled_on_install and allowed_in_incognito_on_install. Once
  // it is installed, the Profile keeps what the user or the app chose, and
  // these do nothing, at an update too. An internal extension ignores them.
  bool enabled;
  bool allowed_in_incognito;
} WrenProfileExtension;

// on_profile_extensions' request (Wren.h).
struct WrenProfileExtensionsRequest {
  // The Profile's ID, as Wren_CopyProfileID gives it.
  const char* profile_id;
  // WREN_PROFILE_PERSISTENT or WREN_PROFILE_EPHEMERAL.
  WrenProfileKind kind;
  // One entry for each of WrenStartup.extensions, in its order.
  WrenProfileExtension* extensions;
  size_t count;
};

// --- Native messaging -------------------------------------------------------
//
// An extension written for Chrome talks to a program on the Mac through
// chrome.runtime.connectNative and sendNativeMessage, which name a native
// messaging host. In Chrome the host is a process Chromium starts from a
// manifest on disk. A host in WrenStartup.native_hosts is the app instead: no
// process starts, and the app hears the port from the event loop.
//
// - A name the app declares hides a host on disk with the same name. Any other
//   name goes to the hosts on disk, as in Chromium. Chromium reads a user's
//   hosts from the user data folder's NativeMessagingHosts, so hosts installed
//   for Chrome are not found.
// - The extension needs the nativeMessaging permission. An extension not in
//   the host's extension_ids gets "Access to the specified native messaging
//   host is forbidden.", and with no handler set every extension gets
//   "Specified native messaging host not found.".
// - The NativeMessagingBlocklist and NativeMessagingAllowlist policies apply
//   to the app's hosts as to any. NativeMessagingUserLevelHosts does not.
// - sendNativeMessage is a port too, which the first answer closes.
// - A message from the extension may be up to 64 MiB. One from the app may be
//   up to 1 MiB, as for any host.
// - A message comes from an extension. Treat it as input, as a scheme's
//   handler treats a URL.

// One port an extension opened to one of the app's hosts. The pointer is
// valid from WREN_NATIVE_PORT_OPENED until the WREN_NATIVE_PORT_CLOSED call
// returns. The port is over once the app closes it or the extension does.
// Using it then is safe: the calls below return WREN_ERROR_CLOSED, NULL or 0.
// A port still open when the app quits ends with the event loop, with no
// WREN_NATIVE_PORT_CLOSED.
typedef struct WrenNativePort WrenNativePort;

typedef enum WrenNativePortEvent {
  // The extension called connectNative or sendNativeMessage.
  WREN_NATIVE_PORT_OPENED = 0,
  // `json` is the extension's message, as JSON text.
  WREN_NATIVE_PORT_MESSAGE = 1,
  // The port's last event, whichever side closed it.
  WREN_NATIVE_PORT_CLOSED = 2,
} WrenNativePortEvent;

// Runs on the main thread, from the event loop, outside Chromium's
// notifications, so it may call any function. `json` is NULL but for
// WREN_NATIVE_PORT_MESSAGE, and valid during the call.
typedef void (*WrenNativeHostHandler)(void* context,
                                      WrenNativePort* port,
                                      WrenNativePortEvent event,
                                      const char* json);

// Sets the handler of a host in WrenStartup.native_hosts, replacing any
// handler set before. NULL removes the handler. A port opened without one
// fails as a missing host does. Returns WREN_ERROR_INVALID_ARGUMENT for a
// name that was not declared.
WREN_API WrenResult Wren_SetNativeHostHandler(const char* name,
                                              WrenNativeHostHandler handler,
                                              void* context);

// The extension's Profile, never an off-the-record one: an extension in an
// off-the-record Profile reports its original. Borrowed. NULL once the port
// is over.
WREN_API WrenProfile* Wren_GetNativePortProfile(WrenNativePort* port);

// Copies the extension's ID. Returns the size needed, including the NUL, and
// copies what fits. Returns 0 once the port is over.
WREN_API size_t Wren_CopyNativePortExtensionID(WrenNativePort* port,
                                               char* buffer,
                                               size_t size);

// Sends the extension a message, the value of `json`. Returns
// WREN_ERROR_INVALID_ARGUMENT for NULL, or for text that is not JSON or is
// over 1 MiB. Returns WREN_ERROR_CLOSED once the port is over.
WREN_API WrenResult Wren_SendNativePortMessage(WrenNativePort* port,
                                               const char* json);

// Closes the port. The extension sees "Native host has exited.", as when a
// host's process ends, and WREN_NATIVE_PORT_CLOSED follows from the event
// loop. Returns WREN_ERROR_CLOSED once the port is over.
WREN_API WrenResult Wren_CloseNativePort(WrenNativePort* port);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_EXTENSIONS_H_
