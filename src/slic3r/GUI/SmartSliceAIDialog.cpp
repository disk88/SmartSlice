#include <wx/app.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/hyperlink.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/stdpaths.h>
#include <wx/textctrl.h>
#include <wx/weakref.h>

#include "SmartSliceAIDialog.hpp"
#include "slic3r/Utils/Http.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <exception>
#include <regex>
#include <string>
#include <utility>

namespace Slic3r::GUI {
namespace {

constexpr const char* kDefaultOllamaUrl = "http://127.0.0.1:11434";
constexpr const char* kSuggestedModel = "qwen3:4b";

wxString settings_path()
{
    wxFileName directory(wxStandardPaths::Get().GetUserConfigDir(), "");
    directory.AppendDir("SmartSliceAI");
    if (!directory.DirExists()) {
        wxFileName::Mkdir(directory.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }
    return directory.GetPathWithSep() + "ai.ini";
}

bool valid_local_url(const std::string& url)
{
    // Only allow loopback endpoints in the MVP. No credential-bearing or remote URLs.
    static const std::regex pattern(R"(^http://(127\.0\.0\.1|localhost):([0-9]{2,5})$)");
    std::smatch match;
    if (!std::regex_match(url, match, pattern))
        return false;
    const int port = std::stoi(match[2].str());
    return port > 0 && port <= 65535;
}

enum class Operation { ListModels, Generate, PullModel };

class SmartSliceAIDialog final : public wxDialog
{
public:
    explicit SmartSliceAIDialog(wxWindow* parent)
        : wxDialog(parent, wxID_ANY, "SmartSlice AI - Gestione IA locale",
                   wxDefaultPosition, wxSize(700, 610),
                   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    {
        wxFileConfig settings("SmartSliceAI", "", settings_path(), "", wxCONFIG_USE_LOCAL_FILE);
        wxString saved_url;
        wxString saved_model;
        settings.Read("local/endpoint", &saved_url, kDefaultOllamaUrl);
        settings.Read("local/model", &saved_model, "");
        preferred_model_ = saved_model;

        auto* outer = new wxBoxSizer(wxVERTICAL);
        outer->Add(new wxStaticText(this, wxID_ANY, "SmartSlice AI - Ollama locale"), 0, wxALL, 8);
        status_ = new wxStaticText(this, wxID_ANY, "Verifica del servizio...");
        outer->Add(status_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

        outer->Add(new wxStaticText(this, wxID_ANY, "Indirizzo del servizio Ollama (solo localhost):"),
                   0, wxLEFT | wxRIGHT, 8);
        auto* endpoint_row = new wxBoxSizer(wxHORIZONTAL);
        endpoint_ = new wxTextCtrl(this, wxID_ANY, saved_url);
        endpoint_row->Add(endpoint_, 1, wxRIGHT, 8);
        auto* refresh = new wxButton(this, wxID_ANY, "Collega / Aggiorna");
        endpoint_row->Add(refresh);
        outer->Add(endpoint_row, 0, wxEXPAND | wxALL, 8);

        auto* model_row = new wxBoxSizer(wxHORIZONTAL);
        model_row->Add(new wxStaticText(this, wxID_ANY, "Modello installato:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        models_ = new wxChoice(this, wxID_ANY);
        model_row->Add(models_, 1, wxRIGHT, 8);
        auto* download = new wxButton(this, wxID_ANY, "Scarica modello 4B");
        model_row->Add(download);
        outer->Add(model_row, 0, wxEXPAND | wxALL, 8);

        outer->Add(new wxStaticText(this, wxID_ANY, "Richiesta di prova:"), 0, wxLEFT | wxRIGHT, 8);
        prompt_ = new wxTextCtrl(this, wxID_ANY,
            "Quali prove faresti per ridurre lo stringing PETG sulla Kobra 3 V2?",
            wxDefaultPosition, wxSize(-1, 65), wxTE_MULTILINE);
        outer->Add(prompt_, 0, wxEXPAND | wxALL, 8);
        send_ = new wxButton(this, wxID_ANY, "Prova AI locale");
        outer->Add(send_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
        outer->Add(new wxStaticText(this, wxID_ANY, "Risposta:"), 0, wxLEFT | wxRIGHT, 8);
        response_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                   wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
        outer->Add(response_, 1, wxEXPAND | wxALL, 8);
        outer->Add(new wxHyperlinkCtrl(this, wxID_ANY, "Installa Ollama (sito ufficiale)",
                                      "https://ollama.com/download/windows"), 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

        SetSizerAndFit(outer);
        SetMinSize(wxSize(620, 520));
        CentreOnParent();
        refresh->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { list_models(); });
        download->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { pull_model(); });
        send_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { generate(); });
        models_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { save_preferences(); });
        download_ = download;
        list_models();
    }

private:
    wxStaticText* status_{};
    wxTextCtrl* endpoint_{};
    wxChoice* models_{};
    wxTextCtrl* prompt_{};
    wxTextCtrl* response_{};
    wxButton* send_{};
    wxButton* download_{};
    wxString preferred_model_;
    bool busy_ = false;

    void save_preferences()
    {
        wxFileConfig settings("SmartSliceAI", "", settings_path(), "", wxCONFIG_USE_LOCAL_FILE);
        settings.Write("local/endpoint", endpoint_->GetValue());
        if (models_->GetSelection() != wxNOT_FOUND)
            settings.Write("local/model", models_->GetStringSelection());
        settings.Flush();
    }

    bool checked_url(std::string& url)
    {
        url = endpoint_->GetValue().ToStdString();
        if (!valid_local_url(url)) {
            wxMessageBox("Per ora sono ammessi solo http://localhost:PORTA o http://127.0.0.1:PORTA.",
                         "Indirizzo non valido", wxOK | wxICON_WARNING, this);
            return false;
        }
        return true;
    }

    void set_busy(bool busy)
    {
        busy_ = busy;
        send_->Enable(!busy && models_->GetSelection() != wxNOT_FOUND);
        download_->Enable(!busy);
        endpoint_->Enable(!busy);
        models_->Enable(!busy);
    }

    void request(Slic3r::Http request, Operation operation)
    {
        // Weak reference prevents accessing a closed dialog from a delayed callback.
        wxWeakRef<wxWindow> weak(this);
        request.timeout_connect(4)
            .timeout_max(operation == Operation::PullModel ? 1800 : 120)
            .size_limit(2 * 1024 * 1024)
            .on_complete([weak, operation](std::string body, unsigned status) {
                if (wxTheApp) wxTheApp->CallAfter([weak, operation, body = std::move(body), status]() {
                    if (!weak) return;
                    static_cast<SmartSliceAIDialog*>(weak.get())->handle_result(body, status, operation);
                });
            })
            .on_error([weak](std::string, std::string error, unsigned status) {
                if (wxTheApp) wxTheApp->CallAfter([weak, error = std::move(error), status]() {
                    if (!weak) return;
                    auto* dialog = static_cast<SmartSliceAIDialog*>(weak.get());
                    dialog->status_->SetLabel(status ? "HTTP " + std::to_string(status) : "Ollama non raggiungibile: " + error);
                    dialog->set_busy(false);
                });
            }).perform();
    }

    void list_models()
    {
        std::string url;
        if (!checked_url(url))
            return;
        if (models_->GetSelection() != wxNOT_FOUND)
            preferred_model_ = models_->GetStringSelection();
        models_->Clear();
        status_->SetLabel("Ricerca modelli già installati...");
        set_busy(true);
        request(Slic3r::Http::get(url + "/api/tags"), Operation::ListModels);
    }

    void pull_model()
    {
        std::string url;
        if (!checked_url(url))
            return;
        if (wxMessageBox(
              "Scaricare qwen3:4b sul computer? Il modello richiede diversi GB di spazio e una connessione Internet. "
              "Il download parte solo dopo questa conferma.",
              "Scarica modello locale", wxYES_NO | wxICON_QUESTION, this) != wxYES)
            return;
        nlohmann::json payload = {{"model", kSuggestedModel}, {"stream", false}};
        auto request_body = Slic3r::Http::post(url + "/api/pull");
        request_body.header("Content-Type", "application/json").set_post_body(payload.dump());
        status_->SetLabel("Download modello in corso. Non chiudere la finestra...");
        set_busy(true);
        request(std::move(request_body), Operation::PullModel);
    }

    void generate()
    {
        std::string url;
        if (!checked_url(url) || models_->GetSelection() == wxNOT_FOUND ||
            prompt_->GetValue().IsEmpty())
            return;
        save_preferences();
        nlohmann::json payload = {
            {"model", models_->GetStringSelection().ToStdString()},
            {"prompt", prompt_->GetValue().ToStdString()},
            {"stream", false}
        };
        auto request_body = Slic3r::Http::post(url + "/api/generate");
        request_body.header("Content-Type", "application/json").set_post_body(payload.dump());
        response_->SetValue("Generazione in corso...");
        status_->SetLabel("Attendo risposta dal modello locale...");
        set_busy(true);
        request(std::move(request_body), Operation::Generate);
    }

    void handle_result(const std::string& body, unsigned status, Operation operation)
    {
        if (status != 200) {
            status_->SetLabel("Ollama: HTTP " + std::to_string(status));
            set_busy(false);
            return;
        }
        try {
            auto data = nlohmann::json::parse(body);
            if (operation == Operation::ListModels) {
                for (const auto& entry : data.at("models")) {
                    if (entry.contains("name") && entry["name"].is_string())
                        models_->Append(wxString::FromUTF8(entry.at("name").get<std::string>()));
                }
                if (models_->GetCount()) {
                    const int pos = models_->FindString(preferred_model_);
                    models_->SetSelection(pos == wxNOT_FOUND ? 0 : pos);
                    status_->SetLabel("Ollama collegato. Modello esistente rilevato.");
                    save_preferences();
                } else {
                    status_->SetLabel("Ollama collegato. Nessun modello presente: puoi scaricarne uno.");
                }
            } else if (operation == Operation::Generate) {
                response_->SetValue(wxString::FromUTF8(data.at("response").get<std::string>()));
                status_->SetLabel("Test AI completato.");
            } else {
                status_->SetLabel("Modello scaricato. Aggiornamento elenco...");
                preferred_model_ = kSuggestedModel;
                set_busy(false);
                list_models();
                return;
            }
        } catch (const std::exception& ex) {
            status_->SetLabel("Risposta Ollama non valida.");
            response_->SetValue(wxString::FromUTF8(ex.what()));
        }
        set_busy(false);
    }
};

} // namespace

void show_smartslice_ai_dialog(wxWindow* parent)
{
    SmartSliceAIDialog dialog(parent);
    dialog.ShowModal();
}

} // namespace Slic3r::GUI
