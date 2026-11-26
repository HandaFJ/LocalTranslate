#pragma once
#include <memory>
#include <wx/wx.h>
#include <wx/splitter.h>
#include "AppConfig.h"
#include "OllamaClient.h"

class MainFrame : public wxFrame {
public:
    MainFrame();
    ~MainFrame() override;

private:
    enum { ID_Translate = wxID_HIGHEST + 1, ID_Clear, ID_Copy, ID_Refresh };

    void BuildUi();
    void LayoutSplit(bool narrow);

    void OnTranslate(wxCommandEvent&);
    void OnClear(wxCommandEvent&);
    void OnCopy(wxCommandEvent&);
    void OnRefresh(wxCommandEvent&);
    void OnSrcText(wxCommandEvent&);
    void OnSize(wxSizeEvent&);
    void OnClose(wxCloseEvent&);

    void RefreshModels(bool manual);
    void OnModelsDone(const ModelsResult& r, bool manual);
    void OnTranslateDone(const TranslateResult& r);

    void SetBusy(bool busy);
    void SetState(const wxString& s) { SetStatusText(s, 0); }

    AppConfig cfg_;
    std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);  // UIスレッドでのみ参照
    bool busy_ = false;
    bool narrow_ = false;

    wxChoice *srcChoice_{}, *dstChoice_{}, *presetChoice_{}, *modelChoice_{};
    wxButton *translateBtn_{}, *clearBtn_{}, *refreshBtn_{}, *copyBtn_{};
    wxSplitterWindow* splitter_{};
    wxTextCtrl *srcText_{}, *dstText_{};
    wxPanel* rightPanel_{};
};
