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

wxArrayString BuildGothicVersionChoices() {
  wxArrayString choices;
  choices.Add(_("Gothic 1"));
  choices.Add(_("Gothic 2 Classic"));
  choices.Add(_("Gothic 2 Night of the Raven"));
  return choices;
}

wxArrayString BuildLocalizationLookupPaths() {
  wxArrayString paths;

  wxFileName exeFile(wxStandardPaths::Get().GetExecutablePath());
  exeFile.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
  const wxString exeDir = exeFile.GetPath();

  paths.Add(wxFileName(exeDir, wxT("locale")).GetFullPath());

  wxFileName installPrefix(exeDir, wxEmptyString);
  installPrefix.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
  if (!installPrefix.GetDirs().empty()) {
    installPrefix.RemoveLastDir(); // bin -> install prefix
    const wxString shareDir =
        wxFileName(installPrefix.GetPath(), wxT("share")).GetFullPath();
    paths.Add(wxFileName(shareDir, wxT("locale")).GetFullPath());
  }

  return paths;
}

bool HasLanguageCatalog(const wxString &localeRoot,
                        const wxString &languageCode) {
  const wxString languageDir =
      wxFileName(localeRoot, languageCode).GetFullPath();
  const wxString lcMessagesDir =
      wxFileName(languageDir, wxT("LC_MESSAGES")).GetFullPath();
  const wxString catalogPath =
      wxFileName(lcMessagesDir, wxT("opengothicstarter.mo")).GetFullPath();
  return wxFileName::FileExists(catalogPath);
}

wxArrayString DetectAvailableLanguageCodes() {
  wxArrayString languageCodes;

  const wxArrayString lookupPaths = BuildLocalizationLookupPaths();
  for (const wxString &path : lookupPaths) {
    if (!wxDir::Exists(path)) {
      continue;
    }

    wxDir localeDir(path);
    wxString languageCode;
    bool hasDir = localeDir.GetFirst(&languageCode, wxEmptyString, wxDIR_DIRS);
    while (hasDir) {
      if (HasLanguageCatalog(path, languageCode) &&
          languageCodes.Index(languageCode, false) == wxNOT_FOUND) {
        languageCodes.Add(languageCode);
      }
      hasDir = localeDir.GetNext(&languageCode);
    }
  }

  languageCodes.Sort();
  return languageCodes;
}

wxString DisplayNameForLanguageCode(const wxString &languageCode) {
  const wxLanguageInfo *langInfo = wxLocale::FindLanguageInfo(languageCode);
  if (langInfo == nullptr || langInfo->Description.empty()) {
    return languageCode;
  }

  return langInfo->Description;
}

void BuildLanguageChoices(const wxString &currentLanguage,
                          wxArrayString &displayChoices,
                          std::vector<wxString> &codes) {
  displayChoices.clear();
  codes.clear();

  displayChoices.Add(_("System default"));
  codes.push_back(wxEmptyString);

  wxArrayString detectedCodes = DetectAvailableLanguageCodes();
  if (!currentLanguage.empty() &&
      detectedCodes.Index(currentLanguage, false) == wxNOT_FOUND) {
    detectedCodes.Add(currentLanguage);
  }
  detectedCodes.Sort();

  for (const wxString &code : detectedCodes) {
    displayChoices.Add(DisplayNameForLanguageCode(code));
    codes.push_back(code);
  }
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

  version_choice =
      new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                   BuildGothicVersionChoices());
  int versionSelection = static_cast<int>(initialVersion);
  if (!IsSelectableGothicVersionIndex(versionSelection)) {
    versionSelection = 2;
  }
  version_choice->SetSelection(versionSelection);
  AddDialogSettingRow(mainSizer, panel, _("Gothic version:"), version_choice);

  wxArrayString languageChoices;
  BuildLanguageChoices(initialLanguage, languageChoices, language_codes);
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
