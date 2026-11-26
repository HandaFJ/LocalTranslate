#include <wx/wx.h>
#include "MainFrame.h"

class TranslateApp : public wxApp {
public:
    bool OnInit() override {
        SetAppName("LocalTranslate");
        auto* frame = new MainFrame();
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(TranslateApp);
