#include "app.h"
#include "pe_icon_loader.h"

#include <cassert>
#include <cstdlib>
#include <string>
#include <vector>
#include <wx/bitmap.h>
#include <wx/choicdlg.h>
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

const wxString APP_NAME = wxT("OpenGothicStarter");
namespace {
constexpr int kSizerExpandAll = static_cast<int>(wxALL) | static_cast<int>(wxEXPAND);

wxString ExpectedOpenGothicBinaryName() {
#if defined(_WIN32)
  return wxT("Gothic2Notr.exe");
#else
  return wxT("Gothic2Notr");
#endif
}
} // namespace

static long ExecuteAsyncCommand(const wxArrayString &command,
                                const wxExecuteEnv &env) {
  std::vector<std::string> argvStorage;
  argvStorage.reserve(command.GetCount());

  for (const wxString &arg : command) {
    const std::string utf8 = arg.ToStdString(wxConvUTF8);
    if (!arg.empty() && utf8.empty()) {
      wxLogError(wxT("Failed to encode command argument for process launch."));
      return 0;
    }

    argvStorage.push_back(utf8);
  }

  std::vector<const char *> argv;
  argv.reserve(argvStorage.size() + 1);
  for (const std::string &arg : argvStorage) {
    argv.push_back(arg.c_str());
  }
  argv.push_back(nullptr);

  return wxExecute(argv.data(), wxEXEC_ASYNC, nullptr, &env);
}

static wxString EscapeAndQuoteForLog(const wxString &arg) {
  wxString escaped;
  escaped.reserve(arg.length() + 2);

  for (wxUniChar ch : arg) {
    if (ch == wxT('\\') || ch == wxT('"')) {
      escaped += wxT('\\');
    }
    escaped += ch;
  }

  return wxString::Format(wxT("\"%s\""), escaped);
}

static wxString BuildCommandLogLine(const wxArrayString &command) {
  wxString rendered;
  for (size_t i = 0; i < command.GetCount(); ++i) {
    if (i > 0) {
      rendered += wxT(" ");
    }
    rendered += EscapeAndQuoteForLog(command[i]);
  }
  return rendered;
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

static bool GetResolvedRuntimePaths(const RuntimePaths *&paths, wxString &error) {
  error.clear();
  paths = nullptr;

  OpenGothicStarterApp *app = RequireInvariant(
      dynamic_cast<OpenGothicStarterApp *>(wxTheApp),
      wxT("wxTheApp must be an OpenGothicStarterApp instance."));

  if (!app->runtime_paths_resolved) {
    error = wxT("Runtime paths were not initialized.");
    return false;
  }

  paths = &app->runtime_paths;
  return true;
}

static wxString GetInstallConfigPath(const RuntimePaths &paths) {
  return wxFileName(paths.system_dir, wxT("OpenGothicStarter.ini")).GetFullPath();
}

static bool DirectoryHasFileCaseInsensitive(const wxString &dirPath,
                                            const wxString &fileName) {
  wxDir dir(dirPath);
  if (!dir.IsOpened()) {
    return false;
  }

  wxString entry;
  bool hasEntry = dir.GetFirst(&entry, wxEmptyString, wxDIR_FILES);
  const wxString needle = fileName.Lower();
  while (hasEntry) {
    if (entry.Lower() == needle) {
      return true;
    }
    hasEntry = dir.GetNext(&entry);
  }

  return false;
}

static bool TryReadStoredGothicVersion(const RuntimePaths &paths,
                                       GothicVersion &version) {
  const wxString configPath = GetInstallConfigPath(paths);
  if (!wxFileName::FileExists(configPath)) {
    return false;
  }

  wxFileConfig cfg(wxEmptyString, wxEmptyString, configPath, wxEmptyString,
                   wxCONFIG_USE_LOCAL_FILE);
  long value = -1;
  if (!cfg.Read(wxT("GENERAL/gothicVersion"), &value)) {
    return false;
  }

  if (value < 0 || value > 2) {
    return false;
  }

  version = static_cast<GothicVersion>(value);
  return true;
}

static bool WriteStoredGothicVersion(const RuntimePaths &paths,
                                     GothicVersion version) {
  const long rawVersion = static_cast<long>(version);
  if (rawVersion < 0 || rawVersion > 2) {
    return false;
  }

  const wxString configPath = GetInstallConfigPath(paths);
  wxFileConfig cfg(wxEmptyString, wxEmptyString, configPath, wxEmptyString,
                   wxCONFIG_USE_LOCAL_FILE);
  cfg.Write(wxT("GENERAL/gothicVersion"), rawVersion);
  return cfg.Flush();
}

static GothicVersion DetectGothicVersion(const RuntimePaths &paths) {
  const wxString dataDir = wxFileName(paths.gothic_root, wxT("Data")).GetFullPath();
  const wxString systemDir = paths.system_dir;

  const wxString addonMarkers[] = {
      wxT("Worlds_Addon.vdf"), wxT("Anims_Addon.vdf"), wxT("Meshes_Addon.vdf"),
      wxT("Textures_Addon.vdf"), wxT("Sounds_Addon.vdf")};

  for (const wxString &marker : addonMarkers) {
    if (DirectoryHasFileCaseInsensitive(dataDir, marker)) {
      return GothicVersion::Gothic2Notr;
    }
  }

  if (DirectoryHasFileCaseInsensitive(systemDir, wxT("Gothic2.exe"))) {
    return GothicVersion::Gothic2Classic;
  }

  if (DirectoryHasFileCaseInsensitive(systemDir, wxT("GOTHIC.EXE")) ||
      DirectoryHasFileCaseInsensitive(systemDir, wxT("GothicMod.exe"))) {
    return GothicVersion::Gothic1;
  }

  return GothicVersion::Unknown;
}

static wxString GothicVersionLabel(GothicVersion version) {
  switch (version) {
  case GothicVersion::Unknown:
    return wxT("Unknown");
  case GothicVersion::Gothic1:
    return wxT("Gothic 1");
  case GothicVersion::Gothic2Classic:
    return wxT("Gothic 2 Classic");
  case GothicVersion::Gothic2Notr:
    return wxT("Gothic 2 Night of the Raven");
  default:
    return wxT("Unknown");
  }
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
  main_sizer->Add(list_ctrl, 1, kSizerExpandAll, 5);

  wxBoxSizer *side_sizer = new wxBoxSizer(wxVERTICAL);
  side_sizer->SetMinSize(wxSize(200, -1));

  button_start = new wxButton(this, wxID_ANY, wxT("Start Game"));
  button_start->Enable(false);

  side_sizer->AddSpacer(5);
  side_sizer->Add(button_start, 0, kSizerExpandAll);

  check_orig = new wxCheckBox(this, wxID_ANY, wxT("Start game without mods"));
  check_window = new wxCheckBox(this, wxID_ANY, wxT("Window mode"));
  check_marvin = new wxCheckBox(this, wxID_ANY, wxT("Marvin mode"));
#if defined(_WIN32)
  check_dx12 = new wxCheckBox(this, wxID_ANY, wxT("Force DirectX 12"));
#endif
  check_rt = new wxCheckBox(this, wxID_ANY, wxT("Ray tracing"));
  check_rti = new wxCheckBox(this, wxID_ANY, wxT("Global illumination"));
  check_meshlets = new wxCheckBox(this, wxID_ANY, wxT("Meshlets"));
  check_vsm = new wxCheckBox(this, wxID_ANY, wxT("Virtual Shadowmap"));
  check_bench = new wxCheckBox(this, wxID_ANY, wxT("Benchmark"));

  field_fxaa = new wxStaticText(this, wxID_ANY, wxT("Anti-Aliasing:"));
  value_fxaa = new wxStaticText(this, wxID_ANY, wxT(""));
  slide_fxaa = new wxSlider(this, wxID_ANY, 0, 0, 2);

  wxBoxSizer *fxaa_sizer = new wxBoxSizer(wxHORIZONTAL);
  fxaa_sizer->Add(field_fxaa);
  fxaa_sizer->AddStretchSpacer();
  fxaa_sizer->Add(value_fxaa);

  side_sizer->AddSpacer(3);
  side_sizer->Add(check_orig, 0, kSizerExpandAll);
  side_sizer->Add(check_window, 0, kSizerExpandAll);
  side_sizer->Add(check_marvin, 0, kSizerExpandAll);
#if defined(_WIN32)
  side_sizer->Add(check_dx12, 0, kSizerExpandAll);
#endif
  side_sizer->Add(check_rt, 0, kSizerExpandAll);
  side_sizer->Add(check_rti, 0, kSizerExpandAll);
  side_sizer->Add(check_meshlets, 0, kSizerExpandAll);
  side_sizer->Add(check_vsm, 0, kSizerExpandAll);
  side_sizer->Add(check_bench, 0, kSizerExpandAll);
  side_sizer->AddSpacer(3);
  side_sizer->Add(fxaa_sizer, 0, kSizerExpandAll);
  side_sizer->Add(slide_fxaa, 0, kSizerExpandAll);

  main_sizer->Add(side_sizer);
  SetSizer(main_sizer);

  Bind(wxEVT_SIZE, &MainPanel::OnSize, this);
  list_ctrl->Bind(wxEVT_LIST_ITEM_SELECTED, &MainPanel::OnSelected, this);
  list_ctrl->Bind(wxEVT_LIST_ITEM_DESELECTED, &MainPanel::OnSelected, this);
  list_ctrl->Bind(wxEVT_LIST_ITEM_ACTIVATED,
                  [this](wxListEvent &) { DoStart(); });
  button_start->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { DoStart(); });
  check_orig->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent &) { DoOrigin(); });

  auto bindParamToggle = [this](wxCheckBox *box) {
    box->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent &) { SaveParams(); });
  };

  bindParamToggle(check_window);
  bindParamToggle(check_marvin);
  bindParamToggle(check_rt);
  bindParamToggle(check_rti);
  bindParamToggle(check_meshlets);
  bindParamToggle(check_vsm);
  bindParamToggle(check_bench);
#if defined(_WIN32)
  bindParamToggle(check_dx12);
#endif
  slide_fxaa->Bind(wxEVT_SLIDER, &MainPanel::OnFXAAScroll, this);
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
      const long row = static_cast<long>(i);
      const int imageIndex = static_cast<int>(i);
      list_ctrl->InsertItem(row, games[i].title, imageIndex);
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
  config->Write(wxT("PARAMS/FXAA"), static_cast<int>(slide_fxaa->GetValue()));
  config->Flush();
}

void MainPanel::OnFXAAScroll(wxCommandEvent &) {
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

int MainPanel::GetSelectedGameIndex() const {
  const long selected = list_ctrl->GetFirstSelected();
  if (selected < 0 || selected >= static_cast<long>(games.size())) {
    return -1;
  }
  return static_cast<int>(selected);
}

bool MainPanel::BuildLaunchCommand(const RuntimePaths &paths, int gameidx,
                                   wxArrayString &command,
                                   wxString &error) const {
  command.clear();
  error.clear();

  command.Add(paths.open_gothic_executable);
  command.Add(wxT("-g"));
  command.Add(paths.gothic_root);

  OpenGothicStarterApp *app = RequireInvariant(
      dynamic_cast<OpenGothicStarterApp *>(wxTheApp),
      wxT("wxTheApp must be an OpenGothicStarterApp instance."));
  switch (app->gothic_version) {
  case GothicVersion::Unknown:
    error = wxT("Stored Gothic version is invalid. Please restart and select a valid version.");
    return false;
  case GothicVersion::Gothic1:
    command.Add(wxT("-g1"));
    break;
  case GothicVersion::Gothic2Classic:
    command.Add(wxT("-g2c"));
    break;
  case GothicVersion::Gothic2Notr:
    command.Add(wxT("-g2"));
    break;
  default:
    error = wxT("Stored Gothic version is invalid. Please restart and select a valid version.");
    return false;
  }

  if (gameidx >= 0) {
    const size_t selectedIndex = static_cast<size_t>(gameidx);
    if (selectedIndex < games.size()) {
      command.Add(wxT("-game:") + games[selectedIndex].file);
    }
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

  return true;
}

wxString MainPanel::ResolveWorkingDirectory(const RuntimePaths &paths,
                                            int gameidx) const {
  if (gameidx >= 0) {
    const size_t selectedIndex = static_cast<size_t>(gameidx);
    if (selectedIndex < games.size()) {
      return games[selectedIndex].datadir;
    }
  }
  return GetDefaultWorkingDirectory(paths);
}

bool MainPanel::EnsureWorkingDirectoryExists(const wxString &path,
                                             wxString &error) const {
  error.clear();
  if (wxDir::Exists(path)) {
    return true;
  }

  if (!wxFileName::Mkdir(path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
    error = wxString::Format(wxT("Failed to create working directory: %s"), path);
    return false;
  }

  return true;
}

void MainPanel::DoStart() {
  const RuntimePaths *paths = nullptr;
  wxString pathError;
  if (!GetResolvedRuntimePaths(paths, pathError)) {
    wxMessageBox(pathError, wxT("Configuration Error"), wxOK | wxICON_ERROR);
    return;
  }
  if (!ValidateRuntimePaths(*paths, pathError)) {
    wxLogWarning(wxT("Runtime path validation failed before launch: %s"), pathError);
    const wxString expectedLauncher =
        wxFileName(paths->system_dir, wxFileName(paths->launcher_executable).GetFullName())
            .GetFullPath();
    const wxString expectedOpenGothic =
        wxFileName(paths->system_dir, ExpectedOpenGothicBinaryName()).GetFullPath();
    wxMessageBox(wxString::Format(
                     wxT("Cannot start game because the Gothic runtime layout is invalid.\n\n"
                         "Expected files in the same directory:\n"
                         "- %s\n"
                         "- %s\n\n"
                         "Fix the installation layout and try again."),
                     expectedLauncher, expectedOpenGothic),
                 wxT("Configuration Error"), wxOK | wxICON_ERROR);
    return;
  }

  const int gameidx = GetSelectedGameIndex();
  wxArrayString command;
  wxString commandError;
  if (!BuildLaunchCommand(*paths, gameidx, command, commandError)) {
    wxMessageBox(commandError, wxT("Configuration Error"), wxOK | wxICON_ERROR);
    return;
  }

  wxExecuteEnv env;
  env.cwd = ResolveWorkingDirectory(*paths, gameidx);
  wxString directoryError;
  if (!EnsureWorkingDirectoryExists(env.cwd, directoryError)) {
    wxMessageBox(directoryError, wxT("Configuration Error"), wxOK | wxICON_ERROR);
    return;
  }

  wxLogMessage(wxT("Starting game command: %s"), BuildCommandLogLine(command));
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

  const RuntimePaths *paths = nullptr;
  wxString pathError;
  if (!GetResolvedRuntimePaths(paths, pathError)) {
    wxLogWarning(wxT("Skipping mod discovery: %s"), pathError);
    return gamesList;
  }
  if (!wxDir::Exists(paths->gothic_root) || !wxDir::Exists(paths->system_dir)) {
    wxLogWarning(wxT("Skipping mod discovery due to invalid runtime directories."));
    return gamesList;
  }

  wxString systemDir = paths->system_dir;
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

      wxString modName = wxFileName(iniName).GetName();
      entry.datadir = GetModWorkingDirectory(*paths, modName);

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
    wxMessageBox(wxString::Format(
                     wxT("OpenGothicStarter must be started from '<Gothic>/system'.\n\n"
                         "Current executable path:\n%s\n\n"
                         "Expected companion binary in that directory:\n%s"),
                     wxStandardPaths::Get().GetExecutablePath(),
                     ExpectedOpenGothicBinaryName()),
                 wxT("Invalid Launcher Location"), wxOK | wxICON_ERROR);
    return false;
  }

  runtime_paths = detectedPaths;
  runtime_paths_resolved = true;

  wxString validationError;
  if (!ValidateRuntimePaths(runtime_paths, validationError)) {
    wxLogWarning(wxT("Runtime path validation failed: %s"), validationError);
    const wxString expectedOpenGothic =
        wxFileName(runtime_paths.system_dir, ExpectedOpenGothicBinaryName()).GetFullPath();
    wxMessageBox(wxString::Format(
                     wxT("OpenGothic binary was not found.\n\n"
                         "Checked directory:\n%s\n\n"
                         "Expected file:\n%s"),
                     runtime_paths.system_dir, expectedOpenGothic),
                 wxT("OpenGothic Not Found"), wxOK | wxICON_ERROR);
    return false;
  }

  wxLogMessage(wxT("Launcher executable: %s"), runtime_paths.launcher_executable);
  wxLogMessage(wxT("Detected Gothic root: %s"), runtime_paths.gothic_root);
  wxLogMessage(wxT("Detected system directory: %s"), runtime_paths.system_dir);
  wxLogMessage(wxT("Detected saves directory: %s"), runtime_paths.saves_dir);
  if (!runtime_paths.open_gothic_executable.empty()) {
    wxLogMessage(wxT("Detected OpenGothic executable: %s"),
                 runtime_paths.open_gothic_executable);
  }

  if (!InitGothicVersion()) {
    return false;
  }

  new MainFrame();
  return true;
}

bool OpenGothicStarterApp::InitConfig() {
  wxStandardPaths::Get().SetFileLayout(wxStandardPaths::FileLayout_XDG);
  wxStandardPaths &path = wxStandardPaths::Get();
  const wxString configPath =
      wxFileName(path.GetUserConfigDir(), APP_NAME).GetFullPath();

  if (!wxDir::Exists(configPath) &&
      !wxFileName::Mkdir(configPath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
    wxLogError(wxT("Failed to create config directory: %s"), configPath);
    return false;
  }

  wxString configFile =
      wxFileName(configPath, APP_NAME.Lower() + wxT(".ini")).GetFullPath();
  wxFileConfig::Set(new wxFileConfig(APP_NAME, wxEmptyString, configFile));
  return true;
}

bool OpenGothicStarterApp::InitGothicVersion() {
  if (!runtime_paths_resolved) {
    return false;
  }

  GothicVersion storedVersion = GothicVersion::Unknown;
  if (TryReadStoredGothicVersion(runtime_paths, storedVersion)) {
    gothic_version = storedVersion;
    wxLogMessage(wxT("Using stored Gothic version: %s"),
                 GothicVersionLabel(gothic_version));
    return true;
  }

  const GothicVersion detectedVersion = DetectGothicVersion(runtime_paths);
  if (detectedVersion != GothicVersion::Unknown) {
    gothic_version = detectedVersion;
    if (!WriteStoredGothicVersion(runtime_paths, gothic_version)) {
      wxLogWarning(wxT("Failed to persist detected Gothic version to %s"),
                   GetInstallConfigPath(runtime_paths));
    }
    wxLogMessage(wxT("Detected Gothic version: %s"),
                 GothicVersionLabel(gothic_version));
    return true;
  }

  const wxString configPath = GetInstallConfigPath(runtime_paths);
  wxArrayString choices;
  choices.Add(wxT("Gothic 1"));
  choices.Add(wxT("Gothic 2 Classic"));
  choices.Add(wxT("Gothic 2 Night of the Raven"));

  wxSingleChoiceDialog dialog(
      nullptr,
      wxString::Format(
          wxT("Gothic version could not be detected automatically.\n"
              "Please choose the version for this installation.\n\n"
              "The selection will be saved in:\n%s"),
          configPath),
      wxT("Select Gothic Version"), choices);
  dialog.SetSelection(2);
  if (dialog.ShowModal() != wxID_OK) {
    wxLogWarning(wxT("Gothic version selection was canceled by user."));
    return false;
  }

  const int selection = dialog.GetSelection();
  if (selection < 0 || selection > 2) {
    wxMessageBox(wxT("Selected Gothic version is invalid."), wxT("Configuration Error"),
                 wxOK | wxICON_ERROR);
    return false;
  }
  gothic_version = static_cast<GothicVersion>(selection);

  if (!WriteStoredGothicVersion(runtime_paths, gothic_version)) {
    wxLogWarning(wxT("Failed to persist selected Gothic version to %s"), configPath);
  }
  wxLogMessage(wxT("Selected Gothic version: %s"),
               GothicVersionLabel(gothic_version));
  return true;
}

wxIMPLEMENT_APP(OpenGothicStarterApp);
