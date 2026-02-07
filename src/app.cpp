#include "app.h"
#include "pe_icon_loader.h"

#include <cassert>
#include <cstdlib>
#include <string>
#include <vector>
#include <wx/bitmap.h>
#include <wx/config.h>
#include <wx/dir.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/icon.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/log.h>
#include <wx/process.h>
#include <wx/slider.h>
#include <wx/stdpaths.h>
#include <wx/textfile.h>

const wxString APP_NAME = wxT("OpenGothicStarter");
const wxString APP_VERSION = wxT("0.0.1");

const wxChar *versions[] = {wxT("Gothic 1"), wxT("Gothic 2 Classic"),
                            wxT("Gothic 2 Night of the Raven")};

const wxArrayString APP_GAME_VERSIONS(WXSIZEOF(versions), versions);

static long ExecuteAsyncCommand(const wxArrayString &command,
                                const wxExecuteEnv &env) {
  std::vector<std::string> argvStorage;
  argvStorage.reserve(command.GetCount());

  for (const wxString &arg : command) {
    wxCharBuffer utf8 = arg.utf8_str();
    if (!utf8) {
      wxLogError(wxT("Failed to encode command argument for process launch."));
      return 0;
    }

    argvStorage.emplace_back(utf8.data());
  }

  std::vector<const char *> argv;
  argv.reserve(argvStorage.size() + 1);
  for (const std::string &arg : argvStorage) {
    argv.push_back(arg.c_str());
  }
  argv.push_back(nullptr);

  return wxExecute(argv.data(), wxEXEC_ASYNC, nullptr, &env);
}

template <typename T>
static T *RequireInvariant(T *ptr, const wxString &message) {
  assert(ptr != nullptr);
  if (!ptr) {
    wxLogError(wxT("Invariant violation: %s"), message);
    std::abort();
  }
  return ptr;
}

static bool ResolveLaunchPaths(wxString &openGothicPath, wxString &gothicPath,
                               wxString &error) {
  error.clear();
  openGothicPath.clear();
  gothicPath.clear();

  OpenGothicStarterApp *app = RequireInvariant(
      dynamic_cast<OpenGothicStarterApp *>(wxTheApp),
      wxT("wxTheApp must be an OpenGothicStarterApp instance."));

  if (app->runtime_paths_resolved) {
    wxString runtimeError;
    if (ValidateRuntimePaths(app->runtime_paths, runtimeError)) {
      openGothicPath = app->runtime_paths.open_gothic_executable;
      gothicPath = app->runtime_paths.gothic_root;
      return true;
    }

    wxLogWarning(wxT("Runtime paths invalid, falling back to configured paths: %s"),
                 runtimeError);
  }

  auto *config = wxConfigBase::Get();
  config->Read(wxT("GENERAL/openGothicPath"), &openGothicPath, wxT(""));
  config->Read(wxT("GENERAL/gothicPath"), &gothicPath, wxT(""));

  if (openGothicPath.empty() || gothicPath.empty()) {
    error = wxT("Could not resolve paths from runtime location or configured settings.");
    return false;
  }
  if (!wxFileName::FileExists(openGothicPath)) {
    error = wxString::Format(wxT("OpenGothic executable does not exist: %s"),
                             openGothicPath);
    return false;
  }
  if (!wxDir::Exists(gothicPath)) {
    error = wxString::Format(wxT("Gothic directory does not exist: %s"), gothicPath);
    return false;
  }

  return true;
}

static bool ResolveGothicRootForDiscovery(wxString &gothicRoot) {
  gothicRoot.clear();

  OpenGothicStarterApp *app = RequireInvariant(
      dynamic_cast<OpenGothicStarterApp *>(wxTheApp),
      wxT("wxTheApp must be an OpenGothicStarterApp instance."));

  if (app->runtime_paths_resolved) {
    wxString runtimeError;
    if (ValidateRuntimePaths(app->runtime_paths, runtimeError)) {
      gothicRoot = app->runtime_paths.gothic_root;
      return true;
    }
  }

  auto *config = wxConfigBase::Get();
  config->Read(wxT("GENERAL/gothicPath"), &gothicRoot, wxT(""));
  if (gothicRoot.empty()) {
    return false;
  }

  return wxDir::Exists(gothicRoot);
}

// clang-format off
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
  EVT_CHECKBOX(2008, MainPanel::OnParams)
#if defined(_WIN32)
  EVT_CHECKBOX(2009, MainPanel::OnParams)
#endif
  EVT_SCROLL(MainPanel::OnFXAAScroll)
wxEND_EVENT_TABLE()
// clang-format on

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

  side_sizer->AddSpacer(5);
  side_sizer->Add(button_start, 0, wxALL | wxEXPAND);

  check_orig = new wxCheckBox(this, 2001, wxT("Start game without mods"));
  check_window = new wxCheckBox(this, 2002, wxT("Window mode"));
  check_marvin = new wxCheckBox(this, 2003, wxT("Marvin mode"));
#if defined(_WIN32)
  check_dx12 = new wxCheckBox(this, 2009, wxT("Force DirectX 12"));
#endif
  check_rt = new wxCheckBox(this, 2004, wxT("Ray tracing"));
  check_rti = new wxCheckBox(this, 2005, wxT("Global illumination"));
  check_meshlets = new wxCheckBox(this, 2006, wxT("Meshlets"));
  check_vsm = new wxCheckBox(this, 2007, wxT("Virtual Shadowmap"));
  check_bench = new wxCheckBox(this, 2008, wxT("Benchmark"));

  field_fxaa = new wxStaticText(this, wxID_ANY, wxT("Anti-Aliasing:"));
  value_fxaa = new wxStaticText(this, wxID_ANY, wxT(""));
  slide_fxaa = new wxSlider(this, wxID_ANY, 0, 0, 2);

  wxBoxSizer *fxaa_sizer = new wxBoxSizer(wxHORIZONTAL);
  fxaa_sizer->Add(field_fxaa);
  fxaa_sizer->AddStretchSpacer();
  fxaa_sizer->Add(value_fxaa);

  side_sizer->AddSpacer(3);
  side_sizer->Add(check_orig, 0, wxALL | wxEXPAND);
  side_sizer->Add(check_window, 0, wxALL | wxEXPAND);
  side_sizer->Add(check_marvin, 0, wxALL | wxEXPAND);
#if defined(_WIN32)
  side_sizer->Add(check_dx12, 0, wxALL | wxEXPAND);
#endif
  side_sizer->Add(check_rt, 0, wxALL | wxEXPAND);
  side_sizer->Add(check_rti, 0, wxALL | wxEXPAND);
  side_sizer->Add(check_meshlets, 0, wxALL | wxEXPAND);
  side_sizer->Add(check_vsm, 0, wxALL | wxEXPAND);
  side_sizer->Add(check_bench, 0, wxALL | wxEXPAND);
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
        const wxString extension = wxFileName(games[i].icon).GetExt().Lower();
#if defined(OGS_HAVE_PE_PARSE) && !defined(_WIN32)
        if (extension == wxT("exe")) {
          LoadIconFromPeExecutable(games[i].icon, icon);
        } else
#elif !defined(_WIN32)
        if (extension == wxT("exe")) {
          // Unix-like wxWidgets builds cannot decode PE resources directly.
        } else
#endif
        {
          wxBitmap bmp(games[i].icon, wxBITMAP_TYPE_ANY);
          if (bmp.IsOk()) {
            wxImage img = bmp.ConvertToImage();
            img = img.Scale(32, 32);
            bmp = wxBitmap(img);
            icon.CopyFromBitmap(bmp);
          }
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

  bool bench;
  config->Read(wxT("PARAMS/bench"), &bench, false);
  check_bench->SetValue(bench);

#if defined(_WIN32)
  bool dx12;
  config->Read(wxT("PARAMS/dx12"), &dx12, false);
  check_dx12->SetValue(dx12);
#endif

  int fxaa;
  config->Read(wxT("PARAMS/FXAA"), &fxaa, 0L);
  slide_fxaa->SetValue(fxaa);

  if (fxaa == 0) {
    value_fxaa->SetLabel(wxT("none"));
  } else {
    value_fxaa->SetLabel(wxString::Format(wxT("%d"), fxaa));
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
  config->Write(wxT("PARAMS/bench"), check_bench->GetValue());
#if defined(_WIN32)
  config->Write(wxT("PARAMS/dx12"), check_dx12->GetValue());
#endif
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
  wxString gothicPath;
  wxString pathError;
  if (!ResolveLaunchPaths(openGothicPath, gothicPath, pathError)) {
    wxMessageBox(pathError, wxT("Configuration Error"), wxOK | wxICON_ERROR);
    return;
  }

  wxArrayString command;
  command.Add(openGothicPath);
  command.Add(wxT("-g"));
  command.Add(gothicPath);

  int version;
  config->Read(wxT("GENERAL/gothicVersion"), &version, 2L);

  switch (version) {
  case 0:
    command.Add(wxT("-g1"));
    break;
  case 1:
    command.Add(wxT("-g2c"));
    break;
  case 2:
    command.Add(wxT("-g2"));
    break;
  default:
    wxMessageBox(wxT("Configured game version is invalid. Please re-select it in Settings."),
                 wxT("Configuration Error"), wxOK | wxICON_ERROR);
    return;
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
  if (check_bench->GetValue()) {
    command.Add(wxT("-benchmark"));
  }

#if defined(_WIN32)
  if (check_dx12->GetValue()) {
    command.Add(wxT("-dx12"));
  }
#endif

  int fxaa = slide_fxaa->GetValue();
  if (fxaa > 0) {
    command.Add(wxT("-aa"));
    command.Add(wxString::Format(wxT("%d"), fxaa));
  }

  wxExecuteEnv env;
  if (gameidx >= 0 && gameidx < (int)games.size()) {
    env.cwd = games[gameidx].datadir;
  } else {
    env.cwd = wxFileName(gothicPath, wxT("Saves")).GetFullPath();
  }

  if (!wxDir::Exists(env.cwd) &&
      !wxFileName::Mkdir(env.cwd, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
    wxMessageBox(wxString::Format(wxT("Failed to create working directory: %s"),
                                  env.cwd),
                 wxT("Configuration Error"), wxOK | wxICON_ERROR);
    return;
  }

  wxLogMessage(wxT("Starting game with %zu arguments."),
               static_cast<size_t>(command.GetCount()));
  wxLogMessage(wxT("Working directory: %s"), env.cwd);

  const long pid = ExecuteAsyncCommand(command, env);
  if (pid == 0) {
    wxMessageBox(wxT("Failed to start OpenGothic process."),
                 wxT("Launch Failed"), wxOK | wxICON_ERROR);
    return;
  }
}

std::vector<GameEntry> MainPanel::InitGames() {
  std::vector<GameEntry> gamesList;

  wxString gothicRoot;
  if (!ResolveGothicRootForDiscovery(gothicRoot)) {
    return gamesList;
  }

  wxString systemDir = wxFileName(gothicRoot, "system").GetFullPath();
  if (!wxDir::Exists(systemDir))
    return gamesList;

  wxDir dir(systemDir);
  wxString iniName;
  bool hasFile = dir.GetFirst(&iniName, wxEmptyString, wxDIR_FILES);

  while (hasFile) {
    if (wxFileName(iniName).GetExt().Lower() != wxT("ini")) {
      hasFile = dir.GetNext(&iniName);
      continue;
    }

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

      wxString dataRoot = wxFileName(gothicRoot, wxT("Saves")).GetFullPath();
      wxString modName = wxFileName(iniName).GetName();
      entry.datadir = wxFileName(dataRoot, modName).GetFullPath();

      if (!wxDir::Exists(entry.datadir) &&
          !wxFileName::Mkdir(entry.datadir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
        wxLogError(wxT("Failed to create mod data directory: %s"), entry.datadir);
        hasFile = dir.GetNext(&iniName);
        continue;
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

bool OpenGothicStarterApp::OnInit() {
  if (!InitConfig()) {
    return false;
  }

  wxInitAllImageHandlers();
  wxLog::SetActiveTarget(new wxLogStderr());

  wxString resolveError;
  RuntimePaths detectedPaths;
  if (!ResolveRuntimePaths(detectedPaths, resolveError)) {
    wxLogWarning(wxT("Runtime path resolution failed: %s"), resolveError);
  } else {
    runtime_paths = detectedPaths;
    runtime_paths_resolved = true;

    wxString validationError;
    if (!ValidateRuntimePaths(runtime_paths, validationError)) {
      wxLogWarning(wxT("Runtime path validation failed: %s"), validationError);
    }

    wxLogMessage(wxT("Launcher executable: %s"), runtime_paths.launcher_executable);
    wxLogMessage(wxT("Detected Gothic root: %s"), runtime_paths.gothic_root);
    wxLogMessage(wxT("Detected system directory: %s"), runtime_paths.system_dir);
    wxLogMessage(wxT("Detected saves directory: %s"), runtime_paths.saves_dir);
    if (!runtime_paths.open_gothic_executable.empty()) {
      wxLogMessage(wxT("Detected OpenGothic executable: %s"),
                   runtime_paths.open_gothic_executable);
    }
  }

  new MainFrame();
  return true;
}

bool OpenGothicStarterApp::InitConfig() {
  wxStandardPaths::Get().SetFileLayout(wxStandardPaths::FileLayout_XDG);
  wxStandardPaths &path = wxStandardPaths::Get();
  config_path = wxFileName(path.GetUserConfigDir(), APP_NAME).GetFullPath();

  if (!wxDir::Exists(config_path) &&
      !wxFileName::Mkdir(config_path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
    wxLogError(wxT("Failed to create config directory: %s"), config_path);
    return false;
  }

  wxString defaultDataPath = wxFileName(config_path, wxT("data")).GetFullPath();
  wxString defaultDir =
      wxFileName(defaultDataPath, wxT("default")).GetFullPath();
  if (!wxDir::Exists(defaultDir) &&
      !wxFileName::Mkdir(defaultDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
    wxLogError(wxT("Failed to create default data directory: %s"), defaultDir);
    return false;
  }

  wxString configFile =
      wxFileName(config_path, APP_NAME.Lower() + wxT(".ini")).GetFullPath();
  wxFileConfig::Set(new wxFileConfig(APP_NAME, wxEmptyString, configFile));
  return true;
}

wxIMPLEMENT_APP(OpenGothicStarterApp);
