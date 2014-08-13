#ifndef JIL_EDITOR_ACTION_H_
#define JIL_EDITOR_ACTION_H_
#pragma once

// Support undo & redo based on Command pattern.

#include <vector>
#include "wx/string.h"
#include "wx/datetime.h"
#include "editor/defs.h"
#include "editor/text_range.h"

namespace jil {
namespace editor {

class TextBuffer;

// Base class for actions operating a range of text.
class RangeAction {
 public:
  virtual ~RangeAction() {
  }

  const TextRange& range() const { return range_; }
  TextDir dir() const { return dir_; }
  bool selected() const { return selected_; }

  virtual TextRange SelectionAfterExec() const = 0;

 protected:
  RangeAction(const TextRange& range, TextDir dir, bool selected)
      : range_(range), dir_(dir), selected_(selected) {
  }

 protected:
  TextRange range_;

  // The direction of the text range.
  TextDir dir_;

  // If the text range is selected or not.
  bool selected_;
};

////////////////////////////////////////////////////////////////////////////////

// Base class for all kinds of actions.
class Action {
  wxDECLARE_NO_COPY_CLASS(Action);

 public:
  virtual ~Action();

  bool saved() const { return saved_; }
  void set_saved(bool saved) { saved_ = saved; }

  const wxDateTime& timestamp() const { return timestamp_; }

  const TextPoint& point() const { return point_; }
  void set_point(const TextPoint& point) { point_ = point; }

  const TextPoint& delta_point() const { return delta_point_; }

  const TextPoint& caret_point() const { return caret_point_; }
  void set_caret_point(const TextPoint& caret_point) {
    caret_point_ = caret_point;
  }

  bool effective() const { return effective_; }

  bool grouped() const { return grouped_; }
  void set_grouped(bool grouped) { grouped_ = grouped; }

  virtual void Exec() = 0;
  virtual void Undo() = 0;

  virtual TextPoint CaretPointAfterExec() const {
    return point_ + delta_point_;
  }

  virtual RangeAction* AsRangeAction() { return NULL; }

 protected:
  Action(TextBuffer* buffer, const TextPoint& point);

 protected:
  // If the change by this action was saved or not.
  bool saved_;

  // Text buffer on which this action is executed.
  TextBuffer* buffer_;

  // Action creation time.
  wxDateTime timestamp_;

  // Action (insert, delete, etc.) point.
  TextPoint point_;

  // Delta point determines the caret point after execute the action.
  // Normally, caret_point_after_exec = point_ + delta_point_;
  TextPoint delta_point_;

  // Action point might not be caret point (See NewLineBelow/Above text
  // functions). Keep it here for restoring caret point after undo.
  TextPoint caret_point_;

  // If the text buffer is changed or not after the execution of this action.
  bool effective_;

  // Flag marking the begin or end of a group of actions.
  // Example:
  // Actions:  X  X  X  X  X  X  X
  // Grouped:  0  1  1  1  0  1  0
  //              \__/  \_____/
  //               G       G
  bool grouped_;
};

////////////////////////////////////////////////////////////////////////////////

class InsertCharAction : public Action {
 public:
  InsertCharAction(TextBuffer* buffer, const TextPoint& point, wchar_t c);
  virtual ~InsertCharAction();

  TextDir dir() const { return dir_; }
  void set_dir(TextDir dir) { dir_ = dir; }

  wchar_t c() const { return c_; }

  virtual void Exec() override;
  virtual void Undo() override;

 private:
  // Normally the dir is forward.
  // If it's backward, caret remains after insert. (See NewLineAbove)
  // Examples:
  // - Forward
  //   Given:
  //     a|bc (| indicates the caret)
  //   Insert char 'x':
  //     ax|bc
  // - Backward
  //   Given:
  //     a|bc
  //   Insert char 'x':
  //     a|xbc
  TextDir dir_;

  wchar_t c_;
};

////////////////////////////////////////////////////////////////////////////////

class InsertStringAction : public Action {
 public:
  InsertStringAction(TextBuffer* buffer,
                     const TextPoint& point,
                     const std::wstring& str);
  virtual ~InsertStringAction();

  virtual void Exec() override;
  virtual void Undo() override;

 private:
  std::wstring str_;
};

////////////////////////////////////////////////////////////////////////////////

class InsertTextAction : public Action {
 public:
  InsertTextAction(TextBuffer* buffer,
                   const TextPoint& point,
                   const std::wstring& text);
  virtual ~InsertTextAction();

  virtual void Exec() override;
  virtual void Undo() override;

 private:
  std::wstring text_;
};

////////////////////////////////////////////////////////////////////////////////

class DeleteAction : public Action {
 public:
  DeleteAction(TextBuffer* buffer,
               const TextPoint& point,
               TextUnit text_unit,
               SeekType seek_type);
  virtual ~DeleteAction();

  virtual void Exec() override;
  virtual void Undo() override;

  virtual TextPoint CaretPointAfterExec() const override;

  TextUnit text_unit() const { return text_unit_; }
  SeekType seek_type() const { return seek_type_; }

  size_t count() const { return count_; }
  void set_count(size_t count) { count_ = count; }

  bool SameKind(const DeleteAction& rhs) const;

  bool Merge(DeleteAction* rhs);
  void DeleteText(const TextRange& range);

 private:
  TextPoint Seek(const TextPoint& point,
                 TextUnit text_unit,
                 SeekType seek_type,
                 size_t count) const;

 private:
  std::wstring text_;
  TextUnit text_unit_;
  SeekType seek_type_;
  size_t count_;
};

////////////////////////////////////////////////////////////////////////////////

class DeleteRangeAction : public Action, public RangeAction {
 public:
  DeleteRangeAction(TextBuffer* buffer,
                    const TextRange& range,
                    TextDir dir,
                    bool selected);
  virtual ~DeleteRangeAction();

  virtual void Exec() override;
  virtual void Undo() override;

  virtual RangeAction* AsRangeAction() override {
    return this;
  }

  virtual TextRange SelectionAfterExec() const override {
    // No selection after execution.
    return TextRange();
  }

 private:
  std::wstring text_;
};

////////////////////////////////////////////////////////////////////////////////

// Increase indent of a range of lines.
// Indent can be implemented by a group of InsertChar actions. But defining
// a specific action for it has the advantage of better performance and
// easier caret point handling.
class IncreaseIndentAction : public Action, public RangeAction {
 public:
  IncreaseIndentAction(TextBuffer* buffer,
                       const TextRange& range,
                       TextDir dir,
                       bool selected);
  virtual ~IncreaseIndentAction();

  virtual void Exec() override;
  virtual void Undo() override;

  // Example:
  // This is the line to be indented.
  //            ^ (caret)
  // Now increase indent:
  //     This is the line to be indented.
  //                ^
  // The line is indented, and the caret is also "indented".
  virtual TextPoint CaretPointAfterExec() const override {
    return caret_point_ + delta_point_;
  }

  virtual RangeAction* AsRangeAction() override { return this; }

  // Return the select range after execute this action.
  // Examples:
  // - Single line selection:
  // This is the line to be indented.
  //         ******** (selection)
  // Now increase indent:
  //     This is the line to be indented.
  //             ********
  // The selection is also "indented".
  //
  // - Multi-line selection:
  // This is line one to be indented.
  //         ************************
  // This is line two to be indented.
  // ****************
  // Now increase indent:
  //     This is line one to be indented.
  //             ************************
  //     This is line two to be indented.
  // ********************
  // The selection is also "indented".
  virtual TextRange SelectionAfterExec() const override;

 private:
  std::wstring spaces_;
};

////////////////////////////////////////////////////////////////////////////////

// Decrease indent of a range of lines.
class DecreaseIndentAction : public Action, public RangeAction {
 public:
  DecreaseIndentAction(TextBuffer* buffer,
                       const TextRange& range,
                       TextDir dir,
                       bool selected);
  virtual ~DecreaseIndentAction();

  virtual void Exec() override;
  virtual void Undo() override;

  virtual TextPoint CaretPointAfterExec() const override;

  virtual RangeAction* AsRangeAction() override { return this; }

  virtual TextRange SelectionAfterExec() const override;

 private:
  // Return true if the indent is changed.
  bool DecreaseIndentLine(Coord ln);

 private:
  std::vector<std::wstring> spaces_array_;
};

////////////////////////////////////////////////////////////////////////////////

// Automatically indent a range of lines.
class AutoIndentAction : public Action, public RangeAction {
 public:
  AutoIndentAction(TextBuffer* buffer,
                   const TextRange& range,
                   TextDir dir,
                   bool selected);
  virtual ~AutoIndentAction();

  virtual void Exec() override;
  virtual void Undo() override;

  virtual TextPoint CaretPointAfterExec() const override;

  virtual RangeAction* AsRangeAction() override { return this; }

  virtual TextRange SelectionAfterExec() const override;

 private:
  // Return true if the indent is changed.
  bool AutoIndentLine(Coord ln);

  void GetIndentStr(Coord indent, std::wstring* indent_str);

 private:
  std::vector<std::wstring> old_indent_strs_;
  std::vector<std::wstring> new_indent_strs_;
};

////////////////////////////////////////////////////////////////////////////////

// Set the file format, or line endings.
class SetFileFormatAction : public Action {
 public:
  SetFileFormatAction(TextBuffer* buffer,
                      const TextPoint& point,
                      FileFormat file_format);
  virtual ~SetFileFormatAction();

  virtual void Exec() override;
  virtual void Undo() override;

  virtual TextPoint CaretPointAfterExec() const override {
    return caret_point_;
  }

  FileFormat file_format() const {
    return file_format_;
  }

  void set_file_format(FileFormat file_format) {
    file_format_ = file_format;
  }

 private:
  FileFormat file_format_;
  FileFormat old_file_format_;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_ACTION_H_
