#include "localization.h"
#include "embedded_locales.h"

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/stdpaths.h>
#include <wx/translation.h>

namespace {

const wxString kI18nDomain = wxT("opengothicstarter");
bool gLocalizationLookupPathsRegistered = false;

bool InitLocale(wxLocale &locale, int language) {
  if (language == wxLANGUAGE_UNKNOWN) {
    return false;
  }
  return locale.Init(language, wxLOCALE_DONT_LOAD_DEFAULT);
}

} // namespace

void InitializeLocalization(std::unique_ptr<wxLocale> &app_locale,
                            const wxString &languageOverride) {
  if (!gLocalizationLookupPathsRegistered) {
    wxArrayString lookupPaths;

    wxFileName exeFile(wxStandardPaths::Get().GetExecutablePath());
    exeFile.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
    const wxString exeDir = exeFile.GetPath();

    lookupPaths.Add(wxFileName(exeDir, wxT("locale")).GetFullPath());

    wxFileName installPrefix(exeDir, wxEmptyString);
    installPrefix.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
    if (!installPrefix.GetDirs().empty()) {
      installPrefix.RemoveLastDir(); // bin -> install prefix
      const wxString shareDir =
          wxFileName(installPrefix.GetPath(), wxT("share")).GetFullPath();
      lookupPaths.Add(wxFileName(shareDir, wxT("locale")).GetFullPath());
    }

    wxString bundledLocaleError;
    const wxString bundledLocaleRoot = GetBundledLocaleRoot(bundledLocaleError);
    if (!bundledLocaleRoot.empty()) {
      lookupPaths.Add(bundledLocaleRoot);
    } else if (!bundledLocaleError.empty()) {
      wxLogWarning(wxT("Bundled locale extraction failed: %s"), bundledLocaleError);
    }

    for (const wxString &path : lookupPaths) {
      if (!wxDir::Exists(path)) {
        continue;
      }

      wxLocale::AddCatalogLookupPathPrefix(path);
      wxLogMessage(wxT("Localization lookup path: %s"), path);
    }

    gLocalizationLookupPathsRegistered = true;
  }

  bool hasOverride = false;
  wxString overrideCanonicalLanguage;
  if (!languageOverride.empty()) {
    const wxLanguageInfo *langInfo = wxLocale::FindLanguageInfo(languageOverride);
    if (langInfo != nullptr) {
      hasOverride = true;
      overrideCanonicalLanguage = langInfo->CanonicalName;
    } else {
      wxLogWarning(wxT("Invalid language override '%s'. Falling back to system locale."),
                   languageOverride);
    }
  }

  const int systemLanguage = wxLocale::GetSystemLanguage();
  app_locale = std::make_unique<wxLocale>();

  if (InitLocale(*app_locale, systemLanguage)) {
    wxLogMessage(wxT("Initialized system locale: %s"),
                 wxLocale::GetLanguageName(systemLanguage));
  } else {
    wxLogWarning(wxT("Failed to initialize system locale; falling back to English."));
    app_locale = std::make_unique<wxLocale>();
    if (!InitLocale(*app_locale, wxLANGUAGE_ENGLISH)) {
      wxLogWarning(
          wxT("Failed to initialize English locale. Continuing without catalog localization."));
      app_locale.reset();
      return;
    }
  }

  wxString catalogLanguage;
  if (hasOverride && !overrideCanonicalLanguage.empty()) {
    catalogLanguage = overrideCanonicalLanguage;
  } else if (app_locale->IsOk()) {
    catalogLanguage = app_locale->GetCanonicalName();
  }

  auto *translations = new wxTranslations();
  wxTranslations::Set(translations);

  if (!catalogLanguage.empty()) {
    translations->SetLanguage(catalogLanguage);
  }

  if (!translations->AddCatalog(kI18nDomain, wxLANGUAGE_ENGLISH_US)) {
    wxLogMessage(
        wxT("No translation catalog found for domain '%s'. Using source language strings."),
        kI18nDomain);
    return;
  }

  if (!catalogLanguage.empty()) {
    wxLogMessage(wxT("Loaded translation catalog '%s' for language '%s'."),
                 kI18nDomain, catalogLanguage);
  }
}
