#include "SmartSliceAIDialog.hpp"

#include "slic3r/Utils/Http.hpp"
#include <nlohmann/json.hpp>
#include <wx/app.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/hyperlink.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/weakref.h>

#include <exception>
#include <string>

namespace Slic3r::GUI {
namespace {

constexpr const char* kOllamaBase = "http://127.0.0.1:11434";

class SmartSliceAIDialog final : public wxDialog
{
public:
    explicit SmartSliceAIDialog(wxWindow* parent)
        : wxDialog(parent, wxID_ANY, "SmartSlice AI - Local model test",
                   wxDefaultPosition, wxSize(650, 510),
                   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    {
        auto* outer = new wxBoxSizer(wxVERTICAL);
        outer->Add(new wxStaticText(this, wxID_ANY, "Local AI test (Ollama)"), 0, wxALL, 8);
        status_ = new wxStaticText(this, wxID_ANY, "Checking Ollama...");
        outer->Add(status_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

        auto* models = new wxBoxSizer(wxHORIZONTAL);
        models->Add(new wxStaticText(this, wxID_ANY, "Model:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        model_ = new wxChoice(this, wxID_ANY);
        models->Add(model_, 1, wxRIGHT, 8);
        auto* refresh = new wxButton(this, wxID_ANY, "Refresh");
        models->Add(refresh);
        outer->Add(models, 0, wxEXPAND | wxALL, 8);

        outer->Add(new wxStaticText(this, wxID_ANY, "Ask a question:"), 0, wxLEFT | wxRIGHT, 8);
        prompt_ = new wxTextCtrl(this, wxID_ANY,
            "Which PETG parameters can reduce stringing on an Anycubic Kobra 3 V2?",
            wxDefaultPosition, wxSize(-1, 70), wxTE_MULTILINE);
        outer->Add(prompt_, 0, wxEXPAND | wxALL, 8);
        send_ = new wxButton(this, wxID_ANY, "Ask local AI");
        outer->Add(send_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
        outer->Add(new wxStaticText(this, wxID_ANY, "Response:"), 0, wxLEFT | wxRIGHT, 8);
        response_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                   wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
        outer->Add(response_, 1, wxEXPAND | wxALL, 8);
        outer->Add(new wxHyperlinkCtrl(this, wxID_ANY, "Install Ollama / download a model",
                                      "https://ollama.com/download/windows"), 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
        SetSizerAndFit(outer);
        SetMinSize(wxSize(540, 440));
        CentreOnParent();

        refresh->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { load_models(); });
        send_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ask_model(); });
        load_models();
    }

private:
    wxStaticText* status_{};
    wxChoice* model_{};
    wxTextCtrl* prompt_{};
    wxTextCtrl* response_{};
    wxButton* send_{};

    // HTTP callbacks run off the UI thread. Never access wxWidgets from those callbacks.
    void request(Slic3r::Http request, bool models)
    {
        wxWeakRef<wxWindow> weak(this);
        request.timeout_connect(3).timeout_max(120).size_limit(1024 * 1024)
            .on_complete([weak, models](std::string body, unsigned status) {
                if (wxTheApp)
                    wxTheApp->CallAfter([weak, models, body = std::move(body), status]() {
                        if (!weak) return;
                        auto* dlg = static_cast<SmartSliceAIDialog*>(weak.get());
                        dlg->handle_response(body, status, models);
                    });
            })
            .on_error([weak, models](std::string, std::string error, unsigned status) {
                if (wxTheApp)
                    wxTheApp->CallAfter([weak, models, error = std::move(error), status]() {
                        if (!weak) return;
                        auto* dlg = static_cast<SmartSliceAIDialog*>(weak.get());
                        dlg->status_->SetLabel(status ? "Ollama HTTP error: " + std::to_string(status)
                                                      : "Ollama unavailable: " + error);
                        if (!models) dlg->send_->Enable(true);
                    });
            }).perform();
    }

    void load_models()
    {
        model_->Clear();
        send_->Enable(false);
        status_->SetLabel("Connecting to local Ollama...");
        request(Slic3r::Http::get(std::string(kOllamaBase) + "/api/tags"), true);
    }

    void ask_model()
    {
        if (model_->GetSelection() == wxNOT_FOUND || prompt_->GetValue().IsEmpty())
            return;
        send_->Enable(false);
        response_->SetValue("Generating locally...");
        status_->SetLabel("Waiting for model response...");
        nlohmann::json payload = {
            {"model", model_->GetStringSelection().ToStdString()},
            {"prompt", prompt_->GetValue().ToStdString()},
            {"stream", false}
        };
        auto req = Slic3r::Http::post(std::string(kOllamaBase) + "/api/generate");
        req.header("Content-Type", "application/json").set_post_body(payload.dump());
        request(std::move(req), false);
    }

    void handle_response(const std::string& body, unsigned status, bool models)
    {
        if (status != 200) {
            status_->SetLabel("Ollama HTTP error: " + std::to_string(status));
            send_->Enable(model_->GetCount() != 0);
            return;
        }
        try {
            const auto data = nlohmann::json::parse(body);
            if (models) {
                for (const auto& item : data.at("models")) {
                    if (item.contains("name") && item["name"].is_string())
                        model_->Append(wxString::FromUTF8(item["name"].get<std::string>()));
                }
                if (model_->GetCount()) {
                    model_->SetSelection(0);
                    status_->SetLabel("Ollama connected. Select a model and ask a question.");
                    send_->Enable(true);
                } else {
                    status_->SetLabel("Ollama connected, but no models installed. Download one first.");
                }
            } else {
                response_->SetValue(wxString::FromUTF8(data.at("response").get<std::string>()));
                status_->SetLabel("Response received from local AI.");
                send_->Enable(true);
            }
        } catch (const std::exception& ex) {
            status_->SetLabel("Unexpected response from Ollama.");
            response_->SetValue(wxString::FromUTF8(ex.what()));
            send_->Enable(model_->GetCount() != 0);
        }
    }
};
} // namespace

void show_smartslice_ai_dialog(wxWindow* parent)
{
    SmartSliceAIDialog dialog(parent);
    dialog.ShowModal();
}

} // namespace Slic3r::GUI
