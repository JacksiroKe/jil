#ifndef JIL_ABOUT_DIALOG_H_
#define JIL_ABOUT_DIALOG_H_

#include "wx/dialog.h"

class wxStaticText;

namespace jil {

class AboutDialog : public wxDialog {
public:
  explicit AboutDialog(wxWindow* parent);

  virtual ~AboutDialog();

private:
  wxStaticText* NewLabel(const wxString& str);
};

}  // namespace jil

#endif  // JIL_ABOUT_DIALOG_H_
