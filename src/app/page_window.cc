#include "app/page_window.h"

#include <cassert>

#include "wx/menu.h"

#include "editor/text_area.h"  // TODO
#include "editor/text_buffer.h"
#include "editor/util.h"

#include "app/i18n_strings.h"
#include "app/id.h"
#include "app/save.h"
#include "app/text_page.h"

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

  if (page_ != page) {
    // Save page state.
    GetState(page_->state());

    page_ = page;

    // Set buffer and restore page state.

    buffer_->DetachListener(this);

    buffer_ = page_->buffer();

    SetState(page_->state());

    HandleTextChange();
    UpdateCaretPosition();
    Refresh();

    buffer_->AttachListener(this);
  }
}

void PageWindow::Page_EditMenu(wxMenu* menu) {
  if (page_ == NULL) {
    return;
  }

  //------------------------------------

  AppendMenuItem(menu, ID_MENU_EDIT_UNDO, kTrEditUndo);
  AppendMenuItem(menu, ID_MENU_EDIT_REDO, kTrEditRedo);
  menu->AppendSeparator();

  //------------------------------------

  AppendMenuItem(menu, ID_MENU_EDIT_CUT, kTrEditCut);
  AppendMenuItem(menu, ID_MENU_EDIT_COPY, kTrEditCopy);
  AppendMenuItem(menu, ID_MENU_EDIT_PASTE, kTrEditPaste);
  menu->AppendSeparator();

  //------------------------------------

  wxMenu* indent_menu = new wxMenu;
  AppendMenuItem(indent_menu, ID_MENU_EDIT_INCREASE_INDENT, kTrEditIncreaseIndent);
  AppendMenuItem(indent_menu, ID_MENU_EDIT_DECREASE_INDENT, kTrEditDecreaseIndent);
  AppendMenuItem(indent_menu, ID_MENU_EDIT_AUTO_INDENT, kTrEditAutoIndent);
  menu->AppendSubMenu(indent_menu, kTrEditIndent);

  //------------------------------------

  wxMenu* comment_menu = new wxMenu;
  AppendMenuItem(comment_menu, ID_MENU_EDIT_COMMENT, kTrEditComment);
  AppendMenuItem(comment_menu, ID_MENU_EDIT_UNCOMMENT, kTrEditUncomment);
  menu->AppendSubMenu(comment_menu, kTrEditComment);
  menu->AppendSeparator();

  //------------------------------------

  AppendMenuItem(menu, ID_MENU_EDIT_FIND, kTrEditFind);
  AppendMenuItem(menu, ID_MENU_EDIT_REPLACE, kTrEditReplace);
  AppendMenuItem(menu, ID_MENU_EDIT_FIND_NEXT, kTrEditFindNext);
  AppendMenuItem(menu, ID_MENU_EDIT_FIND_PREV, kTrEditFindPrev);
  menu->AppendSeparator();

  AppendMenuItem(menu, ID_MENU_EDIT_GO_TO, kTrEditGoTo);
}

bool PageWindow::Page_EditMenuState(int menu_id) {
  if (page_ == NULL) {
    return false;
  }

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

bool PageWindow::Page_FileMenuState(int menu_id, wxString* text) {
  if (page_ == NULL) {
    return false;
  }

  if (menu_id == ID_MENU_FILE_SAVE_AS) {
    if (text != NULL) {
      // TODO: The page label might be too long.
      *text = wxString::Format(kTrFileSaveAsFormat, page_->GetLabel());
    }
    return true;
  }

  return false;
}

bool PageWindow::Page_OnMenu(int menu_id) {
  if (page_ == NULL) {
    return false;
  }

  editor::TextFunc* text_func = binding_->GetTextFuncByMenu(menu_id);
  if (text_func != NULL) {
    text_func->Exec(this);
    return true;
  }

  return false;
}

void PageWindow::Page_OnSaveAs() {
  if (page_ == NULL) {
    return;
  }

  SaveBufferAs(buffer_, NULL);
}

void PageWindow::HandleTextRightUp(wxMouseEvent& evt) {
  if (page_ == NULL) {
    return;
  }

  wxMenu menu;
  menu.Append(ID_MENU_EDIT_CUT, kTrRClickCut);
  menu.Append(ID_MENU_EDIT_COPY, kTrRClickCopy);
  menu.Append(ID_MENU_EDIT_PASTE, kTrRClickPaste);

  // TODO: Add a method to TextWindow.
  wxPoint pos = text_area()->ClientToScreen(evt.GetPosition());
  pos = ScreenToClient(pos);
  PopupMenu(&menu, pos);
}

//------------------------------------------------------------------------------

void PageWindow::GetState(PageState* state) const {
  state->allow_text_change = allow_text_change_;

  state->view_options = view_options_;

  state->caret_point = caret_point_;
  state->max_caret_x = max_caret_x_;

  state->selection = selection_;
}

void PageWindow::SetState(const PageState* state) {
  allow_text_change_ = state->allow_text_change;

  view_options_ = state->view_options;

  caret_point_ = state->caret_point;
  max_caret_x_ = state->max_caret_x;

  selection_ = state->selection;
}

}  // namespace jil
