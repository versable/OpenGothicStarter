#include "settings_dialog.h"

#include "app.h"
#include "embedded_locales.h"

#include <algorithm>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
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
  row->Add(labelCtrl, 0,
           static_cast<int>(wxALL) | static_cast<int>(wxALIGN_CENTER_VERTICAL),
           5);
  row->Add(control, 1,
           static_cast<int>(wxALL) | static_cast<int>(wxEXPAND), 5);
  row->AddSpacer(5);
  parent->Add(row, 0, wxEXPAND);
}

} // namespace

SettingsDialog::SettingsDialog(wxWindow *parent, GothicVersion initialVersion,
                               const wxString &initialLanguage,
                               const SystemPackSettings &initialSystemPack)
    : wxDialog(parent, wxID_ANY, _("Settings"), wxDefaultPosition,
               wxSize(600, -1), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
  auto *panel = new wxPanel(this);
  auto *mainSizer = new wxBoxSizer(wxVERTICAL);
  auto *notebook = new wxNotebook(panel, wxID_ANY);

  auto *generalPanel = new wxPanel(notebook);
  auto *generalSizer = new wxBoxSizer(wxVERTICAL);
  wxArrayString versionChoices;
  versionChoices.Add(_("Gothic 1"));
  versionChoices.Add(_("Gothic 2 Classic"));
  versionChoices.Add(_("Gothic 2 Night of the Raven"));
  version_choice = new wxChoice(generalPanel, wxID_ANY, wxDefaultPosition,
                                wxDefaultSize, versionChoices);
  int versionSelection = static_cast<int>(initialVersion);
  if (!IsSelectableGothicVersionIndex(versionSelection)) {
    versionSelection = 2;
  }
  version_choice->SetSelection(versionSelection);
  AddDialogSettingRow(generalSizer, generalPanel, _("Gothic version:"),
                      version_choice);

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

  wxString bundledLocaleError;
  const wxString bundledLocaleRoot = GetBundledLocaleRoot(bundledLocaleError);
  if (!bundledLocaleRoot.empty()) {
    lookupPaths.Add(bundledLocaleRoot);
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

  language_choice = new wxChoice(generalPanel, wxID_ANY, wxDefaultPosition,
                                 wxDefaultSize, languageChoices);

  int languageSelection = 0;
  for (size_t i = 0; i < language_codes.size(); ++i) {
    if (language_codes[i].CmpNoCase(initialLanguage) == 0) {
      languageSelection = static_cast<int>(i);
      break;
    }
  }
  language_choice->SetSelection(languageSelection);
  AddDialogSettingRow(generalSizer, generalPanel, _("Language:"),
                      language_choice);
  generalSizer->AddStretchSpacer();
  generalPanel->SetSizer(generalSizer);
  notebook->AddPage(generalPanel, _("General"), true);

  auto *systemPackPanel = new wxPanel(notebook);
  auto *systemPackSizer = new wxBoxSizer(wxVERTICAL);

  show_fps_counter_checkbox =
      new wxCheckBox(systemPackPanel, wxID_ANY, wxEmptyString);
  show_fps_counter_checkbox->SetValue(initialSystemPack.show_fps_counter);
  AddDialogSettingRow(systemPackSizer, systemPackPanel, _("Show FPS counter:"),
                      show_fps_counter_checkbox);

  hide_focus_checkbox = new wxCheckBox(systemPackPanel, wxID_ANY, wxEmptyString);
  hide_focus_checkbox->SetValue(initialSystemPack.hide_focus);
  AddDialogSettingRow(systemPackSizer, systemPackPanel, _("Hide focus highlight:"),
                      hide_focus_checkbox);

  vertical_fov_ctrl = new wxSpinCtrlDouble(systemPackPanel, wxID_ANY);
  vertical_fov_ctrl->SetRange(1.0, 179.0);
  vertical_fov_ctrl->SetIncrement(0.5);
  vertical_fov_ctrl->SetDigits(1);
  vertical_fov_ctrl->SetValue(std::clamp(initialSystemPack.vertical_fov, 1.0, 179.0));
  AddDialogSettingRow(systemPackSizer, systemPackPanel, _("Vertical FOV:"),
                      vertical_fov_ctrl);

  fps_limit_ctrl = new wxSpinCtrl(systemPackPanel, wxID_ANY);
  fps_limit_ctrl->SetRange(0, 1000);
  fps_limit_ctrl->SetValue(std::clamp(initialSystemPack.fps_limit, 0, 1000));
  AddDialogSettingRow(systemPackSizer, systemPackPanel, _("FPS limit:"),
                      fps_limit_ctrl);

  interface_scale_ctrl = new wxSpinCtrlDouble(systemPackPanel, wxID_ANY);
  interface_scale_ctrl->SetRange(0.1, 8.0);
  interface_scale_ctrl->SetIncrement(0.1);
  interface_scale_ctrl->SetDigits(1);
  interface_scale_ctrl->SetValue(
      std::clamp(initialSystemPack.interface_scale, 0.1, 8.0));
  AddDialogSettingRow(systemPackSizer, systemPackPanel, _("Interface scale:"),
                      interface_scale_ctrl);

  inventory_cell_size_ctrl = new wxSpinCtrl(systemPackPanel, wxID_ANY);
  inventory_cell_size_ctrl->SetRange(10, 512);
  inventory_cell_size_ctrl->SetValue(
      std::clamp(initialSystemPack.inventory_cell_size, 10, 512));
  AddDialogSettingRow(systemPackSizer, systemPackPanel, _("Inventory cell size:"),
                      inventory_cell_size_ctrl);

  new_chapter_size_x_ctrl = new wxSpinCtrl(systemPackPanel, wxID_ANY);
  new_chapter_size_x_ctrl->SetRange(1, 8192);
  new_chapter_size_x_ctrl->SetValue(
      std::clamp(initialSystemPack.new_chapter_size_x, 1, 8192));
  AddDialogSettingRow(systemPackSizer, systemPackPanel, _("New chapter width:"),
                      new_chapter_size_x_ctrl);

  new_chapter_size_y_ctrl = new wxSpinCtrl(systemPackPanel, wxID_ANY);
  new_chapter_size_y_ctrl->SetRange(1, 8192);
  new_chapter_size_y_ctrl->SetValue(
      std::clamp(initialSystemPack.new_chapter_size_y, 1, 8192));
  AddDialogSettingRow(systemPackSizer, systemPackPanel, _("New chapter height:"),
                      new_chapter_size_y_ctrl);

  save_game_image_size_x_ctrl = new wxSpinCtrl(systemPackPanel, wxID_ANY);
  save_game_image_size_x_ctrl->SetRange(0, 8192);
  save_game_image_size_x_ctrl->SetValue(
      std::clamp(initialSystemPack.save_game_image_size_x, 0, 8192));
  AddDialogSettingRow(systemPackSizer, systemPackPanel, _("Save image width:"),
                      save_game_image_size_x_ctrl);

  save_game_image_size_y_ctrl = new wxSpinCtrl(systemPackPanel, wxID_ANY);
  save_game_image_size_y_ctrl->SetRange(0, 8192);
  save_game_image_size_y_ctrl->SetValue(
      std::clamp(initialSystemPack.save_game_image_size_y, 0, 8192));
  AddDialogSettingRow(systemPackSizer, systemPackPanel, _("Save image height:"),
                      save_game_image_size_y_ctrl);

  show_health_bar_checkbox = new wxCheckBox(systemPackPanel, wxID_ANY, wxEmptyString);
  show_health_bar_checkbox->SetValue(initialSystemPack.show_health_bar);
  AddDialogSettingRow(systemPackSizer, systemPackPanel, _("Show health bar:"),
                      show_health_bar_checkbox);

  wxArrayString barVisibilityChoices;
  barVisibilityChoices.Add(_("Never"));
  barVisibilityChoices.Add(_("Contextual"));
  barVisibilityChoices.Add(_("Always"));

  show_mana_bar_choice =
      new wxChoice(systemPackPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                   barVisibilityChoices);
  show_mana_bar_choice->SetSelection(
      std::clamp(initialSystemPack.show_mana_bar, 0, 2));
  AddDialogSettingRow(systemPackSizer, systemPackPanel, _("Show mana bar:"),
                      show_mana_bar_choice);

  show_swim_bar_choice =
      new wxChoice(systemPackPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                   barVisibilityChoices);
  show_swim_bar_choice->SetSelection(
      std::clamp(initialSystemPack.show_swim_bar, 0, 2));
  AddDialogSettingRow(systemPackSizer, systemPackPanel, _("Show swim bar:"),
                      show_swim_bar_choice);

  systemPackSizer->AddStretchSpacer();
  systemPackPanel->SetSizer(systemPackSizer);
  notebook->AddPage(systemPackPanel, _("SystemPack"), false);

  mainSizer->Add(notebook, 1, wxEXPAND);

  auto *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
  auto *cancelButton = new wxButton(panel, wxID_CANCEL);
  auto *applyButton = new wxButton(panel, wxID_SAVE);
  cancelButton->Bind(wxEVT_BUTTON,
                     [this](wxCommandEvent &) { EndModal(wxID_CANCEL); });
  applyButton->Bind(wxEVT_BUTTON,
                    [this](wxCommandEvent &) { EndModal(wxID_SAVE); });
  buttonSizer->AddStretchSpacer();
  buttonSizer->Add(cancelButton);
  buttonSizer->AddSpacer(5);
  buttonSizer->Add(applyButton);
  buttonSizer->AddSpacer(5);

  mainSizer->Add(buttonSizer, 0,
                 static_cast<int>(wxALL) | static_cast<int>(wxEXPAND), 5);
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

SystemPackSettings SettingsDialog::GetSystemPackSettings() const {
  SystemPackSettings settings;
  settings.show_fps_counter = show_fps_counter_checkbox->GetValue();
  settings.hide_focus = hide_focus_checkbox->GetValue();
  settings.vertical_fov = std::clamp(vertical_fov_ctrl->GetValue(), 1.0, 179.0);
  settings.fps_limit = std::max(0, fps_limit_ctrl->GetValue());
  settings.interface_scale = std::max(0.1, interface_scale_ctrl->GetValue());
  settings.inventory_cell_size = std::max(10, inventory_cell_size_ctrl->GetValue());
  settings.new_chapter_size_x = std::max(1, new_chapter_size_x_ctrl->GetValue());
  settings.new_chapter_size_y = std::max(1, new_chapter_size_y_ctrl->GetValue());
  settings.save_game_image_size_x =
      std::max(0, save_game_image_size_x_ctrl->GetValue());
  settings.save_game_image_size_y =
      std::max(0, save_game_image_size_y_ctrl->GetValue());
  settings.show_health_bar = show_health_bar_checkbox->GetValue();
  settings.show_mana_bar = std::clamp(show_mana_bar_choice->GetSelection(), 0, 2);
  settings.show_swim_bar = std::clamp(show_swim_bar_choice->GetSelection(), 0, 2);
  return settings;
}
