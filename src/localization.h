#pragma once

#include <memory>

class wxLocale;

void InitializeLocalization(std::unique_ptr<wxLocale> &app_locale);
