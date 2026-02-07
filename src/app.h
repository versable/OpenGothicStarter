#pragma once

#include <vector>
#include <wx/filepicker.h>
#include <wx/listctrl.h>
#include <wx/wx.h>

extern const wxString APP_NAME;
extern const wxString APP_VERSION;
extern const wxArrayString APP_GAME_VERSIONS;

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
  wxCheckBox *check_bench;
#if defined(_WIN32)
  wxCheckBox *check_dx12;
#endif
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
  bool OnInit() override;

  wxString config_path;

private:
  bool InitConfig();
};
