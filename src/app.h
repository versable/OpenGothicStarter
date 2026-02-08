#pragma once

#include "runtime_paths.h"

#include <memory>
#include <vector>
#include <wx/intl.h>
#include <wx/listctrl.h>
#include <wx/wx.h>

extern const wxString APP_NAME;

enum class GothicVersion : int {
  Unknown = -1,
  Gothic1 = 0,
  Gothic2Classic = 1,
  Gothic2Notr = 2
};

struct GameEntry {
  wxString file;
  wxString title;
  wxString authors;
  wxString webpage;
  wxString icon;
  wxString datadir;
};

class MainPanel : public wxPanel {
public:
  MainPanel(wxWindow *parent);
  void Populate();

private:
  void InitWidgets();
  void OnSize(wxSizeEvent &event);
  void OnSelected(wxListEvent &);
  void OnFXAAScroll(wxCommandEvent &);
  void DoStart();
  void DoOrigin();
  bool BuildLaunchCommand(const RuntimePaths &paths, int gameidx,
                          wxArrayString &command, wxString &error) const;
  wxString ResolveWorkingDirectory(const RuntimePaths &paths, int gameidx) const;
  bool EnsureWorkingDirectoryExists(const wxString &path, wxString &error) const;
  int GetSelectedGameIndex() const;
  void SaveParams();
  void LoadParams();
  std::vector<GameEntry> InitGames();

  wxListView *list_ctrl;
  wxButton *button_start;
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
};

class MainFrame : public wxFrame {
public:
  MainFrame();

  MainPanel *panel;
};

class OpenGothicStarterApp : public wxApp {
public:
  bool OnInit() override;

  RuntimePaths runtime_paths;
  bool runtime_paths_resolved = false;
  GothicVersion gothic_version = GothicVersion::Unknown;

private:
  bool InitConfig();
  bool InitGothicVersion();

  std::unique_ptr<wxLocale> app_locale;
};
