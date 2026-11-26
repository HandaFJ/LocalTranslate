#pragma once
#include <string>
#include <wx/string.h>

struct AppConfig {
    std::string endpoint = "http://localhost:11434";
    wxString lastModel;
    int srcLang = 0;   // 0 = 自動判定
    int dstLang = 0;   // 0 = 日本語
    int preset = 0;

    void Load();
    void Save() const;
};
