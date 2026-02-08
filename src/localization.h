#pragma once

#include <memory>
#include <wx/string.h>

class wxLocale;

void InitializeLocalization(std::unique_ptr<wxLocale> &app_locale,
                            const wxString &languageOverride = wxString());
