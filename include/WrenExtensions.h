// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: extensions (C API). Wren.h includes this header. Wren.h also explains
// threads, handles and strings.
//
// This header has three parts. Extension actions are the buttons extensions
// add to a toolbar. Installed extensions are the list Chrome's extensions page
// shows. App extensions are the ones the app ships, which Wren installs.

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
// The extensions installed in the Browser's Profile, whether turned on or off.
// The list is the one Chrome's extensions page shows. It leaves out component
// extensions, apps and themes. An extension that is turned off has no action.
//
// Calls name an extension by its ID. An ID that is not there gives
// WREN_ERROR_INVALID_ARGUMENT. WREN_CHANGE_EXTENSION_ACTIONS reports changes
// to this list too.

// The number of installed extensions. They are in order of name.
WREN_API size_t Wren_GetExtensionCount(WrenBrowser* browser);
// Copies the extension ID at `index`, or "" if there is none. The ID is
// UTF-8. Returns the length needed, including the NUL.
WREN_API size_t Wren_CopyExtensionID(WrenBrowser* browser,
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
} WrenExtensionState;

WREN_API WrenResult Wren_GetExtensionState(WrenBrowser* browser,
                                           const char* extension_id,
                                           WrenExtensionState* state);
// Copies the extension's name, in UTF-8. Returns the length needed, including
// the NUL.
WREN_API size_t Wren_CopyExtensionName(WrenBrowser* browser,
                                       const char* extension_id,
                                       char* buffer,
                                       size_t size);
// Copies the extension's icon as a PNG, `points` square at `scale`. `points`
// is 1 to 1024 and `scale` is at most 4. The icon is the one in the
// extension's manifest, or Chrome's default extension icon. It is never
// greyed.
//
// Returns the PNG's size, and copies it only if `size` is large enough.
// Returns 0 until the icon has loaded. WREN_CHANGE_EXTENSION_ACTIONS then
// reports it.
WREN_API size_t Wren_CopyExtensionIconPNG(WrenBrowser* browser,
                                          const char* extension_id,
                                          int points,
                                          double scale,
                                          uint8_t* buffer,
                                          size_t size);
// Turns the extension on or off, as the switch on Chrome's extensions page
// does. Returns WREN_ERROR_NOT_ALLOWED unless its state's can_change is true.
WREN_API WrenResult Wren_SetExtensionEnabled(WrenBrowser* browser,
                                             const char* extension_id,
                                             bool enabled);

// --- App extensions ---------------------------------------------------------
//
// The app lists the extensions it ships in WrenStartup.extensions, at every
// launch. No launch inherits the list of the one before. There are no calls
// here. The list and on_profile_extensions are in Wren.h.
//
// As each persistent or ephemeral Profile loads, Wren asks the app which of
// them the Profile gets (on_profile_extensions). Then, in that Profile:
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
// WREN_CHANGE_EXTENSION_ACTIONS reports each change. Chromium logs a .crx it
// refuses to install, such as one whose manifest it rejects, and Wren leaves
// that extension out.
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

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_EXTENSIONS_H_
