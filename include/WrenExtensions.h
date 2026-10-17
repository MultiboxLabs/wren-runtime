// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: extensions (C API). Wren.h includes this header; see Wren.h for
// threading, handles and strings.
//
// Extensions: their actions in the app's toolbar, and the Profile's
// installed extensions.

#ifndef WREN_PUBLIC_WREN_EXTENSIONS_H_
#define WREN_PUBLIC_WREN_EXTENSIONS_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Extension actions ------------------------------------------------------
//
// The buttons extensions add to Chrome's toolbar (chrome.action), for the
// app's toolbar. A normal Browser has them, for its active tab; other
// Browsers have none, as Chrome shows no toolbar there
// (WREN_ERROR_NOT_ALLOWED). Actions are named by extension ID; one that is
// not there is WREN_ERROR_INVALID_ARGUMENT. Popups, menus and dialogs are
// Chromium's, at the app's views named with Wren_SetAnchor.

// The actions in Chrome's order: pinned ones in the user's order, then the
// others by name. 0 until Chromium has loaded the Profile's extensions.
WREN_API size_t Wren_GetExtensionActionCount(WrenBrowser* browser);
// The extension ID of the action at `index`, or "". Copies a UTF-8 string
// and returns the length needed, including the NUL.
WREN_API size_t Wren_CopyExtensionActionID(WrenBrowser* browser,
                                           size_t index,
                                           char* buffer,
                                           size_t size);

typedef struct WrenExtensionActionState {
  // For the active tab. Clicking a disabled action opens its context menu.
  bool enabled;
  // Shown in the toolbar: the Profile's setting, as in Chrome.
  bool pinned;
  // Pinned by policy; it cannot be unpinned.
  bool force_pinned;
  // Its popup is open.
  bool popup_open;
  // Chromium wants its button shown although it is not pinned, while its
  // popup opens, as Chrome pops the button out next to the extensions
  // button. The popup opens after on_changed has reported this, below the
  // action's anchor if the app names one by then, else the extensions
  // button's.
  bool popped_out;
  // The badge's colours, 0xAARRGGBB.
  uint32_t badge_text_argb;
  uint32_t badge_background_argb;
} WrenExtensionActionState;

WREN_API WrenResult
Wren_GetExtensionActionState(WrenBrowser* browser,
                             const char* extension_id,
                             WrenExtensionActionState* state);
// The extension's name, its tooltip (the action's title and what it may do
// on the page) and its badge text, or "". UTF-8; each returns the length
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
// The action's icon as PNG, `points` square (1 to 1024) at `scale` (up to
// 4): with `badge`, Chrome's 28-point button face scaled to `points` (the
// 16-point icon centred, greyed when disabled, with its badge and, once the
// extension was blocked on the page, the circle behind it); without, the
// extension's icon alone. Returns the PNG's size, copying it only if `size`
// is enough; 0 if there is no icon.
WREN_API size_t Wren_CopyExtensionActionIconPNG(WrenBrowser* browser,
                                                const char* extension_id,
                                                int points,
                                                double scale,
                                                bool badge,
                                                uint8_t* buffer,
                                                size_t size);

// As a click on Chrome's button: the action's popup (below its anchor),
// chrome.action.onClicked, its side panel, or, when disabled, its context
// menu. A click while its popup is open closes it.
WREN_API WrenResult Wren_ClickExtensionAction(WrenBrowser* browser,
                                              const char* extension_id);
// Chromium's context menu for the action (pin, options, remove, site
// access), at its anchor. Runs the menu before returning.
WREN_API WrenResult Wren_ShowExtensionActionMenu(WrenBrowser* browser,
                                                 const char* extension_id);
// Pins or unpins the action for the Profile, as Chrome's menu does.
// WREN_ERROR_NOT_ALLOWED in an off-the-record Browser and for a pin set by
// policy.
WREN_API WrenResult Wren_SetExtensionActionPinned(WrenBrowser* browser,
                                                  const char* extension_id,
                                                  bool pinned);
// Moves a pinned action to `index` among the pinned ones.
// WREN_ERROR_NOT_ALLOWED for an action that is not pinned, as for pinning.
WREN_API WrenResult Wren_MoveExtensionAction(WrenBrowser* browser,
                                             const char* extension_id,
                                             size_t index);
// Opens Chromium's extensions menu (every action, pinning, site access)
// below the "extensions" anchor, or closes it if it is open. Asks on_popover
// before opening it (WREN_POPOVER_EXTENSIONS_MENU).
WREN_API WrenResult Wren_ShowExtensionsMenu(WrenBrowser* browser);

// --- Installed extensions ---------------------------------------------------
//
// The extensions installed in the Browser's Profile, turned on or off, as
// Chrome's extensions page lists them (no component extensions, apps or
// themes). One that is turned off has no action. Named by extension ID; one
// that is not there is WREN_ERROR_INVALID_ARGUMENT.
// WREN_CHANGE_EXTENSION_ACTIONS reports their changes.

// The extensions by name.
WREN_API size_t Wren_GetExtensionCount(WrenBrowser* browser);
// The extension ID at `index`, or "". UTF-8; returns the length needed,
// including the NUL.
WREN_API size_t Wren_CopyExtensionID(WrenBrowser* browser,
                                     size_t index,
                                     char* buffer,
                                     size_t size);

typedef struct WrenExtensionState {
  // Turned on (it may have crashed since).
  bool enabled;
  // The user may turn it on or off here, as with the switch on Chrome's
  // extensions page: policy allows it, and one that is off was turned off
  // by the user (otherwise Chrome's extensions page asks first, for new
  // permissions, or repairs it).
  bool can_change;
} WrenExtensionState;

WREN_API WrenResult Wren_GetExtensionState(WrenBrowser* browser,
                                           const char* extension_id,
                                           WrenExtensionState* state);
// Its name. UTF-8; returns the length needed, including the NUL.
WREN_API size_t Wren_CopyExtensionName(WrenBrowser* browser,
                                       const char* extension_id,
                                       char* buffer,
                                       size_t size);
// Its icon from its manifest as PNG, or Chrome's default extension icon,
// `points` square (1 to 1024) at `scale` (up to 4), not greyed. Returns the
// PNG's size, copying it only if `size` is enough; 0 until it has loaded (then
// WREN_CHANGE_EXTENSION_ACTIONS).
WREN_API size_t Wren_CopyExtensionIconPNG(WrenBrowser* browser,
                                          const char* extension_id,
                                          int points,
                                          double scale,
                                          uint8_t* buffer,
                                          size_t size);
// Turns it on or off, as the switch on Chrome's extensions page.
// WREN_ERROR_NOT_ALLOWED unless its state's can_change.
WREN_API WrenResult Wren_SetExtensionEnabled(WrenBrowser* browser,
                                             const char* extension_id,
                                             bool enabled);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_EXTENSIONS_H_
