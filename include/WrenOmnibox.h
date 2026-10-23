// Copyright 2026 Wren Authors
// Use of this source code requires permission from Multibox Labs; see the
// LICENSE file.

// Wren: Chromium's omnibox as data (C API). Wren.h includes this header.
// Wren.h also explains threads, handles and strings.
//
// The omnibox is Chrome's address field and the list of suggestions under it.
// A Wren app draws its own field and its own list, and Chromium draws none of
// it. Wren gives the app what Chrome's would show:
//
// - The location state. That is the address to show while the user is not
//   editing the field. It comes from Chromium's location bar model, trimmed
//   as Chrome trims it.
// - Suggestions. These are Chromium's autocomplete results for the text the
//   app queries, as Chrome's list shows them. They come from history,
//   bookmarks, open tabs, search engines and extensions' keywords. Wren leaves
//   out suggestions that open Chrome's own UI instead of a page, such as "Ask
//   Google about this page", Lens and AI Mode, because a Wren browser does not
//   have that UI.
// - Opening and deleting a suggestion as Chrome does. Chromium's history,
//   shortcuts and search engines learn from it as they do in Chrome.
//
// The app handles typing itself, as Chrome's field does:
//
// - It queries after each edit.
// - It shows the default suggestion's inline completion after the caret,
//   selected.
// - The arrow keys move through the list, and put each suggestion's text in
//   the field.
// - Escape brings back what the user typed.
// - Tab on a suggestion with a keyword enters keyword mode, such as "Search
//   YouTube".
//
// A normal or popup Browser has an omnibox. An app Browser has none, and calls
// on it return WREN_ERROR_NOT_ALLOWED.
//
// Offsets in the text, such as the caret and the ranges of a style, count
// UTF-16 code units, as NSString does. Text is copied out as UTF-8.

#ifndef WREN_PUBLIC_WREN_OMNIBOX_H_
#define WREN_PUBLIC_WREN_OMNIBOX_H_

#include "Wren.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Location state ---------------------------------------------------------

typedef enum WrenLocationText {
  // The active tab's address as Chrome shows it while the user is not editing
  // the field, such as "example.com/page", without "https://" or "www.".
  WREN_LOCATION_TEXT_DISPLAY = 0,
  // The address to put in the field when editing starts, as Chrome does. That
  // is the whole URL, such as "https://www.example.com/page". It leaves out
  // only "http://", a user name and password, and the trailing slash of a
  // bare host.
  WREN_LOCATION_TEXT_EDIT = 1,
  // What Chrome shows in an empty field, such as "Search Google or type a
  // URL", naming the default search engine. "" with no default search engine.
  WREN_LOCATION_TEXT_PLACEHOLDER = 2,
  // The full URL the address is about. That is the page's URL, or the one a
  // tab is loading because the user asked for it. "about:blank" while
  // show_url is false.
  WREN_LOCATION_TEXT_URL = 3,
} WrenLocationText;

typedef struct WrenLocationState {
  // True if Chrome shows the address. It does not on the New Tab page, where
  // the field is empty and shows the placeholder.
  bool show_url;
} WrenLocationState;

// Reads the location state of the Browser's active tab. Returns
// WREN_ERROR_NAVIGATION_UNAVAILABLE while the Browser has no tab. Once the
// app has read it, WREN_CHANGE_LOCATION reports each change.
WREN_API WrenResult Wren_GetLocationState(WrenBrowser* browser,
                                          WrenLocationState* state);
// Copies one of the location texts, in UTF-8. Returns the length needed,
// including the NUL. Copies "" when the state cannot be read.
WREN_API size_t Wren_CopyLocationText(WrenBrowser* browser,
                                      WrenLocationText text,
                                      char* buffer,
                                      size_t size);

// --- Suggestions ------------------------------------------------------------

// One set of suggestions. Its value changes whenever the list changes. Calls
// that act on a suggestion take it back, and return WREN_ERROR_STALE_RESULT
// for an older set. So a click that lands as the list changes never opens the
// suggestion that took the clicked one's place.
typedef uint64_t WrenOmniboxResult;

typedef enum WrenOmniboxFlag {
  // Do not complete inline, as Chrome's field does not after a deletion or a
  // paste.
  WREN_OMNIBOX_NO_INLINE = 1u << 0,
  // Control is held, as for Ctrl-Enter in Chrome. A typed name becomes
  // "www.<name>.com".
  WREN_OMNIBOX_ADD_COM = 1u << 1,
  // The user has just focused the field, not edited it. Chrome then lists
  // suggestions for an empty field or for the page's address, such as recent
  // and most visited pages, when Chromium has them.
  WREN_OMNIBOX_ON_FOCUS = 1u << 2,
} WrenOmniboxFlag;

typedef struct WrenOmniboxQuery {
  // What the field holds, in UTF-8. In keyword mode, the text after the chip.
  const char* text;
  // The caret, in UTF-16 code units. SIZE_MAX for the end.
  size_t cursor;
  // The keyword whose chip the field shows, or NULL. It is the
  // WREN_SUGGESTION_TEXT_KEYWORD of the suggestion the user pressed Tab on.
  const char* keyword;
  // WrenOmniboxFlag bits.
  uint32_t flags;
} WrenOmniboxQuery;

// Starts Chromium's autocomplete for `query`. It replaces any query running in
// the Browser.
//
// The first suggestions come at once. More come as slower sources answer,
// such as a search engine. WREN_CHANGE_OMNIBOX reports each change.
WREN_API WrenResult Wren_OmniboxQuery(WrenBrowser* browser,
                                      const WrenOmniboxQuery* query);
// Stops the query and clears the suggestions, as Chrome does when its list
// closes, for example on Escape or when the field loses focus.
// WREN_CHANGE_OMNIBOX follows.
WREN_API WrenResult Wren_OmniboxStop(WrenBrowser* browser);

typedef struct WrenOmniboxState {
  WrenOmniboxResult result;
  size_t suggestion_count;
  // True once Chromium expects no more suggestions for this query.
  bool done;
} WrenOmniboxState;

// Reads the Browser's current suggestions. There are none before a query, and
// none after Wren_OmniboxStop.
WREN_API WrenResult Wren_GetOmniboxState(WrenBrowser* browser,
                                         WrenOmniboxState* state);

typedef enum WrenSuggestionKind {
  // A search, either what was typed or one the search engine suggests.
  WREN_SUGGESTION_SEARCH = 0,
  // A search made before.
  WREN_SUGGESTION_SEARCH_HISTORY = 1,
  // An address, either what was typed or one a search engine suggests.
  WREN_SUGGESTION_URL = 2,
  // A page visited before.
  WREN_SUGGESTION_HISTORY = 3,
  // A bookmark.
  WREN_SUGGESTION_BOOKMARK = 4,
  // A tab already open. Opening it switches to it.
  WREN_SUGGESTION_OPEN_TAB = 5,
  // The answer to a calculation.
  WREN_SUGGESTION_CALCULATOR = 6,
  // What the clipboard holds.
  WREN_SUGGESTION_CLIPBOARD = 7,
  // Anything else Chromium offers, such as an extension's keyword.
  WREN_SUGGESTION_OTHER = 8,
} WrenSuggestionKind;

typedef struct WrenSuggestion {
  WrenSuggestionKind kind;
  // True for the suggestion Enter opens. It is the first suggestion, when
  // Chromium allows it to be the default. Its inline completion goes after
  // the caret.
  bool is_default;
  // True if its page is bookmarked. Chrome draws a star.
  bool bookmarked;
  // True if Wren_DeleteSuggestion can remove it, from history or from a
  // search engine's record of past searches. Chrome shows an X.
  bool deletable;
  // True if its page is open in a tab. WREN_OPEN_SWITCH_TO_TAB then switches
  // to that tab, as Chrome's "Switch to this tab" button does.
  bool has_tab;
  // True if Chrome shows the description before the contents, as for an
  // answer to a search.
  bool swap_contents_and_description;
} WrenSuggestion;

// Reads the suggestion at `index`.
WREN_API WrenResult Wren_GetSuggestion(WrenBrowser* browser,
                                       size_t index,
                                       WrenSuggestion* suggestion);

typedef enum WrenSuggestionText {
  // The main line, such as "youtube.com" or "weather paris".
  WREN_SUGGESTION_TEXT_CONTENTS = 0,
  // The second part, such as "YouTube" or "Google Search".
  WREN_SUGGESTION_TEXT_DESCRIPTION = 1,
  // What the field shows while the user selects the suggestion with the arrow
  // keys.
  WREN_SUGGESTION_TEXT_FILL = 2,
  // For the default suggestion, the text to add after the caret, selected, as
  // Chrome completes inline. "" if there is none.
  WREN_SUGGESTION_TEXT_INLINE = 3,
  // The keyword that Tab enters keyword mode with, such as "youtube.com", and
  // the name for its chip, such as "YouTube". Chrome's chip then says "Search
  // YouTube". Each is "" if there is none.
  WREN_SUGGESTION_TEXT_KEYWORD = 4,
  WREN_SUGGESTION_TEXT_KEYWORD_NAME = 5,
  // The header of the group the suggestion starts or belongs to, such as
  // "Recent searches", if Chrome shows one. "" if there is none.
  WREN_SUGGESTION_TEXT_GROUP = 6,
  // The URL the suggestion goes to. "" for one that does not navigate.
  WREN_SUGGESTION_TEXT_URL = 7,
} WrenSuggestionText;

// Copies one of the suggestion's texts, in UTF-8. Returns the length needed,
// including the NUL.
WREN_API size_t Wren_CopySuggestionText(WrenBrowser* browser,
                                        size_t index,
                                        WrenSuggestionText text,
                                        char* buffer,
                                        size_t size);

// Bits of WrenTextStyle.style, for drawing a suggestion's text as Chrome does.
typedef enum WrenTextStyleBits {
  // An address.
  WREN_TEXT_URL = 1u << 0,
  // Text that matches what was typed. Chrome draws it bold.
  WREN_TEXT_MATCH = 1u << 1,
  // Helper text, drawn dimmed.
  WREN_TEXT_DIM = 1u << 2,
} WrenTextStyleBits;

// A style that runs from `start`, in UTF-16 code units, to the next style's
// start, or to the end of the text.
typedef struct WrenTextStyle {
  size_t start;
  uint32_t style;
} WrenTextStyle;

// Copies the styles of a suggestion's CONTENTS or DESCRIPTION, in order, up
// to `capacity` of them. Returns how many there are. Returns 0 for the other
// texts.
WREN_API size_t Wren_GetSuggestionStyles(WrenBrowser* browser,
                                         size_t index,
                                         WrenSuggestionText text,
                                         WrenTextStyle* styles,
                                         size_t capacity);

// Copies the page's favicon for a suggestion of a page, that is of kind URL,
// HISTORY, BOOKMARK or OPEN_TAB. The PNG is at the largest scale Chromium
// keeps, 2x on a Mac, to draw at 16 points. Returns the length needed, or 0
// while there is no icon.
//
// Wren loads the icon from Chromium's favicon store when the suggestion
// arrives. WREN_CHANGE_OMNIBOX then reports it, with the same
// WrenOmniboxResult. For the other kinds, the app draws its own icon by kind.
WREN_API size_t Wren_CopySuggestionIconPNG(WrenBrowser* browser,
                                           size_t index,
                                           uint8_t* buffer,
                                           size_t size);

// Opens a suggestion as Chrome does when the user chooses it. The app then
// clears its field and calls Wren_OmniboxStop, as Chrome resets its field.
//
// Returns WREN_ERROR_INVALID_ARGUMENT for an index out of range, and for
// WREN_OPEN_SWITCH_TO_TAB on a suggestion without has_tab.
WREN_API WrenResult Wren_OpenSuggestion(WrenBrowser* browser,
                                        WrenOmniboxResult result,
                                        size_t index,
                                        WrenOpenDisposition disposition);
// Removes a deletable suggestion from where Chromium keeps it, as Chrome's X
// does. The suggestions then change. Returns WREN_ERROR_NOT_ALLOWED if the
// suggestion is not deletable.
WREN_API WrenResult Wren_DeleteSuggestion(WrenBrowser* browser,
                                          WrenOmniboxResult result,
                                          size_t index);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WREN_PUBLIC_WREN_OMNIBOX_H_
