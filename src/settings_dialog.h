#pragma once

#include <vector>
#include <wx/dialog.h>
#include <wx/string.h>

enum class GothicVersion : int;
class wxChoice;
class wxCheckBox;
class wxSpinCtrl;
class wxSpinCtrlDouble;

struct SystemPackSettings {
  bool show_fps_counter = false;
  bool hide_focus = false;
  double vertical_fov = 67.5;
  int fps_limit = 0;
  double interface_scale = 1.0;
  int inventory_cell_size = 70;
  int new_chapter_size_x = 800;
  int new_chapter_size_y = 600;
  int save_game_image_size_x = 320;
  int save_game_image_size_y = 200;
  bool show_health_bar = true;
  int show_mana_bar = 2;
  int show_swim_bar = 1;
};

class SettingsDialog : public wxDialog {
public:
  SettingsDialog(wxWindow *parent, GothicVersion initialVersion,
                 const wxString &initialLanguage,
                 const SystemPackSettings &initialSystemPack);

  GothicVersion GetSelectedVersion() const;
  wxString GetLanguageOverride() const;
  SystemPackSettings GetSystemPackSettings() const;

private:
  wxChoice *version_choice;
  wxChoice *language_choice;
  std::vector<wxString> language_codes;
  wxCheckBox *show_fps_counter_checkbox;
  wxCheckBox *hide_focus_checkbox;
  wxSpinCtrlDouble *vertical_fov_ctrl;
  wxSpinCtrl *fps_limit_ctrl;
  wxSpinCtrlDouble *interface_scale_ctrl;
  wxSpinCtrl *inventory_cell_size_ctrl;
  wxSpinCtrl *new_chapter_size_x_ctrl;
  wxSpinCtrl *new_chapter_size_y_ctrl;
  wxSpinCtrl *save_game_image_size_x_ctrl;
  wxSpinCtrl *save_game_image_size_y_ctrl;
  wxCheckBox *show_health_bar_checkbox;
  wxChoice *show_mana_bar_choice;
  wxChoice *show_swim_bar_choice;
};
