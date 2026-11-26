#include "AppConfig.h"
#include <wx/config.h>

void AppConfig::Load() {
    wxConfigBase* c = wxConfigBase::Get();
    wxString ep;
    c->Read("endpoint", &ep, wxString::FromUTF8(endpoint));
    endpoint = ep.utf8_string();
    c->Read("lastModel", &lastModel, wxString());
    c->Read("srcLang", &srcLang, 0);
    c->Read("dstLang", &dstLang, 0);
    c->Read("preset", &preset, 0);
}

void AppConfig::Save() const {
    wxConfigBase* c = wxConfigBase::Get();
    c->Write("endpoint", wxString::FromUTF8(endpoint));
    c->Write("lastModel", lastModel);
    c->Write("srcLang", srcLang);
    c->Write("dstLang", dstLang);
    c->Write("preset", preset);
    c->Flush();
}
