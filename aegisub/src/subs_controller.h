// Copyright (c) 2013, Thomas Goyne <plorkyeran@aegisub.org>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
// Aegisub Project http://www.aegisub.org/

#include <libaegisub/fs_fwd.h>
#include <libaegisub/signal.h>

#include <boost/container/list.hpp>
#include <boost/filesystem/path.hpp>
#include <set>
#include <wx/timer.h>

class AssEntry;
class AssFile;
struct AssFileCommit;

namespace agi { struct Context; }

class SubsController {
	agi::Context *context;
	agi::signal::Connection undo_connection;

	struct UndoInfo;
	boost::container::list<UndoInfo> undo_stack;
	boost::container::list<UndoInfo> redo_stack;

	/// Revision counter for undo coalescing and modified state tracking
	int commit_id = 0;
	/// Last saved version of this file
	int saved_commit_id = 0;
	/// Last autosaved version of this file
	int autosaved_commit_id = 0;

	/// Timer for triggering autosaves
	wxTimer autosave_timer;

	/// A new file has been opened (filename)
	agi::signal::Signal<agi::fs::path> FileOpen;
	/// The file is about to be saved
	/// This signal is intended for adding metadata such as video filename,
	/// frame number, etc. Ideally this would all be done immediately rather
	/// than waiting for a save, but that causes (more) issues with undo
	agi::signal::Signal<> FileSave;

	/// The filename of the currently open file, if any
	agi::fs::path filename;

	void OnCommit(AssFileCommit c);

	/// Set the filename, updating things like the MRU and last used path
	void SetFileName(agi::fs::path const& file);

public:
	SubsController(agi::Context *context);

	/// The file's path and filename if any, or platform-appropriate "untitled"
	agi::fs::path Filename() const;

	/// Does the file have unsaved changes?
	bool IsModified() const { return commit_id != saved_commit_id; };

	/// @brief Load from a file
	/// @param file File name
	/// @param charset Character set of file or empty to autodetect
	void Load(agi::fs::path const& file, std::string charset="");

	/// @brief Save to a file
	/// @param file Path to save to
	/// @param encoding Encoding to use, or empty to let the writer decide (which usually means "App/Save Charset")
	void Save(agi::fs::path const& file, std::string const& encoding="");

	/// Close the currently open file (i.e. open a new blank file)
	void Close();

	/// If there are unsaved changes, asl the user if they want to save them
	/// @param allow_cancel Let the user cancel the closing
	/// @return wxYES, wxNO or wxCANCEL (note: all three are true in a boolean context)
	int TryToClose(bool allow_cancel = true) const;

	/// @brief Autosave the file if there have been any chances since the last autosave
	/// @return File name used or empty if no save was performed
	agi::fs::path AutoSave();

	/// Can the file be saved in its current format?
	bool CanSave() const;

	DEFINE_SIGNAL_ADDERS(FileOpen, AddFileOpenListener)
	DEFINE_SIGNAL_ADDERS(FileSave, AddFileSaveListener)

	/// @brief Undo the last set of changes to the file
	void Undo();
	/// @brief Redo the last undone changes
	void Redo();
	/// Check if undo stack is empty
	bool IsUndoStackEmpty() const { return undo_stack.size() <= 1; };
	/// Check if redo stack is empty
	bool IsRedoStackEmpty() const { return redo_stack.empty(); };
	/// Get the description of the first undoable change
	wxString GetUndoDescription() const;
	/// Get the description of the first redoable change
	wxString GetRedoDescription() const;
};
