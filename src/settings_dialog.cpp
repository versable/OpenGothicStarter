#include "settings_dialog.h"

#include "app.h"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/stdpaths.h>

namespace {

bool IsSelectableGothicVersionIndex(int index) {
  return index >= 0 && index <= 2;
}

void AddDialogSettingRow(wxBoxSizer *parent, wxWindow *panel,
                         const wxString &label, wxWindow *control) {
  auto *row = new wxBoxSizer(wxHORIZONTAL);
  auto *labelCtrl = new wxStaticText(panel, wxID_ANY, label, wxDefaultPosition,
                                     wxSize(200, -1));
  row->AddSpacer(5);
  row->Add(labelCtrl, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
  row->Add(control, 1, wxALL | wxEXPAND, 5);
  row->AddSpacer(5);
  parent->Add(row, 0, wxEXPAND);
}

} // namespace

SettingsDialog::SettingsDialog(wxWindow *parent, GothicVersion initialVersion,
                               const wxString &initialLanguage)
    : wxDialog(parent, wxID_ANY, _("Settings"), wxDefaultPosition,
               wxSize(600, -1), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
  auto *panel = new wxPanel(this);
  auto *mainSizer = new wxBoxSizer(wxVERTICAL);

  wxArrayString versionChoices;
  versionChoices.Add(_("Gothic 1"));
  versionChoices.Add(_("Gothic 2 Classic"));
  versionChoices.Add(_("Gothic 2 Night of the Raven"));
  version_choice =
      new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                   versionChoices);
  int versionSelection = static_cast<int>(initialVersion);
  if (!IsSelectableGothicVersionIndex(versionSelection)) {
    versionSelection = 2;
  }
  version_choice->SetSelection(versionSelection);
  AddDialogSettingRow(mainSizer, panel, _("Gothic version:"), version_choice);

  wxArrayString languageChoices;
  language_codes.clear();
  languageChoices.Add(_("System default"));
  language_codes.push_back(wxEmptyString);

  wxArrayString lookupPaths;
  wxFileName exeFile(wxStandardPaths::Get().GetExecutablePath());
  exeFile.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
  const wxString exeDir = exeFile.GetPath();
  lookupPaths.Add(wxFileName(exeDir, wxT("locale")).GetFullPath());

  wxFileName installPrefix(exeDir, wxEmptyString);
  installPrefix.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
  if (!installPrefix.GetDirs().empty()) {
    installPrefix.RemoveLastDir(); // bin -> install prefix
    const wxString shareDir =
        wxFileName(installPrefix.GetPath(), wxT("share")).GetFullPath();
    lookupPaths.Add(wxFileName(shareDir, wxT("locale")).GetFullPath());
  }

  wxArrayString detectedCodes;
  for (const wxString &path : lookupPaths) {
    if (!wxDir::Exists(path)) {
      continue;
    }

    wxDir localeDir(path);
    wxString languageCode;
    bool hasDir = localeDir.GetFirst(&languageCode, wxEmptyString, wxDIR_DIRS);
    while (hasDir) {
      const wxString languageDir =
          wxFileName(path, languageCode).GetFullPath();
      const wxString lcMessagesDir =
          wxFileName(languageDir, wxT("LC_MESSAGES")).GetFullPath();
      const wxString catalogPath =
          wxFileName(lcMessagesDir, wxT("opengothicstarter.mo")).GetFullPath();
      if (wxFileName::FileExists(catalogPath) &&
          detectedCodes.Index(languageCode, false) == wxNOT_FOUND) {
        detectedCodes.Add(languageCode);
      }
      hasDir = localeDir.GetNext(&languageCode);
    }
  }

  if (!initialLanguage.empty() &&
      detectedCodes.Index(initialLanguage, false) == wxNOT_FOUND) {
    detectedCodes.Add(initialLanguage);
  }
  detectedCodes.Sort();

  for (const wxString &code : detectedCodes) {
    const wxLanguageInfo *langInfo = wxLocale::FindLanguageInfo(code);
    if (langInfo == nullptr || langInfo->Description.empty()) {
      languageChoices.Add(code);
    } else {
      languageChoices.Add(langInfo->Description);
    }
    language_codes.push_back(code);
  }

  language_choice = new wxChoice(panel, wxID_ANY, wxDefaultPosition,
                                 wxDefaultSize, languageChoices);

  int languageSelection = 0;
  for (size_t i = 0; i < language_codes.size(); ++i) {
    if (language_codes[i].CmpNoCase(initialLanguage) == 0) {
      languageSelection = static_cast<int>(i);
      break;
    }
  }
  language_choice->SetSelection(languageSelection);
  AddDialogSettingRow(mainSizer, panel, _("Language:"), language_choice);

  auto *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
  auto *cancelButton = new wxButton(panel, wxID_CANCEL);
  auto *applyButton = new wxButton(panel, wxID_OK);
  buttonSizer->AddStretchSpacer();
  buttonSizer->Add(cancelButton);
  buttonSizer->AddSpacer(5);
  buttonSizer->Add(applyButton);
  buttonSizer->AddSpacer(5);

  mainSizer->AddStretchSpacer();
  mainSizer->Add(buttonSizer, 0, wxALL | wxEXPAND, 5);
  panel->SetSizerAndFit(mainSizer);

  auto *dialogSizer = new wxBoxSizer(wxVERTICAL);
  dialogSizer->Add(panel, 1, wxEXPAND);
  SetSizerAndFit(dialogSizer);
}

GothicVersion SettingsDialog::GetSelectedVersion() const {
  const int selection = version_choice->GetSelection();
  if (!IsSelectableGothicVersionIndex(selection)) {
    return GothicVersion::Unknown;
  }

  return static_cast<GothicVersion>(selection);
}

wxString SettingsDialog::GetLanguageOverride() const {
  const int selection = language_choice->GetSelection();
  if (selection < 0 ||
      selection >= static_cast<int>(language_codes.size())) {
    return wxEmptyString;
  }
  return language_codes[static_cast<size_t>(selection)];
}
