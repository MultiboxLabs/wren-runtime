// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: Chromium's prompts and toasts as data (C API). Wren.h includes this
// header. Wren.h also explains threads, handles and strings.
//
// Chromium shows prompts and toasts by itself: a prompt to save a password,
// an address or a card, translate, a permission prompt, and a toast such as
// "Link copied". Before each, Wren asks the app's on_popover (WrenUI.h). When
// the app answers with its own popover, this header has what Chrome's would
// show and do. It also has the app's own toasts, which Chromium shows in its
// toast queue. Their sentences are in WrenUnstableUI.h.

#ifndef WREN_PUBLIC_WREN_PROMPTS_H_
#define WREN_PUBLIC_WREN_PROMPTS_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Prompt data ------------------------------------------------------------
//
// A prompt the app answered with its own popover carries what Chrome's prompt
// shows and does. That is WREN_POPOVER_PASSWORDS, _SAVE_ADDRESS, _SAVE_CARD,
// _TRANSLATE and _PERMISSION.
//
// The data lasts until the prompt closes. Every call then returns
// WREN_ERROR_POPOVER_CLOSED, or copies "". A call for another kind of request
// returns WREN_ERROR_INVALID_ARGUMENT. The prompts' texts are in
// WrenUnstableUI.h.

// Which of a request's strings Wren_CopyPopoverText copies. Each is for the
// kinds of request it names. Another kind copies "".
typedef enum WrenPopoverText {
  // For SAVE, UPDATE and AUTO_SIGNIN: the username of the credential the
  // prompt is about.
  WREN_POPOVER_TEXT_USERNAME = 0,
  // For SAVED: the username of the credential at `index`, or "" if it has
  // none.
  WREN_POPOVER_TEXT_SAVED_USERNAME = 1,
  // A revealed candidate's password at `index`, or an opened credential's
  // (Wren_RevealPasswords, Wren_ShowPasswordDetails). "" otherwise.
  WREN_POPOVER_TEXT_PASSWORD = 2,
  // For WREN_POPOVER_SAVE_CARD: the card's last four digits, such as "1111",
  // and the name on the card, as the form gave it. The name is "" if the form
  // gave none.
  WREN_POPOVER_TEXT_CARD_LAST_DIGITS = 3,
  WREN_POPOVER_TEXT_CARD_NAME = 4,
  // For WREN_POPOVER_TRANSLATE: the code of the language at `index`, from the
  // list of source languages or of target languages. Codes are such as "fr"
  // and "zh-TW", and "und" is the source list's unknown language.
  WREN_POPOVER_TEXT_TRANSLATE_SOURCE_CODE = 5,
  WREN_POPOVER_TEXT_TRANSLATE_TARGET_CODE = 6,
  // For WREN_POPOVER_PERMISSION: the Chromium name of the requested
  // permission at `index`, as Settings URLs use it, such as "geolocation" or
  // "media-stream-camera".
  WREN_POPOVER_TEXT_PERMISSION_ID = 7,
  // For WREN_POPOVER_TOAST: the toast's name, as in WrenPopoverRequest.toast.
  WREN_POPOVER_TEXT_TOAST_NAME = 8,
} WrenPopoverText;

// Copies one of the request's strings, in UTF-8. Returns the length needed,
// including the NUL. `index` picks an item of a list, and the kinds without
// one ignore it. An index past the end copies "", and so does a value outside
// WrenPopoverText.
WREN_API size_t Wren_CopyPopoverText(WrenPopoverID popover,
                                     size_t index,
                                     WrenPopoverText text,
                                     char* buffer,
                                     size_t size);

// Passwords, as Chrome's prompts have them: saving or updating one, the ones
// saved for the site, and signing in automatically.
//
// Passwords themselves reach the app only when Chromium's rule allows it, as
// Chrome reveals them. Right after a sign-in, Chrome shows the password at
// once. When the user reopened the prompt, it asks for Touch ID or the Mac
// password first. The app can read them until the request closes.
typedef enum WrenPasswordsPrompt {
  WREN_PASSWORDS_SAVE = 0,
  WREN_PASSWORDS_UPDATE = 1,
  // The passwords saved for the site, from the key icon or after saving.
  WREN_PASSWORDS_SAVED = 2,
  // Chromium signed in with a saved password. A short notice.
  WREN_PASSWORDS_AUTO_SIGNIN = 3,
} WrenPasswordsPrompt;

typedef struct WrenPasswordsState {
  WrenPasswordsPrompt prompt;
  // For SAVE and UPDATE: the number of passwords the form had, which Chrome
  // shows as a dropdown when there is more than one, and the one selected.
  // Then whether they are revealed, and whether revealing them asks for
  // authentication first.
  size_t candidate_count;
  size_t candidate;
  bool revealed;
  bool reveal_needs_authentication;
  // For SAVED: the number of saved credentials, as Chromium had them when the
  // request opened. A password saved a moment ago may still be on its way.
  size_t credential_count;
  // For SAVE and UPDATE: true if the password goes to the user's Google
  // Account, not to this device.
  bool account_store;
} WrenPasswordsState;

// Reads the prompt's state. Its texts come from Wren_CopyPopoverUIText
// (WrenUnstableUI.h).
WREN_API WrenResult Wren_GetPasswordsState(WrenPopoverID popover,
                                           WrenPasswordsState* state);
// Returns the length of a candidate's password, for SAVE and UPDATE, or of a
// saved credential's, for SAVED, so the app can draw its dots. 0 past the end.
WREN_API size_t Wren_GetPasswordLength(WrenPopoverID popover, size_t index);

// Runs later, from the event loop, with whether the passwords may be shown.
typedef void (*WrenPasswordsCallback)(void* context,
                                      WrenPopoverID popover,
                                      bool allowed);

// For SAVE and UPDATE: Chrome's eye button. Asks for authentication where
// Chromium does. Once allowed, WREN_POPOVER_TEXT_PASSWORD reads the
// candidates.
WREN_API WrenResult Wren_RevealPasswords(WrenPopoverID popover,
                                         WrenPasswordsCallback callback,
                                         void* context);
// For SAVED: opens a credential's details, after Touch ID or the Mac password,
// as Chrome does. Once allowed, WREN_POPOVER_TEXT_PASSWORD reads that
// credential, and Wren_UpdateSavedPassword changes it.
WREN_API WrenResult Wren_ShowPasswordDetails(WrenPopoverID popover,
                                             size_t index,
                                             WrenPasswordsCallback callback,
                                             void* context);
// For SAVE and UPDATE: saves, as Chrome's Save does. It saves with the
// username as edited, where NULL keeps it, and the password the user typed,
// or else the candidate at `candidate`. The request then closes.
WREN_API WrenResult Wren_SavePassword(WrenPopoverID popover,
                                      const char* username,
                                      const char* password,
                                      size_t candidate);
// For SAVED, after Wren_ShowPasswordDetails: changes the credential's
// username, password and note. NULL keeps each one.
WREN_API WrenResult Wren_UpdateSavedPassword(WrenPopoverID popover,
                                             size_t index,
                                             const char* username,
                                             const char* password,
                                             const char* note);

typedef enum WrenPasswordsAction {
  // For SAVE and UPDATE: Chrome's Not now, or No thanks. The request closes.
  WREN_PASSWORDS_NOT_NOW = 0,
  // For SAVE: never for this site. The request closes.
  WREN_PASSWORDS_NEVER = 1,
  // Opens Chromium's password manager, in a new tab.
  WREN_PASSWORDS_MANAGE = 2,
} WrenPasswordsAction;

// Does one of the prompt's actions.
WREN_API WrenResult Wren_PasswordsAction(WrenPopoverID popover,
                                         WrenPasswordsAction action);

// Saving or updating an address.
typedef struct WrenAddressState {
  bool is_update;
} WrenAddressState;

WREN_API WrenResult Wren_GetAddressState(WrenPopoverID popover,
                                         WrenAddressState* state);

typedef enum WrenAddressAnswer {
  WREN_ADDRESS_SAVE = 0,
  WREN_ADDRESS_NO_THANKS = 1,
  // Opens Chromium's address editor, a dialog. The request closes.
  WREN_ADDRESS_EDIT = 2,
} WrenAddressAnswer;

// Answers the prompt. Every answer closes the request.
WREN_API WrenResult Wren_AnswerAddressPrompt(WrenPopoverID popover,
                                             WrenAddressAnswer answer);

// Saving a card on this device. Wren never gives the app the card's number or
// security code.
typedef enum WrenCardNetwork {
  // A network Wren does not name, or none known.
  WREN_CARD_NETWORK_OTHER = 0,

  WREN_CARD_NETWORK_AMERICAN_EXPRESS = 1,
  WREN_CARD_NETWORK_DINERS = 2,
  WREN_CARD_NETWORK_DISCOVER = 3,
  WREN_CARD_NETWORK_ELO = 4,
  WREN_CARD_NETWORK_JCB = 5,
  WREN_CARD_NETWORK_MASTERCARD = 6,
  WREN_CARD_NETWORK_MIR = 7,
  WREN_CARD_NETWORK_TROY = 8,
  WREN_CARD_NETWORK_UNIONPAY = 9,
  WREN_CARD_NETWORK_VERVE = 10,
  WREN_CARD_NETWORK_VISA = 11,
} WrenCardNetwork;

typedef struct WrenCardState {
  WrenCardNetwork network;
  int expiration_month;  // 1 to 12, or 0 if missing
  int expiration_year;   // four digits, or 0 if missing
  // True if Chrome asks for the name, or for the expiration date, before
  // saving.
  bool needs_name;
  bool needs_expiration;
} WrenCardState;

WREN_API WrenResult Wren_GetCardState(WrenPopoverID popover,
                                      WrenCardState* state);
// Saves the card, with the name and expiration date the user gave when asked.
// Pass NULL and 0 when Chrome did not ask. The request closes.
WREN_API WrenResult Wren_SaveCard(WrenPopoverID popover,
                                  const char* name,
                                  int expiration_month,
                                  int expiration_year);
// Chrome's No thanks. The request closes.
WREN_API WrenResult Wren_DeclineCard(WrenPopoverID popover);

// Translating the page. It needs Chromium configured with a Google API key,
// and Wren adds none.
typedef enum WrenTranslateStep {
  WREN_TRANSLATE_BEFORE = 0,
  WREN_TRANSLATE_TRANSLATING = 1,
  WREN_TRANSLATE_AFTER = 2,
  WREN_TRANSLATE_ERROR = 3,
} WrenTranslateStep;

typedef struct WrenTranslateState {
  WrenTranslateStep step;
  // The page's language and the one to translate to, as indexes in their
  // lists, and the length of each list.
  size_t source;
  size_t target;
  size_t source_count;
  size_t target_count;

  bool always_translate;
  bool never_language;
  bool never_site;
  // True if the site can be put on the never list.
  bool can_never_site;
} WrenTranslateState;

WREN_API WrenResult Wren_GetTranslateState(WrenPopoverID popover,
                                           WrenTranslateState* state);
// Chooses the source and target languages, by their indexes in their lists.
WREN_API WrenResult Wren_SetTranslateLanguages(WrenPopoverID popover,
                                               size_t source,
                                               size_t target);

typedef enum WrenTranslateAction {
  WREN_TRANSLATE_PAGE = 0,
  WREN_TRANSLATE_REVERT = 1,
  // Not this time. The request closes.
  WREN_TRANSLATE_DECLINE = 2,
  // The next three turn a setting on or off, as `on` says.
  WREN_TRANSLATE_ALWAYS = 3,
  WREN_TRANSLATE_NEVER_LANGUAGE = 4,
  WREN_TRANSLATE_NEVER_SITE = 5,
} WrenTranslateAction;

// Does one of the prompt's actions.
WREN_API WrenResult Wren_TranslateAction(WrenPopoverID popover,
                                         WrenTranslateAction action,
                                         bool on);

// A permission request.
typedef enum WrenPermissionQuiet {
  // Chromium wants its full prompt.
  WREN_PERMISSION_NOT_QUIET = 0,
  // The other values mean Chromium wants a quiet prompt, an icon in the
  // address field. This one is because the user chose quieter messaging.
  WREN_PERMISSION_QUIET_BY_SETTING = 1,
  // Because the site is known for abusive requests or content.
  WREN_PERMISSION_QUIET_ABUSIVE = 2,
  // Because Chromium predicts the user will not allow it.
  WREN_PERMISSION_QUIET_PREDICTED = 3,
  // Because the page asked without a user gesture.
  WREN_PERMISSION_QUIET_NO_GESTURE = 4,
} WrenPermissionQuiet;

typedef struct WrenPermissionPromptState {
  // The number of permissions asked for together, such as the camera and the
  // microphone.
  size_t count;

  WrenPermissionQuiet quiet;
  // True if Chrome offers "Allow this time" for every one of them.
  bool can_allow_this_time;
} WrenPermissionPromptState;

WREN_API WrenResult
Wren_GetPermissionPromptState(WrenPopoverID popover,
                              WrenPermissionPromptState* state);

typedef enum WrenPermissionAnswer {
  WREN_PERMISSION_ANSWER_ALLOW = 0,
  WREN_PERMISSION_ANSWER_ALLOW_THIS_TIME = 1,
  WREN_PERMISSION_ANSWER_BLOCK = 2,
  // Closed without deciding, as with the close button of Chrome's prompt.
  WREN_PERMISSION_ANSWER_DISMISS = 3,
} WrenPermissionAnswer;

// Answers the request, which then closes.
//
// Chrome ignores a click on its prompt that comes too soon after the prompt
// appeared, within the system's double-click interval. So an Allow that soon
// after the request returns WREN_ERROR_NOT_ALLOWED.
WREN_API WrenResult Wren_AnswerPermissionPrompt(WrenPopoverID popover,
                                                WrenPermissionAnswer answer);
// For a quiet request: Chromium asks again for its full prompt, as a new
// request. This request closes.
WREN_API WrenResult Wren_ShowFullPermissionPrompt(WrenPopoverID popover);

// --- Toasts -----------------------------------------------------------------
//
// A toast the app answered with its own (WREN_POPOVER_TOAST). Its text and its
// button's text are Chrome's (Wren_CopyPopoverUIText, WrenUnstableUI.h). Its
// close button calls Wren_ClosePopover.
//
// As with the prompts above, every call returns WREN_ERROR_POPOVER_CLOSED
// once the toast has closed, and WREN_ERROR_INVALID_ARGUMENT for another kind
// of request.

typedef struct WrenToastState {
  // Whether Chrome's toast has an action button, which Wren_PressToastButton
  // presses, a close button, and a menu (Wren_GetToastMenuItemCount).
  bool has_button;
  bool has_close_button;
  bool has_menu;
  // True if a spinner takes the place of its icon, because something is
  // under way.
  bool has_throbber;
  // True if the toast is about the Browser, not its active tab. It then stays
  // across tab switches and navigations.
  bool global;
} WrenToastState;

WREN_API WrenResult Wren_GetToastState(WrenPopoverID popover,
                                       WrenToastState* state);
// Copies the toast's icon as a PNG, `points` square at `scale`. `points` is 1
// to 1024 and `scale` is at most 4. The icon is in Chromium's colour for the
// window's appearance.
//
// Returns the PNG's size, and copies it only if `size` is large enough.
// Returns 0 when there is no icon, as with a spinner.
WREN_API size_t Wren_CopyToastIconPNG(WrenPopoverID popover,
                                      int points,
                                      double scale,
                                      uint8_t* buffer,
                                      size_t size);
// Presses Chrome's action button. It does what it says, and the request
// closes. Returns WREN_ERROR_NOT_ALLOWED for a toast without one.
WREN_API WrenResult Wren_PressToastButton(WrenPopoverID popover);

// One item of the toast's menu, as Chrome's menu shows it. The labels are in
// WrenUnstableUI.h.
typedef struct WrenToastMenuItem {
  bool separator;
  bool enabled;
  bool checked;
} WrenToastMenuItem;

// The number of items in the toast's menu. 0 without a menu.
WREN_API size_t Wren_GetToastMenuItemCount(WrenPopoverID popover);
WREN_API WrenResult Wren_GetToastMenuItem(WrenPopoverID popover,
                                          size_t index,
                                          WrenToastMenuItem* item);
// Chooses the item, as in Chrome's menu, and the request closes. Returns
// WREN_ERROR_NOT_ALLOWED for a separator or a disabled item.
WREN_API WrenResult Wren_ChooseToastMenuItem(WrenPopoverID popover,
                                             size_t index);

// --- The app's own toasts ---------------------------------------------------
//
// A toast of the app's, such as "Address copied", shown as Chromium shows its
// own. It goes through the Browser's toast queue. So it replaces the toast
// showing, and goes after about 4 seconds, when the tab switches or
// navigates, or when the next toast comes.
//
// Before it shows, Wren asks on_popover as for Chromium's toasts, as
// WREN_POPOVER_TOAST with the toast the app named, or "Embedder". The answer
// may be Chromium's, the app's own, or none. The app's own toast reads its text
// and icon as any toast does.

typedef struct WrenToastOptions {
  // Required, and not empty. UTF-8.
  const char* text;
  // Optional. A square PNG, drawn as given in place of Chromium's info icon,
  // which is drawn in the toast's text colour. Any size works, but 32 pixels
  // across or more looks sharp on a Retina display.
  const void* icon_png;
  size_t icon_png_size;
  // Optional. The toast's name, in UTF-8, in place of "Embedder", which the
  // app reads back as WrenPopoverRequest.toast and
  // WREN_POPOVER_TEXT_TOAST_NAME. NULL or empty gives "Embedder". Wren takes
  // any name, so give names Chromium's own toasts do not use, such as
  // "AddressCopied".
  const char* name;
} WrenToastOptions;

// Shows one of the app's own toasts in the Browser.
//
// Returns WREN_ERROR_CLOSED for a closed Browser, and
// WREN_ERROR_INVALID_ARGUMENT for no text or an icon that is not a square PNG.
// Returns WREN_ERROR_NOT_ALLOWED from on_popover, and when the user's
// settings, as Chrome's, show only toasts with a button.
WREN_API WrenResult Wren_ShowToast(WrenBrowser* browser,
                                   const WrenToastOptions* options);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_PROMPTS_H_
