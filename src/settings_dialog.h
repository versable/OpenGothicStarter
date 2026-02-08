#pragma once

#include <vector>
#include <wx/dialog.h>
#include <wx/string.h>

enum class GothicVersion : int;
class wxChoice;

class SettingsDialog : public wxDialog {
public:
  SettingsDialog(wxWindow *parent, GothicVersion initialVersion,
                 const wxString &initialLanguage);

  GothicVersion GetSelectedVersion() const;
  wxString GetLanguageOverride() const;

private:
  wxChoice *version_choice;
  wxChoice *language_choice;
  std::vector<wxString> language_codes;
};
