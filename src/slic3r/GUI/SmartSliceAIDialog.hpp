#pragma once

class wxWindow;

namespace Slic3r::GUI {

// Opens the first SmartSlice AI smoke-test dialog.
// This dialog never changes print settings or communicates with a printer.
void show_smartslice_ai_dialog(wxWindow* parent);

} // namespace Slic3r::GUI
