#include "MainFrame.h"

#include <thread>
#include <wx/clipbrd.h>
#include <wx/config.h>
#include "PromptBuilder.h"

namespace {
wxString U(const char* s) { return wxString::FromUTF8(s); }
}

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "LocalTranslate", wxDefaultPosition, wxSize(1000, 640)) {
    cfg_.Load();
    BuildUi();
    Centre();
    RefreshModels(false);
}

MainFrame::~MainFrame() { *alive_ = false; }

void MainFrame::BuildUi() {
    auto* root = new wxPanel(this);
    auto* vbox = new wxBoxSizer(wxVERTICAL);

    // ---- 上部コントロールバー ----
    auto* bar = new wxBoxSizer(wxHORIZONTAL);
    srcChoice_ = new wxChoice(root, wxID_ANY);
    for (auto& l : PromptBuilder::SourceLanguages()) srcChoice_->Append(U(l.label));
    dstChoice_ = new wxChoice(root, wxID_ANY);
    for (auto& l : PromptBuilder::TargetLanguages()) dstChoice_->Append(U(l.label));
    presetChoice_ = new wxChoice(root, wxID_ANY);
    for (auto& p : PromptBuilder::Presets()) presetChoice_->Append(U(p.label));
    modelChoice_ = new wxChoice(root, wxID_ANY, wxDefaultPosition, wxSize(200, -1));
    refreshBtn_ = new wxButton(root, ID_Refresh, U("更新"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    translateBtn_ = new wxButton(root, ID_Translate, U("翻訳 (Ctrl+Enter)"));
    clearBtn_ = new wxButton(root, ID_Clear, U("クリア"));

    srcChoice_->SetSelection(cfg_.srcLang < (int)srcChoice_->GetCount() ? cfg_.srcLang : 0);
    dstChoice_->SetSelection(cfg_.dstLang < (int)dstChoice_->GetCount() ? cfg_.dstLang : 0);
    presetChoice_->SetSelection(cfg_.preset < (int)presetChoice_->GetCount() ? cfg_.preset : 0);

    auto addLabeled = [&](const wxString& label, wxWindow* w) {
        bar->Add(new wxStaticText(root, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
        bar->Add(w, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    };
    addLabeled(U("元言語"), srcChoice_);
    addLabeled(U("訳言語"), dstChoice_);
    addLabeled(U("プリセット"), presetChoice_);
    addLabeled(U("モデル"), modelChoice_);
    bar->Add(refreshBtn_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    bar->AddStretchSpacer(1);
    bar->Add(translateBtn_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    bar->Add(clearBtn_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);
    vbox->Add(bar, 0, wxEXPAND | wxTOP | wxBOTTOM, 6);

    // ---- 中央2分割ペイン ----
    splitter_ = new wxSplitterWindow(root, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     wxSP_LIVE_UPDATE | wxSP_3DSASH);
    splitter_->SetMinimumPaneSize(120);
    splitter_->SetSashGravity(0.5);

    srcText_ = new wxTextCtrl(splitter_, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                              wxTE_MULTILINE | wxTE_WORDWRAP);
    rightPanel_ = new wxPanel(splitter_);
    auto* rbox = new wxBoxSizer(wxVERTICAL);
    dstText_ = new wxTextCtrl(rightPanel_, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                              wxTE_MULTILINE | wxTE_WORDWRAP | wxTE_READONLY);
    copyBtn_ = new wxButton(rightPanel_, ID_Copy, U("訳文をコピー"));
    rbox->Add(dstText_, 1, wxEXPAND);
    rbox->Add(copyBtn_, 0, wxALIGN_RIGHT | wxTOP, 4);
    rightPanel_->SetSizer(rbox);

    splitter_->SplitVertically(srcText_, rightPanel_);
    vbox->Add(splitter_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    root->SetSizer(vbox);

    // ---- ステータスバー ----
    CreateStatusBar(4);
    const int widths[] = {-2, -3, -2, -2};
    SetStatusWidths(4, widths);
    SetState(U("状態: 未接続"));
    SetStatusText(U("モデル: -"), 1);
    SetStatusText(U("原文: 0文字"), 2);
    SetStatusText(U("処理時間: -.-s"), 3);

    // ---- ショートカット Ctrl+Enter ----
    wxAcceleratorEntry entries[1];
    entries[0].Set(wxACCEL_CTRL, WXK_RETURN, ID_Translate);
    SetAcceleratorTable(wxAcceleratorTable(1, entries));

    Bind(wxEVT_BUTTON, &MainFrame::OnTranslate, this, ID_Translate);
    Bind(wxEVT_MENU, &MainFrame::OnTranslate, this, ID_Translate);
    Bind(wxEVT_BUTTON, &MainFrame::OnClear, this, ID_Clear);
    Bind(wxEVT_BUTTON, &MainFrame::OnCopy, this, ID_Copy);
    Bind(wxEVT_BUTTON, &MainFrame::OnRefresh, this, ID_Refresh);
    Bind(wxEVT_TEXT, &MainFrame::OnSrcText, this, srcText_->GetId());
    Bind(wxEVT_SIZE, &MainFrame::OnSize, this);
    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);
}

// 幅が狭いときは上下分割へ切り替える
void MainFrame::LayoutSplit(bool narrow) {
    if (narrow == narrow_) return;
    narrow_ = narrow;
    if (splitter_->IsSplit()) splitter_->Unsplit();
    srcText_->Show();
    rightPanel_->Show();
    if (narrow) splitter_->SplitHorizontally(srcText_, rightPanel_);
    else        splitter_->SplitVertically(srcText_, rightPanel_);
}

void MainFrame::OnSize(wxSizeEvent& e) {
    if (splitter_) LayoutSplit(GetClientSize().x < 720);
    e.Skip();
}

void MainFrame::OnSrcText(wxCommandEvent&) {
    SetStatusText(wxString::Format(U("原文: %lu文字"), (unsigned long)srcText_->GetValue().length()), 2);
}

void MainFrame::OnClear(wxCommandEvent&) {
    srcText_->Clear();
    dstText_->Clear();
    SetStatusText(U("処理時間: -.-s"), 3);
    srcText_->SetFocus();
}

void MainFrame::OnCopy(wxCommandEvent&) {
    const wxString text = dstText_->GetValue();
    if (text.empty()) return;
    if (wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxTextDataObject(text));
        wxTheClipboard->Close();
        SetState(U("状態: 訳文をコピーしました"));
    }
}

void MainFrame::OnRefresh(wxCommandEvent&) { RefreshModels(true); }

void MainFrame::SetBusy(bool busy) {
    busy_ = busy;
    translateBtn_->Enable(!busy);
    refreshBtn_->Enable(!busy);
}

// ---- モデル一覧（非同期） ----
void MainFrame::RefreshModels(bool manual) {
    SetState(U("状態: 接続中..."));
    refreshBtn_->Disable();
    auto alive = alive_;
    const std::string endpoint = cfg_.endpoint;
    std::thread([this, alive, endpoint, manual] {
        ModelsResult r = OllamaClient(endpoint).ListModels();
        wxTheApp->CallAfter([this, alive, r, manual] {
            if (!*alive) return;
            OnModelsDone(r, manual);
        });
    }).detach();
}

void MainFrame::OnModelsDone(const ModelsResult& r, bool manual) {
    refreshBtn_->Enable(!busy_);
    if (!r.ok) {
        SetState(U("状態: 未接続"));
        if (manual) wxMessageBox(wxString::FromUTF8(r.error), U("接続エラー"), wxOK | wxICON_WARNING, this);
        else SetStatusText(wxString::FromUTF8(r.error), 1);
        return;
    }
    const wxString current = modelChoice_->GetSelection() != wxNOT_FOUND
                                 ? modelChoice_->GetStringSelection() : cfg_.lastModel;
    modelChoice_->Clear();
    for (auto& m : r.models) modelChoice_->Append(wxString::FromUTF8(m));
    if (modelChoice_->GetCount() == 0) {
        SetState(U("状態: 接続OK（モデル未導入）"));
        SetStatusText(U("モデル: -"), 1);
        if (manual)
            wxMessageBox(U("導入済みのモデルがありません。`ollama pull <モデル名>` で導入してください。"),
                         U("モデルなし"), wxOK | wxICON_INFORMATION, this);
        return;
    }
    int idx = modelChoice_->FindString(current);
    modelChoice_->SetSelection(idx == wxNOT_FOUND ? 0 : idx);
    SetState(U("状態: 接続OK"));
    SetStatusText(U("モデル: ") + modelChoice_->GetStringSelection(), 1);
}

// ---- 翻訳（非同期） ----
void MainFrame::OnTranslate(wxCommandEvent&) {
    if (busy_) return;
    const wxString src = srcText_->GetValue();
    if (src.find_first_not_of(" \t\r\n") == wxString::npos) return;  
    if (modelChoice_->GetSelection() == wxNOT_FOUND) {
        wxMessageBox(U("モデルが選択されていません。「更新」でモデル一覧を取得してください。"),
                     U("モデル未選択"), wxOK | wxICON_INFORMATION, this);
        return;
    }

    const std::string model = modelChoice_->GetStringSelection().utf8_string();
    const std::string system = PromptBuilder::BuildSystemPrompt(
        srcChoice_->GetSelection(), dstChoice_->GetSelection(), presetChoice_->GetSelection());
    const std::string user = src.utf8_string();
    const std::string endpoint = cfg_.endpoint;

    SetBusy(true);
    SetState(U("状態: 翻訳中..."));
    SetStatusText(U("モデル: ") + modelChoice_->GetStringSelection(), 1);

    auto alive = alive_;
    std::thread([this, alive, endpoint, model, system, user] {
        TranslateResult r = OllamaClient(endpoint).Translate(model, system, user);
        wxTheApp->CallAfter([this, alive, r] {
            if (!*alive) return;
            OnTranslateDone(r);
        });
    }).detach();
}

void MainFrame::OnTranslateDone(const TranslateResult& r) {
    SetBusy(false);
    SetStatusText(wxString::Format(U("処理時間: %.1fs"), r.seconds), 3);
    if (r.ok) {
        dstText_->SetValue(wxString::FromUTF8(r.text));
        SetState(U("状態: 接続OK"));
    } else {
        SetState(U("状態: エラー"));
        wxMessageBox(wxString::FromUTF8(r.error), U("翻訳エラー"), wxOK | wxICON_ERROR, this);
    }
}

void MainFrame::OnClose(wxCloseEvent& e) {
    cfg_.srcLang = srcChoice_->GetSelection();
    cfg_.dstLang = dstChoice_->GetSelection();
    cfg_.preset = presetChoice_->GetSelection();
    if (modelChoice_->GetSelection() != wxNOT_FOUND) cfg_.lastModel = modelChoice_->GetStringSelection();
    cfg_.Save();
    e.Skip();
}
