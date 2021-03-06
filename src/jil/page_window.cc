#include "jil/page_window.h"

#include <cassert>

#include "wx/menu.h"

#include "editor/text_area.h"
#include "editor/text_buffer.h"
#include "editor/util.h"
#include "editor/wrap.h"

#include "jil/i18n_strings.h"
#include "jil/id.h"
#include "jil/text_page.h"

namespace jil {

IMPLEMENT_CLASS(PageWindow, editor::TextWindow)

PageWindow::PageWindow(TextPage* page)
    : editor::TextWindow(page->buffer())
    , page_(page) {
}

PageWindow::~PageWindow() {
}

void PageWindow::SetPage(TextPage* page) {
  assert(page != NULL);
  assert(page->buffer() != NULL);

  if (page_ == page) {
    return;
  }

  // The text page will not listen to its buffer any more.
  // Instead, the page window will listen.
  page->Detach();

  // The page window will not listen to its buffer any more. 
  buffer_->DetachListener(this);

  // Save the view state to the text page.
  GetState(page_->state());

  // Let the text page listen to its buffer.
  page_->Attach();

  // Set text page and buffer.
  page_ = page;
  buffer_ = page_->buffer();

  // Apply the view state from the new text page.
  SetState(page_->state());

  HandleTextChange();
  UpdateCaretPosition();

  // NOTE: Don't do this inside SetState().
  // Scroll to view start AFTER set virtual size.
  Scroll(page_->state()->view_start);

  Refresh();

  buffer_->AttachListener(this);
}

bool PageWindow::IsPagePlaceholder() const {
  return page_->buffer()->id() == kPlaceholderBufferId;
}

//------------------------------------------------------------------------------

void PageWindow::GetEditMenu(wxMenu* menu) {
  AppendMenuItem(menu, ID_MENU_EDIT_UNDO, kTrEditUndo);
  AppendMenuItem(menu, ID_MENU_EDIT_REDO, kTrEditRedo);
  menu->AppendSeparator();

  AppendMenuItem(menu, ID_MENU_EDIT_CUT, kTrEditCut);
  AppendMenuItem(menu, ID_MENU_EDIT_COPY, kTrEditCopy);
  AppendMenuItem(menu, ID_MENU_EDIT_PASTE, kTrEditPaste);
  menu->AppendSeparator();

  wxMenu* indent_menu = new wxMenu;
  AppendMenuItem(indent_menu, ID_MENU_EDIT_INCREASE_INDENT, kTrEditIncreaseIndent);
  AppendMenuItem(indent_menu, ID_MENU_EDIT_DECREASE_INDENT, kTrEditDecreaseIndent);
  AppendMenuItem(indent_menu, ID_MENU_EDIT_AUTO_INDENT, kTrEditAutoIndent);
  menu->AppendSubMenu(indent_menu, kTrEditIndent);

  wxMenu* comment_menu = new wxMenu;
  AppendMenuItem(comment_menu, ID_MENU_EDIT_COMMENT, kTrEditComment);
  AppendMenuItem(comment_menu, ID_MENU_EDIT_UNCOMMENT, kTrEditUncomment);
  menu->AppendSubMenu(comment_menu, kTrEditComment);
  menu->AppendSeparator();

  wxMenu* find_menu = new wxMenu;
  AppendMenuItem(find_menu, ID_MENU_EDIT_FIND, kTrEditFind);
  AppendMenuItem(find_menu, ID_MENU_EDIT_REPLACE, kTrEditReplace);
  AppendMenuItem(find_menu, ID_MENU_EDIT_FIND_NEXT, kTrEditFindNext);
  AppendMenuItem(find_menu, ID_MENU_EDIT_FIND_PREV, kTrEditFindPrev);
  menu->AppendSubMenu(find_menu, kTrEditFindReplace);
  menu->AppendSeparator();

  AppendMenuItem(menu, ID_MENU_EDIT_GO_TO, kTrEditGoTo);
}

bool PageWindow::GetEditMenuState(int menu_id) {
  switch (menu_id) {
  case ID_MENU_EDIT_UNDO:
    return CanUndo();

  case ID_MENU_EDIT_REDO:
    return CanRedo();

  case ID_MENU_EDIT_PASTE:
    return !editor::IsClipboardEmpty();

  default:
    return true;
  }
}

bool PageWindow::GetFileMenuState(int menu_id) {
  if (menu_id == ID_MENU_FILE_SAVE_AS) {
    return true;
  }

  return false;
}

bool PageWindow::HandleMenu(int menu_id) {
  editor::TextFunc* text_func = binding_->GetTextFuncByMenu(menu_id);
  if (text_func != NULL) {
    text_func->Exec(this);
    return true;
  }

  return false;
}

//------------------------------------------------------------------------------

void PageWindow::HandleTextRightUp(wxMouseEvent& evt) {
  wxMenu menu;
  menu.Append(ID_MENU_EDIT_CUT, kTrRClickCut);
  menu.Append(ID_MENU_EDIT_COPY, kTrRClickCopy);
  menu.Append(ID_MENU_EDIT_PASTE, kTrRClickPaste);

  wxPoint pos = text_area()->ClientToScreen(evt.GetPosition());
  pos = ScreenToClient(pos);
  PopupMenu(&menu, pos);
}

void PageWindow::GetState(PageState* state) const {
  state->allow_text_change = allow_text_change_;

  state->view_options = view_options_;

  state->caret_point = caret_point_;
  state->max_caret_x = max_caret_x_;

  state->view_start = GetViewStart();

  state->selection = selection_;

  // Transfer wrap helper.
  state->wrap_helper = wrap_helper_;
  wrap_helper_.reset();
}

void PageWindow::SetState(PageState* state) {
  allow_text_change_ = state->allow_text_change;

  caret_point_ = state->caret_point;
  max_caret_x_ = state->max_caret_x;

  // NOTE:
  // Don't scroll to the view_start right now.
  // Scroll after set virtual size.

  selection_ = state->selection;

  // Transfer wrap helper.
  wrap_helper_ = state->wrap_helper;
  state->wrap_helper.reset();

  if (view_options_.wrap) {
    // Create the wrap helper if necessary.
    if (!wrap_helper_) {
      wrap_helper()->Wrap(NULL);  
    }
  }

  SetViewOptions(state->view_options);

  // Rewrap if the client width has been changed.
  if (wrap_helper_) {
    int text_width = GetTextClientWidth();
    if (text_width != wrap_helper_->client_width()) {
      wrap_helper_->set_client_width(text_width);
      wrap_helper_->Wrap(NULL);
    }
  }
}

void PageWindow::OnPageModifiedStateChange(TextPage* page) {
  // Pass the text page, whose modified state has changed, as client data,
  // though it might not be useful to the final event handler.
  PostEvent(editor::TextWindow::kModifiedStateEvent, page);
}

}  // namespace jil
