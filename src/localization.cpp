#include "localization.h"

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/stdpaths.h>

namespace {

const wxString kI18nDomain = wxT("opengothicstarter");
bool gLocalizationLookupPathsRegistered = false;

void AddCatalogLookupPathIfExists(const wxString &path) {
  if (!wxDir::Exists(path)) {
    return;
  }

  wxLocale::AddCatalogLookupPathPrefix(path);
  wxLogMessage(wxT("Localization lookup path: %s"), path);
}

wxArrayString BuildLocalizationLookupPaths() {
  wxArrayString paths;

  wxFileName exeFile(wxStandardPaths::Get().GetExecutablePath());
  exeFile.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
  const wxString exeDir = exeFile.GetPath();

  paths.Add(wxFileName(exeDir, wxT("locale")).GetFullPath());

  wxFileName installPrefix(exeDir, wxEmptyString);
  installPrefix.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
  if (!installPrefix.GetDirs().empty()) {
    installPrefix.RemoveLastDir(); // bin -> install prefix
    const wxString shareDir =
        wxFileName(installPrefix.GetPath(), wxT("share")).GetFullPath();
    paths.Add(wxFileName(shareDir, wxT("locale")).GetFullPath());
  }

  return paths;
}

void RegisterLocalizationLookupPaths() {
  if (gLocalizationLookupPathsRegistered) {
    return;
  }

  const wxArrayString lookupPaths = BuildLocalizationLookupPaths();
  for (const wxString &path : lookupPaths) {
    AddCatalogLookupPathIfExists(path);
  }
  gLocalizationLookupPathsRegistered = true;
}

bool InitLocale(wxLocale &locale, int language) {
  if (language == wxLANGUAGE_UNKNOWN) {
    return false;
  }
  return locale.Init(language, wxLOCALE_DONT_LOAD_DEFAULT);
}

} // namespace

void InitializeLocalization(std::unique_ptr<wxLocale> &app_locale,
                            const wxString &languageOverride) {
  RegisterLocalizationLookupPaths();

  int requestedLanguage = wxLANGUAGE_UNKNOWN;
  bool hasOverride = false;
  if (!languageOverride.empty()) {
    const wxLanguageInfo *langInfo = wxLocale::FindLanguageInfo(languageOverride);
    if (langInfo != nullptr) {
      hasOverride = true;
      requestedLanguage = langInfo->Language;
    } else {
      wxLogWarning(wxT("Invalid language override '%s'. Falling back to system locale."),
                   languageOverride);
    }
  }

  const int systemLanguage = wxLocale::GetSystemLanguage();
  app_locale = std::make_unique<wxLocale>();

  if (hasOverride && InitLocale(*app_locale, requestedLanguage)) {
    wxLogMessage(wxT("Initialized language override: %s"),
                 wxLocale::GetLanguageName(requestedLanguage));
  } else if (InitLocale(*app_locale, systemLanguage)) {
    wxLogMessage(wxT("Initialized system locale: %s"),
                 wxLocale::GetLanguageName(systemLanguage));
  } else {
    wxLogWarning(wxT("Failed to initialize requested/system locale; falling back to English."));
    app_locale = std::make_unique<wxLocale>();
    if (!InitLocale(*app_locale, wxLANGUAGE_ENGLISH)) {
      wxLogWarning(
          wxT("Failed to initialize English locale. Continuing without catalog localization."));
      app_locale.reset();
      return;
    }
  }

  if (!app_locale->AddCatalog(kI18nDomain)) {
    wxLogMessage(
        wxT("No translation catalog found for domain '%s'. Using source language strings."),
        kI18nDomain);
  }
}
