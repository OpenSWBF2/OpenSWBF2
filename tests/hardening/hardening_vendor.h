#pragma once
#include <string>

namespace Hardening
{

struct D3dxStaged
{
    std::string sStatus;
    std::string sDetail;
};

D3dxStaged D3dxStagedCheck(const std::string& sPinsText, const std::string& sPlatform, const std::string& sExeDir);
std::string D3dxStagedLine(const D3dxStaged& kCheck);

}
