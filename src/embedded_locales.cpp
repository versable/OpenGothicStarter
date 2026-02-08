#include "embedded_locales.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <wx/app.h>
#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/mstream.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

extern const unsigned char gEmbeddedLocaleZip[];
extern const std::size_t gEmbeddedLocaleZipSize;

namespace {

wxString gBundledLocaleRoot;
wxString gBundledLocaleError;
bool gBundledLocalePrepared = false;

} // namespace

wxString GetBundledLocaleRoot(wxString &error) {
  if (!gBundledLocalePrepared) {
    gBundledLocalePrepared = true;

    if (gEmbeddedLocaleZipSize > 0) {
      constexpr uint64_t kExtractionFormatVersion = 2ull;
      uint64_t hash = 1469598103934665603ull;
      hash ^= kExtractionFormatVersion;
      hash *= 1099511628211ull;
      for (std::size_t i = 0; i < gEmbeddedLocaleZipSize; ++i) {
        hash ^= static_cast<uint64_t>(gEmbeddedLocaleZip[i]);
        hash *= 1099511628211ull;
      }

      const wxString hashLabel =
          wxString::Format(wxT("%016llx"), static_cast<unsigned long long>(hash));
      wxString cacheRoot;
      wxString xdgCacheHome;
      if (wxGetEnv(wxT("XDG_CACHE_HOME"), &xdgCacheHome) &&
          !xdgCacheHome.empty()) {
        cacheRoot = xdgCacheHome;
      } else {
        cacheRoot = wxFileName(wxGetHomeDir(), wxT(".cache")).GetFullPath();
      }

      wxString appDirName = wxT("OpenGothicStarter");
      if (wxTheApp != nullptr && !wxTheApp->GetAppName().empty()) {
        appDirName = wxTheApp->GetAppName();
      }

      const wxString cacheBase =
          wxFileName(cacheRoot, appDirName).GetFullPath();
      const wxString extractionRoot =
          wxFileName(cacheBase, hashLabel).GetFullPath();
      const wxString sentinelPath =
          wxFileName(extractionRoot, wxT(".ready")).GetFullPath();

      if (wxFileName::FileExists(sentinelPath)) {
        gBundledLocaleRoot = extractionRoot;
      } else {
        if (wxDirExists(extractionRoot)) {
          wxFileName::Rmdir(extractionRoot, wxPATH_RMDIR_RECURSIVE);
        }

        if (!wxFileName::Mkdir(extractionRoot, wxS_DIR_DEFAULT,
                               wxPATH_MKDIR_FULL)) {
          gBundledLocaleError = wxString::Format(
              wxT("Failed to create locale cache directory: %s"), extractionRoot);
        } else {
          wxMemoryInputStream zipData(gEmbeddedLocaleZip, gEmbeddedLocaleZipSize);
          wxZipInputStream zip(zipData);
          wxFileName rootDir(extractionRoot, wxEmptyString);
          rootDir.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
          const wxString rootPrefix = rootDir.GetPathWithSep();

          for (std::unique_ptr<wxZipEntry> entry(zip.GetNextEntry());
               entry != nullptr; entry.reset(zip.GetNextEntry())) {
            const wxString entryName = entry->GetName();
            if (entryName.empty()) {
              continue;
            }

            wxFileName outputPath(extractionRoot + wxFILE_SEP_PATH + entryName);
            outputPath.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
            if (!outputPath.GetFullPath().StartsWith(rootPrefix)) {
              gBundledLocaleError =
                  wxString::Format(wxT("Blocked unsafe zip entry path: %s"),
                                   entryName);
              gBundledLocaleRoot.clear();
              break;
            }

            if (entry->IsDir()) {
              if (!wxFileName::Mkdir(outputPath.GetFullPath(), wxS_DIR_DEFAULT,
                                     wxPATH_MKDIR_FULL)) {
                gBundledLocaleError = wxString::Format(
                    wxT("Failed to create locale directory: %s"),
                    outputPath.GetFullPath());
                gBundledLocaleRoot.clear();
                break;
              }
              continue;
            }

            if (!wxFileName::Mkdir(outputPath.GetPath(), wxS_DIR_DEFAULT,
                                   wxPATH_MKDIR_FULL)) {
              gBundledLocaleError = wxString::Format(
                  wxT("Failed to create locale parent directory: %s"),
                  outputPath.GetPath());
              gBundledLocaleRoot.clear();
              break;
            }

            wxFFileOutputStream out(outputPath.GetFullPath());
            if (!out.IsOk()) {
              gBundledLocaleError = wxString::Format(
                  wxT("Failed to write locale file: %s"),
                  outputPath.GetFullPath());
              gBundledLocaleRoot.clear();
              break;
            }

            char buffer[4096];
            while (zip.CanRead()) {
              zip.Read(buffer, sizeof(buffer));
              const size_t readCount = zip.LastRead();
              if (readCount == 0) {
                break;
              }

              out.Write(buffer, readCount);
              if (!out.IsOk()) {
                gBundledLocaleError = wxString::Format(
                    wxT("Failed while writing locale file: %s"),
                    outputPath.GetFullPath());
                gBundledLocaleRoot.clear();
                break;
              }
            }

            if (gBundledLocaleRoot.empty() && !gBundledLocaleError.empty()) {
              break;
            }
          }

          if (gBundledLocaleError.empty()) {
            wxFFile sentinel(sentinelPath, wxT("wb"));
            if (!sentinel.IsOpened() || !sentinel.Write(wxT("ready"))) {
              gBundledLocaleError = wxString::Format(
                  wxT("Failed to finalize locale cache marker: %s"),
                  sentinelPath);
            } else {
              gBundledLocaleRoot = extractionRoot;
            }
          }
        }
      }
    }
  }

  error = gBundledLocaleError;
  return gBundledLocaleRoot;
}
