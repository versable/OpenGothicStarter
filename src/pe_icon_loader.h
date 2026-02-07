#pragma once

#include <wx/string.h>

class wxIcon;

bool LoadIconFromPeExecutable(const wxString &path, wxIcon &icon);
