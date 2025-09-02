#include <vector>
#include <wx/bitmap.h>
#include <wx/config.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/filepicker.h>
#include <wx/icon.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/log.h>
#include <wx/process.h>
#include <wx/slider.h>
#include <wx/stdpaths.h>
#include <wx/textfile.h>
#include <wx/wx.h>

const wxString APP_NAME = wxT("OpenGothicStarter");
const wxString APP_VERSION = wxT("0.0.1");

const wxChar *versions[] = {wxT("Gothic 1"), wxT("Gothic 2 Classic"),
                            wxT("Gothic 2 Night of the Raven")};

const wxArrayString APP_GAME_VERSIONS(WXSIZEOF(versions), versions);

struct GameEntry {
  wxString file;
  wxString title;
  wxString authors;
  wxString webpage;
  wxString icon;
  wxString datadir;
};

class SettingsDialog : public wxDialog {
public:
  SettingsDialog(wxWindow *parent);

private:
  void OnCancel(wxCommandEvent &);
  void OnSave(wxCommandEvent &);
  void AddSetting(const wxString &label, wxWindow *ctrl);

  wxPanel *panel;
  wxBoxSizer *sizer;
  wxFilePickerCtrl *ogpath;
  wxDirPickerCtrl *gamepath;
  wxChoice *gameversion;

  wxDECLARE_EVENT_TABLE();
};

class MainPanel : public wxPanel {
public:
  MainPanel(wxWindow *parent);
  void Populate();
  void OnOrigin(wxCommandEvent &);
  void DoOrigin();

private:
  void InitWidgets();
  void OnSize(wxSizeEvent &event);
  void OnSelected(wxListEvent &);
  void OnStart(wxListEvent &);
  void OnStart(wxCommandEvent &);
  void OnParams(wxCommandEvent &);
  void OnFXAAScroll(wxScrollEvent &);
  void DoStart();
  void SaveParams();
  void LoadParams();
  std::vector<GameEntry> InitGames();

  wxListView *list_ctrl;
  wxButton *button_start;
  wxButton *button_settings;
  wxCheckBox *check_orig;
  wxCheckBox *check_window;
  wxCheckBox *check_marvin;
  wxCheckBox *check_rt;
  wxCheckBox *check_rti;
  wxCheckBox *check_meshlets;
  wxCheckBox *check_vsm;
  wxStaticText *field_fxaa;
  wxStaticText *value_fxaa;
  wxSlider *slide_fxaa;

  std::vector<GameEntry> games;

  wxDECLARE_EVENT_TABLE();
};

class MainFrame : public wxFrame {
public:
  MainFrame();
  void OnSettings(wxCommandEvent &);

  MainPanel *panel;

private:
  wxDECLARE_EVENT_TABLE();
};

class OpenGothicStarterApp : public wxApp {
public:
  virtual bool OnInit() override;

  wxString config_path;

private:
  void InitConfig();
};

wxBEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
    EVT_BUTTON(wxID_CANCEL, SettingsDialog::OnCancel)
    EVT_BUTTON(wxID_OK, SettingsDialog::OnSave)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(MainPanel, wxPanel)
  EVT_SIZE(MainPanel::OnSize)
  EVT_LIST_ITEM_SELECTED(wxID_ANY, MainPanel::OnSelected)
  EVT_LIST_ITEM_DESELECTED(wxID_ANY, MainPanel::OnSelected)
  EVT_LIST_ITEM_ACTIVATED(wxID_ANY, MainPanel::OnStart)
  EVT_BUTTON(1001, MainPanel::OnStart)
  EVT_CHECKBOX(2001, MainPanel::OnOrigin)
  EVT_CHECKBOX(2002, MainPanel::OnParams)
  EVT_CHECKBOX(2003, MainPanel::OnParams)
  EVT_CHECKBOX(2004, MainPanel::OnParams)
  EVT_CHECKBOX(2005, MainPanel::OnParams)
  EVT_CHECKBOX(2006, MainPanel::OnParams)
  EVT_CHECKBOX(2007, MainPanel::OnParams)
  EVT_SCROLL(MainPanel::OnFXAAScroll)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
  EVT_BUTTON(1002, MainFrame::OnSettings)
wxEND_EVENT_TABLE()

SettingsDialog::SettingsDialog(wxWindow *parent)
    : wxDialog(parent, wxID_ANY, wxT("Settings"), wxDefaultPosition,
               wxSize(600, -1), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {

  panel = new wxPanel(this);
  sizer = new wxBoxSizer(wxVERTICAL);

  auto *config = wxConfigBase::Get();

  ogpath = new wxFilePickerCtrl(panel, wxID_ANY);
  wxString ogPath;
  config->Read(wxT("GENERAL/openGothicPath"), &ogPath, wxT(""));
  ogpath->SetPath(ogPath);
  AddSetting(wxT("OpenGothic executable:"), ogpath);

  gamepath = new wxDirPickerCtrl(panel, wxID_ANY);
  wxString gothicPath;
  config->Read(wxT("GENERAL/gothicPath"), &gothicPath, wxT(""));
  gamepath->SetPath(gothicPath);
  AddSetting(wxT("Gothic directory:"), gamepath);

  gameversion = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             APP_GAME_VERSIONS);
  int version;
  config->Read(wxT("GENERAL/gothicVersion"), &version, 2L);
  gameversion->SetSelection(version);
  AddSetting(wxT("Game version:"), gameversion);

  wxBoxSizer *btn_sizer = new wxBoxSizer(wxHORIZONTAL);
  wxButton *button_cancel = new wxButton(panel, wxID_CANCEL, wxT("Cancel"));
  wxButton *button_save = new wxButton(panel, wxID_OK, wxT("✓ Apply"));

  btn_sizer->AddStretchSpacer();
  btn_sizer->Add(button_cancel);
  btn_sizer->AddSpacer(5);
  btn_sizer->Add(button_save);
  btn_sizer->AddSpacer(5);

  sizer->AddStretchSpacer();
  sizer->Add(btn_sizer, 0, wxEXPAND);

  panel->SetSizerAndFit(sizer);
}

void SettingsDialog::OnCancel(wxCommandEvent &) { EndModal(wxID_CANCEL); }

void SettingsDialog::OnSave(wxCommandEvent &) {
  auto *config = wxConfigBase::Get();

  config->Write(wxT("GENERAL/gothicPath"), gamepath->GetPath());
  config->Write(wxT("GENERAL/openGothicPath"), ogpath->GetPath());
  config->Write(wxT("GENERAL/gothicVersion"),
                (int)gameversion->GetSelection());
  config->Flush();

  MainFrame *mainFrame = static_cast<MainFrame *>(GetParent());
  mainFrame->panel->Populate();
  mainFrame->panel->DoOrigin();

  EndModal(wxID_OK);
}

void SettingsDialog::AddSetting(const wxString &label, wxWindow *ctrl) {
  wxBoxSizer *row_sizer = new wxBoxSizer(wxHORIZONTAL);
  wxStaticText *labelCtrl = new wxStaticText(
      panel, wxID_ANY, label, wxDefaultPosition, wxSize(200, -1));

  row_sizer->AddSpacer(5);
  row_sizer->Add(labelCtrl, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
  row_sizer->Add(ctrl, 1, wxALL | wxEXPAND, 5);
  row_sizer->AddSpacer(5);
  sizer->Add(row_sizer, 0, wxEXPAND);
}

MainPanel::MainPanel(wxWindow *parent) : wxPanel(parent) {
  InitWidgets();
  Populate();
}

void MainPanel::InitWidgets() {
  wxBoxSizer *main_sizer = new wxBoxSizer(wxHORIZONTAL);

  list_ctrl = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL);
  list_ctrl->InsertColumn(0, wxT(""));
  main_sizer->Add(list_ctrl, 1, wxALL | wxEXPAND, 5);

  wxBoxSizer *side_sizer = new wxBoxSizer(wxVERTICAL);
  side_sizer->SetMinSize(wxSize(200, -1));

  button_start = new wxButton(this, 1001, wxT("Start Game"));
  button_start->Enable(false);
  button_settings = new wxButton(this, 1002, wxT("Settings"));

  side_sizer->AddSpacer(5);
  side_sizer->Add(button_start, 0, wxALL | wxEXPAND);
  side_sizer->AddSpacer(3);
  side_sizer->Add(button_settings, 0, wxALL | wxEXPAND);

  check_orig = new wxCheckBox(this, 2001, wxT("Start game without mods"));
  check_window = new wxCheckBox(this, 2002, wxT("Window mode"));
  check_marvin = new wxCheckBox(this, 2003, wxT("Marvin mode"));
  check_rt = new wxCheckBox(this, 2004, wxT("Ray tracing"));
  check_rti = new wxCheckBox(this, 2005, wxT("Global illumination"));
  check_meshlets = new wxCheckBox(this, 2006, wxT("Meshlets"));
  check_vsm = new wxCheckBox(this, 2007, wxT("Virtual Shadowmap"));

  field_fxaa = new wxStaticText(this, wxID_ANY, wxT("FXAA:"));
  value_fxaa = new wxStaticText(this, wxID_ANY, wxT(""));
  slide_fxaa = new wxSlider(this, wxID_ANY, 0, 0, 5);

  wxBoxSizer *fxaa_sizer = new wxBoxSizer(wxHORIZONTAL);
  fxaa_sizer->Add(field_fxaa);
  fxaa_sizer->AddStretchSpacer();
  fxaa_sizer->Add(value_fxaa);

  side_sizer->AddSpacer(3);
  side_sizer->Add(check_orig, 0, wxALL | wxEXPAND);
  side_sizer->Add(check_window, 0, wxALL | wxEXPAND);
  side_sizer->Add(check_marvin, 0, wxALL | wxEXPAND);
  side_sizer->Add(check_rt, 0, wxALL | wxEXPAND);
  side_sizer->Add(check_rti, 0, wxALL | wxEXPAND);
  side_sizer->Add(check_meshlets, 0, wxALL | wxEXPAND);
  side_sizer->Add(check_vsm, 0, wxALL | wxEXPAND);
  side_sizer->AddSpacer(3);
  side_sizer->Add(fxaa_sizer, 0, wxALL | wxEXPAND);
  side_sizer->Add(slide_fxaa, 0, wxALL | wxEXPAND);

  main_sizer->Add(side_sizer);
  SetSizer(main_sizer);
}

void MainPanel::Populate() {
  games = InitGames();
  list_ctrl->DeleteAllItems();

  if (!games.empty()) {
    wxImageList *imageList = new wxImageList(32, 32);

    for (size_t i = 0; i < games.size(); i++) {
      wxIcon icon;
      if (wxFileName::FileExists(games[i].icon)) {
        wxBitmap bmp(games[i].icon, wxBITMAP_TYPE_ICO);
        if (bmp.IsOk()) {
          wxImage img = bmp.ConvertToImage();
          img = img.Scale(32, 32);
          bmp = wxBitmap(img);
          icon.CopyFromBitmap(bmp);
        }
      }
      if (!icon.IsOk()) {
        wxBitmap bmp(32, 32);
        icon.CopyFromBitmap(bmp);
      }

      imageList->Add(icon);
      list_ctrl->InsertItem(i, games[i].title, i);
    }

    list_ctrl->AssignImageList(imageList, wxIMAGE_LIST_SMALL);
  }

  LoadParams();
}

void MainPanel::LoadParams() {
  auto *config = wxConfigBase::Get();

  bool windowMode;
  config->Read(wxT("PARAMS/windowMode"), &windowMode, false);
  check_window->SetValue(windowMode);

  bool marvin;
  config->Read(wxT("PARAMS/marvin"), &marvin, false);
  check_marvin->SetValue(marvin);

  bool rayTracing;
  config->Read(wxT("PARAMS/rayTracing"), &rayTracing, false);
  check_rt->SetValue(rayTracing);

  bool illumination;
  config->Read(wxT("PARAMS/illumination"), &illumination, false);
  check_rti->SetValue(illumination);

  bool meshlets;
  config->Read(wxT("PARAMS/meshlets"), &meshlets, false);
  check_meshlets->SetValue(meshlets);

  bool vsm;
  config->Read(wxT("PARAMS/vsm"), &vsm, false);
  check_vsm->SetValue(vsm);

  int fxaa;
  config->Read(wxT("PARAMS/FXAA"), &fxaa, 0L);
  slide_fxaa->SetValue(fxaa);

  if (fxaa == 0) {
    value_fxaa->SetLabel(wxT("none"));
  } else {
    value_fxaa->SetLabel(wxString::Format(wxT("%ld"), fxaa));
  }
}

void MainPanel::SaveParams() {
  auto *config = wxConfigBase::Get();

  config->Write(wxT("PARAMS/windowMode"), check_window->GetValue());
  config->Write(wxT("PARAMS/marvin"), check_marvin->GetValue());
  config->Write(wxT("PARAMS/rayTracing"), check_rt->GetValue());
  config->Write(wxT("PARAMS/illumination"), check_rti->GetValue());
  config->Write(wxT("PARAMS/meshlets"), check_meshlets->GetValue());
  config->Write(wxT("PARAMS/vsm"), check_vsm->GetValue());
  config->Write(wxT("PARAMS/FXAA"), (int)slide_fxaa->GetValue());
  config->Flush();
}

void MainPanel::OnParams(wxCommandEvent &) { SaveParams(); }

void MainPanel::OnFXAAScroll(wxScrollEvent &) {
  int fxaa_value = slide_fxaa->GetValue();
  if (fxaa_value == 0) {
    value_fxaa->SetLabel(wxT("none"));
  } else {
    value_fxaa->SetLabel(wxString::Format(wxT("%d"), fxaa_value));
  }
  SaveParams();
}

void MainPanel::OnSelected(wxListEvent &) {
  button_start->Enable(list_ctrl->GetFirstSelected() != -1);
}
void MainPanel::DoOrigin() {
  bool state = check_orig->GetValue();
  if (state) {
    list_ctrl->DeleteAllItems();
  } else {
    Populate();
  }
  button_start->Enable(state);
}

void MainPanel::OnOrigin(wxCommandEvent &) { DoOrigin(); }

void MainPanel::OnStart(wxListEvent &) { DoStart(); }

void MainPanel::OnStart(wxCommandEvent &) { DoStart(); }

void MainPanel::DoStart() {
  auto *config = wxConfigBase::Get();

  wxString openGothicPath;
  config->Read(wxT("GENERAL/openGothicPath"), &openGothicPath, wxT(""));

  wxString gothicPath;
  config->Read(wxT("GENERAL/gothicPath"), &gothicPath, wxT(""));

  if (openGothicPath.IsEmpty() || gothicPath.IsEmpty()) {
    wxMessageBox(wxT("Please configure paths in Settings first."),
                 wxT("Configuration Required"), wxOK | wxICON_WARNING);
    return;
  }

  wxArrayString command;
  command.Add(openGothicPath);
  command.Add(wxT("-g"));
  command.Add(gothicPath);

  int version;
  config->Read(wxT("GENERAL/gothicVersion"), &version, 2L);

  if (version == 0) {
    command.Add(wxT("-g1"));
  } else if (version == 1) {
    command.Add(wxT("-g2c"));
  } else if (version == 2) {
    command.Add(wxT("-g2"));
  }

  int gameidx = list_ctrl->GetFirstSelected();
  if (gameidx >= 0 && gameidx < (int)games.size()) {
    command.Add(wxT("-game:") + games[gameidx].file);
  }

  if (check_window->GetValue()) {
    command.Add(wxT("-window"));
  }
  if (check_marvin->GetValue()) {
    command.Add(wxT("-devmode"));
  }

  command.Add(wxT("-rt"));
  command.Add(check_rt->GetValue() ? wxT("1") : wxT("0"));
  command.Add(wxT("-gi"));
  command.Add(check_rti->GetValue() ? wxT("1") : wxT("0"));
  command.Add(wxT("-ms"));
  command.Add(check_meshlets->GetValue() ? wxT("1") : wxT("0"));
  command.Add(wxT("-vsm"));
  command.Add(check_vsm->GetValue() ? wxT("1") : wxT("0"));

  int fxaa = slide_fxaa->GetValue();
  if (fxaa > 0) {
    command.Add(wxT("-aa"));
    command.Add(wxString::Format(wxT("%d"), fxaa));
  }

  wxExecuteEnv env;
  if (gameidx >= 0 && gameidx < (int)games.size()) {
    env.cwd = games[gameidx].datadir;
  } else {
    OpenGothicStarterApp *app = static_cast<OpenGothicStarterApp *>(wxTheApp);
    env.cwd = wxFileName(app->config_path, wxT("data")).GetFullPath();
    env.cwd = wxFileName(env.cwd, wxT("default")).GetFullPath();
  }

  wxLogMessage(wxT("Starting game with command: %s"), wxJoin(command, ' '));
  wxLogMessage(wxT("Working directory: %s"), env.cwd);

  wxExecute(wxJoin(command, ' '), wxEXEC_ASYNC, nullptr, &env);
}

std::vector<GameEntry> MainPanel::InitGames() {
  std::vector<GameEntry> gamesList;
  auto *config = wxConfigBase::Get();

  wxString gothicRoot;
  config->Read("GENERAL/gothicPath", &gothicRoot, "");
  if (gothicRoot.IsEmpty())
    return gamesList;

  wxString systemDir = wxFileName(gothicRoot, "system").GetFullPath();
  if (!wxDir::Exists(systemDir))
    return gamesList;

  wxDir dir(systemDir);
  wxString iniName;
  bool hasFile = dir.GetFirst(&iniName, "*.ini", wxDIR_FILES);

  while (hasFile) {
    wxString iniPath = wxFileName(systemDir, iniName).GetFullPath();

    wxFileConfig cfg(wxEmptyString, wxEmptyString, iniPath, wxEmptyString,
                     wxCONFIG_USE_LOCAL_FILE);

    if (!cfg.HasGroup("/INFO") || !cfg.HasGroup("/FILES")) {
      hasFile = dir.GetNext(&iniName);
      continue;
    }

    cfg.SetPath("/INFO");
    wxString title, authors, webpage, iconKey;
    bool okTitle = cfg.Read("Title", &title);
    bool okIcon = cfg.Read("Icon", &iconKey);
    cfg.Read("Authors", &authors);
    cfg.Read("Webpage", &webpage);

    if (okTitle) {
      GameEntry entry;
      entry.file = iniName;
      entry.title = title;
      entry.authors = authors;
      entry.webpage = webpage;

      if (okIcon && !iconKey.IsEmpty()) {
        entry.icon = wxFileName(systemDir, iconKey).GetFullPath();
      } else {
        entry.icon.Clear();
      }

      OpenGothicStarterApp *app = static_cast<OpenGothicStarterApp *>(wxTheApp);

      wxString dataRoot = wxFileName(app->config_path, "data").GetFullPath();
      wxString modName = wxFileName(iniName).GetName();
      entry.datadir = wxFileName(dataRoot, modName).GetFullPath();

      if (!wxDir::Exists(entry.datadir)) {
        wxFileName::Mkdir(entry.datadir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
      }

      gamesList.push_back(std::move(entry));
    }

    hasFile = dir.GetNext(&iniName);
  }

  return gamesList;
}

void MainPanel::OnSize(wxSizeEvent &event) {
  if (list_ctrl) {
    list_ctrl->SetColumnWidth(0, list_ctrl->GetSize().GetWidth());
  }
  event.Skip();
}

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, APP_NAME, wxDefaultPosition,
              wxSize(550, 400)) {
  panel = new MainPanel(this);
  Show();
}

void MainFrame::OnSettings(wxCommandEvent &) {
  SettingsDialog settings_dialog(this);
  settings_dialog.ShowModal();
}

bool OpenGothicStarterApp::OnInit() {
  InitConfig();

  wxLog::SetActiveTarget(new wxLogStderr());

  new MainFrame();
  return true;
}

void OpenGothicStarterApp::InitConfig() {
  wxStandardPaths::Get().SetFileLayout(wxStandardPaths::FileLayout_XDG);
  wxStandardPaths &path = wxStandardPaths::Get();
  config_path = wxFileName(path.GetUserConfigDir(), APP_NAME).GetFullPath();

  if (!wxDir::Exists(config_path)) {
    wxFileName::Mkdir(config_path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
  }

  wxString defaultDataPath = wxFileName(config_path, wxT("data")).GetFullPath();
  wxString defaultDir =
      wxFileName(defaultDataPath, wxT("default")).GetFullPath();
  if (!wxDir::Exists(defaultDir)) {
    wxFileName::Mkdir(defaultDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
  }

  wxString configFile =
      wxFileName(config_path, APP_NAME.Lower() + wxT(".ini")).GetFullPath();
  wxFileConfig::Set(new wxFileConfig(APP_NAME, wxEmptyString, configFile));
}

wxIMPLEMENT_APP(OpenGothicStarterApp);
