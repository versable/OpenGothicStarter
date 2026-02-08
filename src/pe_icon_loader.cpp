#include "pe_icon_loader.h"

#include <cstdint>
#include <map>
#include <vector>
#include <wx/bitmap.h>
#include <wx/icon.h>
#include <wx/mstream.h>

#if defined(OGS_HAVE_PE_PARSE) && !defined(_WIN32)
#include <pe-parse/parse.h>

namespace {

bool ReadLe16(const std::vector<uint8_t> &data, size_t offset, uint16_t &value) {
  if (offset + 2 > data.size()) {
    return false;
  }

  const uint16_t lo = static_cast<uint16_t>(data[offset]);
  const uint16_t hi =
      static_cast<uint16_t>(static_cast<uint16_t>(data[offset + 1]) << 8);
  value = static_cast<uint16_t>(lo | hi);
  return true;
}

void AppendLe16(std::vector<uint8_t> &data, uint16_t value) {
  data.push_back(static_cast<uint8_t>(value & 0xFFu));
  data.push_back(static_cast<uint8_t>((value >> 8) & 0xFFu));
}

void AppendLe32(std::vector<uint8_t> &data, uint32_t value) {
  data.push_back(static_cast<uint8_t>(value & 0xFFu));
  data.push_back(static_cast<uint8_t>((value >> 8) & 0xFFu));
  data.push_back(static_cast<uint8_t>((value >> 16) & 0xFFu));
  data.push_back(static_cast<uint8_t>((value >> 24) & 0xFFu));
}

struct GroupIconEntry {
  uint8_t width;
  uint8_t height;
  uint8_t colorCount;
  uint8_t reserved;
  uint16_t planes;
  uint16_t bitCount;
  uint16_t resourceId;
  std::vector<uint8_t> imageData;
};

} // namespace
#endif

bool LoadIconFromPeExecutable(const wxString &path, wxIcon &icon) {
#if defined(OGS_HAVE_PE_PARSE) && !defined(_WIN32)
  if (wxImage::FindHandler(wxBITMAP_TYPE_ICO) == nullptr) {
    return false;
  }

  const wxScopedCharBuffer utf8Path = path.utf8_str();
  if (!utf8Path) {
    return false;
  }

  peparse::parsed_pe *parsed = peparse::ParsePEFromFile(utf8Path.data());
  if (parsed == nullptr) {
    return false;
  }

  struct ExtractionState {
    std::map<uint16_t, std::vector<uint8_t>> icons;
    std::vector<uint8_t> groupIconData;
  } extractionState;

  peparse::IterRsrc(
      parsed,
      [](void *N, const peparse::resource &r) -> int {
        auto *extraction = static_cast<ExtractionState *>(N);

        if (r.type != peparse::RT_ICON && r.type != peparse::RT_GROUP_ICON) {
          return 0;
        }

        std::vector<uint8_t> bytes;
        if (r.buf == nullptr) {
          return 0;
        }
        bytes.reserve(r.size);
        for (uint32_t i = 0; i < r.size; ++i) {
          uint8_t byte = 0;
          if (!peparse::readByte(r.buf, i, byte)) {
            return 0;
          }
          bytes.push_back(byte);
        }

        if (bytes.empty()) {
          return 0;
        }

        if (r.type == peparse::RT_ICON) {
          if (r.name <= UINT16_MAX && extraction->icons.find(static_cast<uint16_t>(r.name)) ==
                                          extraction->icons.end()) {
            extraction->icons.emplace(static_cast<uint16_t>(r.name), std::move(bytes));
          }
          return 0;
        }

        if (extraction->groupIconData.empty()) {
          extraction->groupIconData = std::move(bytes);
        }
        return 0;
      },
      &extractionState);

  peparse::DestructParsedPE(parsed);

  if (extractionState.groupIconData.empty() || extractionState.icons.empty()) {
    return false;
  }

  uint16_t reserved = 0;
  uint16_t type = 0;
  uint16_t count = 0;
  if (!ReadLe16(extractionState.groupIconData, 0, reserved) ||
      !ReadLe16(extractionState.groupIconData, 2, type) ||
      !ReadLe16(extractionState.groupIconData, 4, count)) {
    return false;
  }
  if (reserved != 0 || type != 1) {
    return false;
  }
  if (extractionState.groupIconData.size() < 6 + static_cast<size_t>(count) * 14) {
    return false;
  }

  std::vector<GroupIconEntry> entries;
  entries.reserve(count);
  for (uint16_t i = 0; i < count; ++i) {
    const size_t offset = 6 + static_cast<size_t>(i) * 14;

    uint16_t planes = 0;
    uint16_t bitCount = 0;
    uint16_t resourceId = 0;
    if (!ReadLe16(extractionState.groupIconData, offset + 4, planes) ||
        !ReadLe16(extractionState.groupIconData, offset + 6, bitCount) ||
        !ReadLe16(extractionState.groupIconData, offset + 12, resourceId)) {
      continue;
    }

    const auto it = extractionState.icons.find(resourceId);
    if (it == extractionState.icons.end()) {
      continue;
    }

    GroupIconEntry entry{};
    entry.width = extractionState.groupIconData[offset];
    entry.height = extractionState.groupIconData[offset + 1];
    entry.colorCount = extractionState.groupIconData[offset + 2];
    entry.reserved = extractionState.groupIconData[offset + 3];
    entry.planes = planes;
    entry.bitCount = bitCount;
    entry.resourceId = resourceId;
    entry.imageData = it->second;
    entries.push_back(std::move(entry));
  }
  if (entries.empty()) {
    return false;
  }

  std::vector<uint8_t> icoData;
  AppendLe16(icoData, 0);
  AppendLe16(icoData, 1);
  AppendLe16(icoData, static_cast<uint16_t>(entries.size()));

  uint32_t imageOffset = 6 + static_cast<uint32_t>(entries.size()) * 16;
  for (const GroupIconEntry &entry : entries) {
    icoData.push_back(entry.width);
    icoData.push_back(entry.height);
    icoData.push_back(entry.colorCount);
    icoData.push_back(entry.reserved);
    AppendLe16(icoData, entry.planes);
    AppendLe16(icoData, entry.bitCount);
    AppendLe32(icoData, static_cast<uint32_t>(entry.imageData.size()));
    AppendLe32(icoData, imageOffset);
    imageOffset += static_cast<uint32_t>(entry.imageData.size());
  }

  for (const GroupIconEntry &entry : entries) {
    icoData.insert(icoData.end(), entry.imageData.begin(), entry.imageData.end());
  }

  wxMemoryInputStream icoStream(icoData.data(), icoData.size());
  wxImage image(icoStream, wxBITMAP_TYPE_ICO);
  if (!image.IsOk()) {
    return false;
  }

  wxBitmap bitmap(image);
  if (!bitmap.IsOk()) {
    return false;
  }

  icon.CopyFromBitmap(bitmap);
  return icon.IsOk();
#else
  (void)path;
  (void)icon;
  return false;
#endif
}
