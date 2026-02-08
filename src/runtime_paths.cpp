#include "runtime_paths.h"

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

namespace {

wxString FindOpenGothicExecutable(const wxString &system_dir) {
#if defined(_WIN32)
  static const wxString candidates[] = {wxT("Gothic2Notr.exe")};
#else
  static const wxString candidates[] = {wxT("Gothic2Notr")};
#endif

  for (const wxString &name : candidates) {
    const wxString path = wxFileName(system_dir, name).GetFullPath();
    if (wxFileName::FileExists(path)) {
      return path;
    }
  }

  return wxEmptyString;
}

} // namespace

bool ResolveRuntimePaths(RuntimePaths &paths, wxString &error) {
  error.clear();
  paths = RuntimePaths{};

  paths.launcher_executable = wxStandardPaths::Get().GetExecutablePath();
  if (paths.launcher_executable.empty()) {
    error = wxT("Failed to resolve launcher executable path.");
    return false;
  }

  wxFileName launcher_file(paths.launcher_executable);
  launcher_file.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
  paths.launcher_executable = launcher_file.GetFullPath();
  paths.launcher_dir = launcher_file.GetPath();

  if (!wxDir::Exists(paths.launcher_dir)) {
    error = wxString::Format(wxT("Launcher directory does not exist: %s"),
                             paths.launcher_dir);
    return false;
  }

  wxFileName launcher_dir_name(paths.launcher_dir, wxEmptyString);
  if (launcher_dir_name.GetDirs().empty()) {
    error = wxString::Format(wxT("Failed to inspect launcher directory: %s"),
                             paths.launcher_dir);
    return false;
  }

  const wxString base_dir = launcher_dir_name.GetDirs().back().Lower();
  if (base_dir == wxT("system")) {
    paths.system_dir = paths.launcher_dir;
    launcher_dir_name.RemoveLastDir();
    paths.gothic_root = launcher_dir_name.GetPath();
  } else {
    const wxString nested_system_dir =
        wxFileName(paths.launcher_dir, wxT("system")).GetFullPath();
    if (wxDir::Exists(nested_system_dir)) {
      paths.gothic_root = paths.launcher_dir;
      paths.system_dir = nested_system_dir;
    }
  }

  if (paths.gothic_root.empty() || paths.system_dir.empty()) {
    error = wxString::Format(
        wxT("Unable to derive Gothic root/system from launcher path: %s"),
        paths.launcher_executable);
    return false;
  }

  paths.open_gothic_executable = FindOpenGothicExecutable(paths.system_dir);
  paths.saves_dir = wxFileName(paths.gothic_root, wxT("Saves")).GetFullPath();

  return true;
}

bool ValidateRuntimePaths(const RuntimePaths &paths, wxString &error) {
  error.clear();

  if (paths.launcher_executable.empty() ||
      !wxFileName::FileExists(paths.launcher_executable)) {
    error = wxString::Format(wxT("Launcher executable not found: %s"),
                             paths.launcher_executable);
    return false;
  }

  if (paths.gothic_root.empty() || !wxDir::Exists(paths.gothic_root)) {
    error = wxString::Format(wxT("Gothic root directory not found: %s"),
                             paths.gothic_root);
    return false;
  }

  if (paths.system_dir.empty() || !wxDir::Exists(paths.system_dir)) {
    error = wxString::Format(wxT("System directory not found: %s"),
                             paths.system_dir);
    return false;
  }

  if (paths.open_gothic_executable.empty()) {
    error =
        wxString::Format(wxT("OpenGothic executable was not found in: %s"),
                         paths.system_dir);
    return false;
  }

  if (!wxFileName::FileExists(paths.open_gothic_executable)) {
    error = wxString::Format(wxT("OpenGothic executable does not exist: %s"),
                             paths.open_gothic_executable);
    return false;
  }

  return true;
}

wxString GetDefaultWorkingDirectory(const RuntimePaths &paths) {
  return paths.saves_dir;
}

wxString GetModWorkingDirectory(const RuntimePaths &paths, const wxString &mod_id) {
  if (mod_id.empty()) {
    return GetDefaultWorkingDirectory(paths);
  }

  return wxFileName(paths.saves_dir, mod_id).GetFullPath();
}
