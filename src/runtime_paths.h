#pragma once

#include <wx/string.h>

struct RuntimePaths {
  wxString launcher_executable;
  wxString launcher_dir;
  wxString system_dir;
  wxString gothic_root;
  wxString open_gothic_executable;
  wxString saves_dir;
};

bool ResolveRuntimePaths(RuntimePaths &paths, wxString &error);
bool ValidateRuntimePaths(const RuntimePaths &paths, wxString &error);

wxString GetDefaultWorkingDirectory(const RuntimePaths &paths);
wxString GetModWorkingDirectory(const RuntimePaths &paths, const wxString &mod_id);
