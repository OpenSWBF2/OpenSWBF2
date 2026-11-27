#pragma once
#include <string>
#include <utility>
#include <vector>

namespace Hardening
{

struct XmlNode
{
    bool bElement;
    bool bCData;
    std::string sText;
    std::string sLocal;
    std::string sNs;
    std::vector<std::pair<std::string, std::string> > kAttrs;
    std::vector<XmlNode> kChildren;
    const XmlNode* pParent;
};

extern const char* const kMsBuildNs;

std::string Attr(const XmlNode& kNode, const char* pcName);
void Elements(const XmlNode& kNode, std::vector<const XmlNode*>& kOut);
bool IsMsb(const XmlNode& kNode, const char* pcLocal);
std::string InnerText(const XmlNode& kNode);
bool ParseXml(const std::string& sText, const std::string& sWhat, XmlNode& kRoot, std::string& sError);

}
