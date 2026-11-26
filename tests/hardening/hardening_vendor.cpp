#include <windows.h>

#include <map>

#include "gate/gate_keys.h"
#include "hardening/hardening_vendor.h"
#include "hardening/hardening_xml.h"

namespace Hardening
{

namespace
{

const char* const kStagedName = "vendor:d3dx9-staged";

D3dxStaged Staged(const char* pcStatus, const std::string& sDetail)
{
    D3dxStaged k;
    k.sStatus = pcStatus;
    k.sDetail = sDetail;
    return k;
}

bool VendorPins(const std::string& sText, std::map<std::string, std::string>& kPins, std::string& sError)
{
    XmlNode kRoot;
    if (!ParseXml(sText, "lib\\vendor-pins.props", kRoot, sError))
    {
        return false;
    }
    for (size_t g = 0; g < kRoot.kChildren.size(); ++g)
    {
        const XmlNode& kGroup = kRoot.kChildren[g];
        if (!kGroup.bElement || !Gate::EqualNoCase(kGroup.sLocal, "PropertyGroup"))
        {
            continue;
        }
        for (size_t p = 0; p < kGroup.kChildren.size(); ++p)
        {
            if (kGroup.kChildren[p].bElement)
            {
                kPins[Gate::LowerText(kGroup.kChildren[p].sLocal)] = Gate::TrimText(InnerText(kGroup.kChildren[p]));
            }
        }
    }
    return true;
}

}

D3dxStaged D3dxStagedCheck(const std::string& sPinsText, const std::string& sPlatform, const std::string& sExeDir)
{
    if (GetFileAttributesW(Gate::Utf8ToWide(Gate::JoinPath(sExeDir, "data")).c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        return Staged("SKIP", "not staged");
    }
    std::map<std::string, std::string> kPins;
    std::string sError;
    if (!VendorPins(sPinsText, kPins, sError))
    {
        return Staged("FAIL", sError);
    }
    const std::map<std::string, std::string>::const_iterator it =
        kPins.find(Gate::EqualNoCase(sPlatform, "x64") ? "d3dxx64sha256" : "d3dxwin32sha256");
    const std::string sPin = it == kPins.end() ? std::string() : it->second;
    const std::string sDll = Gate::JoinPath(sExeDir, "d3dx9_43.dll");
    if (!Gate::FileExists(sDll))
    {
        return Staged("FAIL", sDll + " missing");
    }
    std::string sSha;
    if (!Gate::FileSha(sDll, sSha))
    {
        return Staged("FAIL", "cannot hash " + sDll);
    }
    sSha = Gate::LowerText(sSha);
    if (sSha != sPin)
    {
        return Staged("FAIL", sDll + " is " + sSha + ", pin is " + sPin);
    }
    return Staged("PASS", std::string());
}

std::string D3dxStagedLine(const D3dxStaged& kCheck)
{
    const std::string sHead = "[" + kCheck.sStatus + "] " + kStagedName;
    return kCheck.sDetail.empty() ? sHead : sHead + " - " + kCheck.sDetail;
}

}
