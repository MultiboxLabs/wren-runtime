# The UI a Wren browser provides

A Wren app owns its windows and draws its browser UI. Chromium's browser layer runs inside it. This page lists every piece of UI a browser on Wren has, and what the app does for each. It either reuses Chromium's or draws its own.

- **Reuse Chromium's.** The app adds a button, names an anchor with `Wren_SetAnchor`, or makes a call. Chromium draws the rest.
- **Draw its own.** The app reads a request or a state, and follows its change kind.

Many pieces offer both, chosen per request. Chromium asks before it opens one of its popovers, through `on_popover`. The app answers Chromium's, its own, or none.

Where a table says to name an anchor, an app with native UI names a view with `Wren_SetAnchor`. An app whose UI is a page in a web view names an element of that page with `Wren_SetAnchorElement`. See [anchors in a web view](anchors-in-web-views.md). `WrenToolbar.h` lists the anchors' names. The app names a view for each feature it has a button for, such as "passwords", "zoom" or "cast", and the address field stands in for the rest. `Wren_GetAnchorState` says whether one of Chromium's bubbles hangs off an anchor now, so the app can draw its button pressed. `WREN_CHANGE_ANCHORS` reports each change.

The API is in [`Wren.h`](../include/Wren.h), [`WrenUI.h`](../include/WrenUI.h), [`WrenOmnibox.h`](../include/WrenOmnibox.h), [`WrenBookmarks.h`](../include/WrenBookmarks.h) and [`WrenSession.h`](../include/WrenSession.h). What this release does not do is in [This release](this-release.md). Wren's sample browser draws its own site panel, extensions panel and password prompts, and reuses Chromium's for the rest.

The tables name the C functions. The Swift API and the Rust crate have each under their own name. `Wren_GetSiteState` is `browser.siteState()` in Swift and `Browser::site_state` in Rust, and `WREN_CHANGE_SITE` is `ChangeKind.site` and `ChangeKind::Site`. Each Swift and Rust item's comment names its C function, so searching `wren-swift` or `wren-rust` for a C name finds its wrapper. [Windows and tabs](windows-and-tabs.md) explains the first table at length.

## The window and tabs

The app draws all of these. There is nothing of Chromium's to reuse.

| UI | What the app does | API |
| --- | --- | --- |
| Tab strip | Draws the tabs, their titles, favicons, loading and pinning, and the New Tab button | `Wren_GetTabCount`, `Wren_GetTabAt`, `Wren_GetTabState`, `Wren_NewTab`; `WREN_CHANGE_TABS`, `WREN_CHANGE_TAB` |
| Tab views | Places each tab's view in its window, shows the active one, at a whole number of points each way: Chromium drops a fraction, which leaves a one-pixel strip at the view's bottom or right edge | `Wren_GetTabView`, `Wren_AttachBrowserToView`, `Wren_FocusTab` |
| Crashed tab | Draws its own "Aw, snap!" over the tab, with a reload | `WrenTabState.crash`, `Wren_Reload` |
| Tab alerts and the speaker | Draws Chrome's icon for what a tab is doing: the first of its alerts, in the header's order, or the muted speaker for a muted tab that plays sound. A speaker or a menu item mutes and unmutes the tab | `WrenTabState.alerts`, `WrenTabState.muted`, `Wren_SetTabMuted`; `WREN_TAB_FIELD_ALERTS`, `WREN_TAB_FIELD_MUTED` |
| Tab groups | Draws group labels and colours; menus to group and ungroup | `Wren_GroupTabs`, `Wren_SetTabGroupVisuals`; `WREN_CHANGE_GROUP` |
| Moving tabs to another window | Its own menu items, or a drag between windows, that move tabs to another Browser of the Profile or a new one. Brings the other window forward itself | `Wren_MoveTabsToBrowser`, `Wren_MoveTabsToNewBrowser` |
| Pictures of tabs | Draws a tab's picture where it wants one, such as on a hover card or in a tab switcher. Wren takes it on request, of a shown or a background tab | `Wren_CaptureTab` |
| Split tabs | Lays out both sides and draws the divider | `Wren_SplitTabs`, `Wren_SetSplitRatio`; `WREN_CHANGE_SPLIT` |
| Side panels | Places the side panel view, at the width Chromium remembers, and draws its own resize handle; Chromium puts each panel's page in it | `Wren_GetSidePanelView`, `Wren_ShowSidePanel`, `Wren_SetSidePanelWidth`; `WREN_CHANGE_SIDE_PANEL` |
| Status bubble | Draws Chrome's bubble at the bottom left of a page, or leaves it out: the address of the link under the pointer, or what a loading page waits for. Chrome shows it 80 ms after the text comes and fades it 250 ms after it goes, a third of the page wide with the address elided, growing to fit after 1.6 s, and moves it out of the pointer's way | `WrenTabState.status`, `Wren_CopyTabStatusText`; `WREN_TAB_FIELD_STATUS` |
| Popups and other windows Chromium opens | Shows each in a window of its own with the parts Chrome gives that kind: a page's popup gets its address, read-only; an extension's popup window gets none | `on_browser_opened`, `Wren_GetBrowserType`, `Wren_GetWindowFeatures`, `Wren_GetRequestedBounds` |
| DevTools | Nothing: docked DevTools is part of the tab's view; undocked it is a Browser of its own | `Wren_OpenDevTools`; `on_browser_opened` |
| Menus and commands | Uses Wren's default menu, Chrome's (`WREN_MENU_CHROMIUM`), or its own | `WrenStartup.menu` |
| Browser-modal dialogs | Blocks its own UI while one is open, as Chrome blocks its window | `Wren_HasModalDialog`; `WREN_CHANGE_MODAL_DIALOG` |
| Restoring the last session | Asks for it when it wants its windows and tabs back (at launch, from a button, or never); each window arrives as a Browser to show, with the frame it had | `Wren_GetLastSession`, `Wren_RestoreLastSession`; `on_browser_opened`, `Wren_GetRequestedBounds` |
| Bookmarks bar and folders | Draws the Profile's bookmarks bar (the bar folder's items, its folders as menus) and whatever else of Chrome's bookmark manager it offers: adding, renaming, moving and removing; keeps folders of its own by their saved IDs; opens a bookmark as Chrome's bar does. Chromium's own bar is not shown | `Wren_GetBookmarksState`, `Wren_GetBookmarkChildAt`, `Wren_GetBookmarkItem`, `Wren_AddBookmarkItem`, `Wren_MoveBookmarkItem`, `Wren_OpenBookmarkItem`; `WREN_CHANGE_BOOKMARKS` |
| Reopen Closed Tab | Adds the menu item or button, enabled while Chromium has something to reopen | `Wren_CanReopenClosedTab`, `Wren_ReopenClosedTab`; `WREN_CHANGE_CLOSED_TABS` |
| Full screen | Takes its window in and out of full screen when Chromium asks (a page, Escape), reports the window's changes, and shows only the page while one is in full screen; Chromium shows its "Press Esc" notice over the page | `on_fullscreen`, `Wren_SetFullscreen`, `Wren_GetFullscreenTab`; `WREN_CHANGE_FULLSCREEN` |
| Moving, resizing, minimising | Does what Chromium asks of a window, adapts it or ignores it (`on_window_request`): an extension's `chrome.windows.update`, a debugging tool, a popup's `resizeTo`. Shows a new window minimised if `Wren_GetRequestedWindowState` says so. Chromium reads the window as it is |

### A window drawn in HTML

A window whose UI is one [web view](web-views.md)'s page has the rows above, drawn by the page from what the app sends it. It also has these of its own. [A browser drawn in HTML](html-browser.md) goes through them.

| UI | What the app does | API |
| --- | --- | --- |
| The window | Makes it, with a hidden title bar if the page draws one, places the web view in it and attaches each Browser to the web view's view | `Wren_CreateWebView`, `Wren_AttachBrowserToView` |
| Title bar | Nothing but the option: the page marks what moves the window in CSS (`app-region`) and leaves the window's buttons room | `WrenWebViewOptions.drags_window` |
| Tabs and side panels | Tells the page which IDs to show; the page writes `<wren-view>` | `WrenWebViewOptions.embeds_views` |
| Toolbar buttons | Answers the page's calls, and names the page's marks as each Browser's anchors | `WrenWebViewOptions.messages` and `anchors`; `Wren_SetAnchorElement` |
| Workspaces (several Browsers shown in turn) | Keeps all but one in the background, and decides when Chromium asks for another to come forward | `Wren_SetBrowserBackgrounded`; `WREN_CHANGE_FOREGROUND_REQUEST` |
| The page failing or crashing | Reloads it, or shows something of its own | `Wren_GetWebViewState`, `Wren_ReloadWebView`; `WREN_CHANGE_WEB_VIEW` |
| A development server | Lists its origin for the view, in development builds only | `WrenWebViewOptions.trusted_origins` |
| Context menus | Shows the items the page sends for its right clicks, as a message; a text field has Wren's editing menu | `Wren_ShowContextMenu` ([Menus](menus.md)) |

## The address field

| UI | Reuse Chromium's | Draw your own |
| --- | --- | --- |
| Address and focus | — | Draws the field; `on_focus_location_bar` asks for focus; for Search the Web, `Wren_GetLocationFocus` says it is for a search, in the keyword mode of `WREN_LOCATION_TEXT_SEARCH_KEYWORD` |
| What the field shows | — | The address trimmed for display, the whole one for editing, and the placeholder naming the search engine: `Wren_GetLocationState`, `Wren_CopyLocationText`; `WREN_CHANGE_LOCATION` |
| Suggestions as the user types | — | Chromium's autocomplete for the text, as data: `Wren_OmniboxQuery`, `Wren_GetOmniboxState`, `Wren_GetSuggestion`, `Wren_CopySuggestionText`; `WREN_CHANGE_OMNIBOX`. Draw the list; open a row with `Wren_OpenSuggestion`, so history and search engines learn as in Chrome |
| Site icon and security | Name the icon as `"location"`; its click opens page info (`Wren_ShowPageInfo`) | The security, always shown: `Wren_GetSiteState`; `WREN_CHANGE_SITE` |
| Page actions (key, zoom, star, translate, intent picker) | Draw the icons Wren gives (`Wren_GetPageActionState`, `Wren_CopyPageActionIconPNG`); clicks with `Wren_ClickPageAction` | The same icons, or your own; `WREN_CHANGE_PAGE_ACTIONS` |
| Content setting icons (blocked popup, location in use) | Draw the icons Wren gives (`Wren_GetContentSettingIcon`), highlighted while their bubble is open (`Wren_IsContentSettingBubbleOpen`); clicks with `Wren_ShowContentSettingBubble` | `WREN_CHANGE_CONTENT_SETTINGS` |
| Cast button | Draw the button Wren gives (`WREN_PAGE_ACTION_CAST`): visible while Chrome would show its own, pressed while Cast's dialog is open; the click opens or closes the dialog. `WREN_CHROMIUM_COMMAND_CAST` is Cast… for the app's menu | `WREN_CHANGE_PAGE_ACTIONS` |
| Where the address field's bubbles hang | Name the whole field as `"address-field"` | — |

## Popovers

Each popover asks `on_popover` first, whatever opened it. The app answers one of three things.

- `WREN_POPOVER_CHROMIUM` shows Chromium's, at the anchor named. It is also what happens with no handler.
- `WREN_POPOVER_APP` means the app draws its own from the data.
- `WREN_POPOVER_NONE` shows nothing.

A request the app takes stays open until `Wren_ClosePopover`, or until Wren withdraws it. `WREN_CHANGE_POPOVER` reports either.

| Popover | Asked when | Draw your own from |
| --- | --- | --- |
| Page info | `Wren_ShowPageInfo` | The site state: the origin, security, permissions by ID, devices, cookies, and their actions (`Wren_GetSiteState`, `Wren_SetSitePermission`, `Wren_SiteAction`) |
| Extensions menu | `Wren_ShowExtensionsMenu`; Chromium's menu commands | What each extension may do on the site (`Wren_GetExtensionsSite`, `Wren_GetExtensionOnSite`, the requests), or either of Chromium's designs (unstable) |
| Zoom | Cmd-+ and the like, an extension's zoom, the zoom icon | The zoom state (`Wren_GetZoomState`, `Wren_CopyZoomExtensionID`, `Wren_Zoom`); close it by yourself after 1.5 s, as Chrome |
| Find bar | Cmd-F, Find Next, `Wren_Find`; a tab with a find coming back | The find state (`Wren_GetFindState`, `Wren_CopyFindString`); drive it with `Wren_SetFindString` and `Wren_Find`: Return and Shift-Return step, Escape ends, Control-Return clicks a link's match; focus your field when asked |
| Download bubble | All of a Profile's downloads done (its last active Browser); `Wren_ClickDownloadButton`; a warning from chrome://downloads | The Profile's downloads (`Wren_GetDownloadCount`, `Wren_GetDownloadAt`, `Wren_GetDownloadState`), acted on with `Wren_DoDownloadAction`; close the short one after 5 s unless used, as Chrome |
| Bookmark star | Cmd-D, the star (in an active window, as Chrome) | The bookmark (`Wren_GetBookmark`, the suggested folders by ID, `Wren_SetBookmarkTitle`, `Wren_MoveBookmark`) |
| Content setting bubble | `Wren_ShowContentSettingBubble` | What was blocked and the site's value (`Wren_GetContentSettingState`, `Wren_CopyContentSettingItemURL`, `Wren_SetContentSettingValue`) |
| Translate | Translate offering itself, the translate icon | The request's translate data (`Wren_GetTranslateState`, `Wren_CopyTranslateLanguageCode`); needs a Google API key |
| Save or update a password, the saved passwords | After a sign-in, the key; never while the Profile's password manager is off (`Wren_SetPasswordManager`) | The request's passwords data (`Wren_GetPasswordsState`, the eye with `Wren_RevealPasswords`, `Wren_SavePassword`) |
| Save or update an address | After a form | The request's address data (`Wren_GetAddressState`, `Wren_AnswerAddressPrompt`) |
| Save a card on this device | After a payment form | The request's card data (`Wren_GetCardState`: the network; `Wren_CopyCardLastDigits`, `Wren_SaveCard`); never the number |
| Permission prompt | A page asks for a permission | The request's permission data (`Wren_GetPermissionPromptState`, `Wren_CopyPermissionID`, `Wren_AnswerPermissionPrompt`); most apps answer Chromium's |
| Toast ("Link copied") | Chromium shows one: copying a link, adding to the reading list | The request's toast data (`WrenPopoverRequest.toast` names it; `Wren_GetToastState`, its icon, `Wren_PressToastButton`, its menu); Chromium closes it when its own would go. Chromium's hangs off the "toast" anchor. The app's own toasts (`Wren_ShowToast`) come the same way, named as the app named them, or "Embedder" |

The texts of every popover are Chrome's, in `WrenUnstableUI.h`, which may change in any release. See [compatibility](compatibility.md). Everything in this table is in the stable part of the API.

Show an automatic request, `WREN_POPOVER_AUTOMATIC`, without taking focus. Inside `on_popover`, only decide. Wren refuses calls that change Browsers or tabs until it returns.

## Extensions

| UI | Reuse Chromium's | Draw your own |
| --- | --- | --- |
| Action buttons | — | Draw them: `Wren_GetExtensionActionCount`, `Wren_CopyExtensionActionIconPNG`, `Wren_ClickExtensionAction`; name each as `"extension:<id>"`; `WREN_CHANGE_EXTENSION_ACTIONS` |
| Popups | Chromium's, below the action's button (they are pages) | — |
| Context menu | `Wren_ShowExtensionActionMenu` | — |
| Extensions button and menu | Name the button `"extensions"`; its menu is a popover (above) | The button's state: `Wren_GetExtensionsButtonState`, `Wren_GrantRequestedSiteAccess` |
| Installed extensions, on and off | Chromium's extensions page (`chrome://extensions`) | List a Profile's and turn them on or off, or allow them in incognito: `Wren_GetExtensionCount`, `Wren_GetExtensionState`, `Wren_CopyExtensionIconPNG`, `Wren_SetExtensionEnabled`, `Wren_SetExtensionAllowedInIncognito`; `WREN_CHANGE_EXTENSIONS` |

## Chromium's own, to reuse

These stand on their own and stay Chromium's. The app gives them a place, and nothing more. Attach every Browser you show with `Wren_AttachBrowserToView`, and name the anchors.

- Dialogs and sheets: extension install and uninstall, the bookmark and address editors, the certificate viewer, site data, form resubmission.
- Device choosers (USB, HID, serial, Bluetooth), passkeys and WebAuthn, HTTP authentication, a page's alerts and confirms.
- The prompts of a page's `<permission>` elements (`<geolocation>`, `<usermedia>`), and of the New Tab page's microphone when you answer "Chromium's": a sheet over the page, centred or by the element. Wren dims the page in the tab's view, so the dim stays inside however you clip the page.
- The find bar, unless you draw your own. It hangs below the right end of the `"address-field"` anchor, or at the page's top right without one.
- The download bubble, unless you draw your own. It hangs below the `"downloads"` anchor, or else the `"address-field"` one.
- Chrome's prompts about downloads in progress, before an off-the-record Profile's last Browser closes or the app quits. To ask yourself, answer the app's in `on_downloads_prompt`, then answer with `Wren_AnswerDownloadsPrompt`.
- Chrome's download button belongs to its toolbar. Draw yours from `Wren_GetDownloadButtonState`, which says whether it shows, its icon, its progress ring, and whether it is active. Name it `"downloads"`, and click it with `Wren_ClickDownloadButton`.
- chrome://downloads, unless you draw your own. Draw it from `Wren_GetDownloadHistory`, newest first, with each download's `start_ms` and `end_ms` for its day and `Wren_GetDownloadHistoryState` for whether it has loaded. Remove from List is `WREN_DOWNLOAD_REMOVE`, and Clear All is `Wren_ClearDownloadHistory`; neither has an Undo, so show yours first. Hide both where the state says the history is not `removable`.

## Not available yet

Infobars are not available. That includes page info's offer to reload, for which the site state's `reload_needed` stands in. Neither are Lens's overlay and the Chrome commands Wren disables. See [This release](this-release.md).
