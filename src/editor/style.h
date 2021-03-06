#ifndef EDITOR_STYLE_H_
#define EDITOR_STYLE_H_

#include <vector>

#include "wx/colour.h"
#include "wx/font.h"
#include "wx/string.h"

#include "editor/lex.h"

namespace editor {

class StyleValue {
public:
  StyleValue(const wxColour& fg = wxNullColour,
             const wxColour& bg = wxNullColour,
             int font = 0)
      : fg_(fg), bg_(bg), font_(font) {
  }

  void Set(const wxColour& fg = wxNullColour,
           const wxColour& bg = wxNullColour,
           int font = 0) {
    fg_ = fg;
    bg_ = bg;
    font_ = font;
  }

  const wxColour& fg() const { return fg_; }
  const wxColour& bg() const { return bg_; }
  int font() const { return font_; }

  void set_fg(const wxColour& fg) { fg_ = fg; }
  void set_bg(const wxColour& bg) { bg_ = bg; }
  void set_font(int font) { font_ = font; }

private:
  wxColour fg_;
  wxColour bg_;
  int font_;  // Examples: kBold, kBold | kUnderline.
};

class Style {
public:
  enum Font {
    kBold = 1,
    kItalic = 2,
    kUnderline = 4
  };

  // Non-lex style items:
  enum Item {
    kNormal = 0,
    kVisual,      // Selected text
    kMatching,    // Find matching
    kNumber,      // Line number area
    kCaretLine,
    kSpace,       // Space and Tab
    kBlank,       // Blank area (similar to Vim NonText)
    kDelimiter,   // !@#$%^&*()+-=~, etc.
    kItemCount,
  };

public:
  Style();
  ~Style();

  void Set(int item,
           const wxColour& fg = wxNullColour,
           const wxColour& bg = wxNullColour,
           int font = 0);

  void Set(int item, const StyleValue& rhs) {
    Set(item, rhs.fg(), rhs.bg(), rhs.font());
  }

  const StyleValue* Get(int item) const {
    return values_[item];
  }

  void Set(Lex lex,
           const wxColour& fg = wxNullColour,
           const wxColour& bg = wxNullColour,
           int font = 0);

  void Set(Lex lex, const StyleValue& rhs) {
    Set(lex, rhs.fg(), rhs.bg(), rhs.font());
  }

  const StyleValue* Get(Lex lex) const;

private:
  typedef std::vector<StyleValue*> Values;
  Values values_;
  Values lex_values_[kLexMajorTypeCount];
};

}  // namespace editor

#endif  // EDITOR_STYLE_H_
