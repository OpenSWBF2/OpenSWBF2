#include <windows.h>
#include <stdlib.h>
#include <string.h>

#include <map>

#include "hardening/hardening_xml.h"

namespace Hardening
{

namespace
{

bool IsSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

std::string Narrow(const std::wstring& sText)
{
    if (sText.empty())
    {
        return std::string();
    }
    const int iLen = WideCharToMultiByte(CP_UTF8, 0, sText.data(), static_cast<int>(sText.size()), 0, 0, 0, 0);
    std::string sOut(static_cast<size_t>(iLen), '\0');
    WideCharToMultiByte(CP_UTF8, 0, sText.data(), static_cast<int>(sText.size()), &sOut[0], iLen, 0, 0);
    return sOut;
}

class XmlReader
{
public:
    XmlReader(const std::string& sText) : msText(sText), muPos(0)
    {
    }

    bool Parse(XmlNode& kRoot, std::string& sError)
    {
        for (;;)
        {
            SkipSpace();
            if (Starts("<?"))
            {
                if (!SkipPast("?>", sError))
                {
                    return false;
                }
            }
            else if (Starts("<!--"))
            {
                if (!SkipPast("-->", sError))
                {
                    return false;
                }
            }
            else if (Starts("<!DOCTYPE"))
            {
                if (!SkipPast(">", sError))
                {
                    return false;
                }
            }
            else
            {
                break;
            }
        }
        if (!Starts("<"))
        {
            sError = "xml has no root element";
            return false;
        }
        std::map<std::string, std::string> kScope;
        if (!ParseElement(kRoot, kScope, sError))
        {
            return false;
        }
        Link(kRoot, 0);
        return true;
    }

private:
    static void Link(XmlNode& kNode, const XmlNode* pParent)
    {
        kNode.pParent = pParent;
        for (size_t i = 0; i < kNode.kChildren.size(); ++i)
        {
            Link(kNode.kChildren[i], &kNode);
        }
    }

    bool Starts(const char* pcText) const
    {
        return msText.compare(muPos, strlen(pcText), pcText) == 0;
    }

    void SkipSpace()
    {
        while (muPos < msText.size() && IsSpace(msText[muPos]))
        {
            ++muPos;
        }
    }

    bool SkipPast(const char* pcEnd, std::string& sError)
    {
        const size_t uAt = msText.find(pcEnd, muPos);
        if (uAt == std::string::npos)
        {
            sError = std::string("xml is missing ") + pcEnd;
            return false;
        }
        muPos = uAt + strlen(pcEnd);
        return true;
    }

    static void AppendUtf8(unsigned long uCode, std::string& sOut)
    {
        std::wstring sWide;
        if (uCode >= 0x10000)
        {
            uCode -= 0x10000;
            sWide += static_cast<wchar_t>(0xD800 + (uCode >> 10));
            sWide += static_cast<wchar_t>(0xDC00 + (uCode & 0x3FF));
        }
        else
        {
            sWide += static_cast<wchar_t>(uCode);
        }
        sOut += Narrow(sWide);
    }

    static bool Decode(const std::string& sRaw, bool bAttribute, std::string& sOut, std::string& sError)
    {
        sOut.clear();
        for (size_t i = 0; i < sRaw.size(); ++i)
        {
            const char c = sRaw[i];
            if (c == '&')
            {
                const size_t uEnd = sRaw.find(';', i);
                if (uEnd == std::string::npos)
                {
                    sError = "xml entity without ;";
                    return false;
                }
                const std::string sName = sRaw.substr(i + 1, uEnd - i - 1);
                if (sName == "amp")
                {
                    sOut += '&';
                }
                else if (sName == "lt")
                {
                    sOut += '<';
                }
                else if (sName == "gt")
                {
                    sOut += '>';
                }
                else if (sName == "quot")
                {
                    sOut += '"';
                }
                else if (sName == "apos")
                {
                    sOut += '\'';
                }
                else if (sName.size() > 1 && sName[0] == '#')
                {
                    const bool bHex = sName[1] == 'x' || sName[1] == 'X';
                    AppendUtf8(strtoul(sName.c_str() + (bHex ? 2 : 1), 0, bHex ? 16 : 10), sOut);
                }
                else
                {
                    sError = "xml entity &" + sName + "; is not known";
                    return false;
                }
                i = uEnd;
                continue;
            }
            if (c == '\r')
            {
                if (i + 1 < sRaw.size() && sRaw[i + 1] == '\n')
                {
                    ++i;
                }
                sOut += bAttribute ? ' ' : '\n';
                continue;
            }
            if (bAttribute && (c == '\n' || c == '\t'))
            {
                sOut += ' ';
                continue;
            }
            sOut += c;
        }
        return true;
    }

    std::string ReadName()
    {
        const size_t uStart = muPos;
        while (muPos < msText.size() && !IsSpace(msText[muPos]) && msText[muPos] != '>' && msText[muPos] != '/' &&
               msText[muPos] != '=')
        {
            ++muPos;
        }
        return msText.substr(uStart, muPos - uStart);
    }

    static void SplitName(const std::string& sName, std::string& sPrefix, std::string& sLocal)
    {
        const size_t uColon = sName.find(':');
        sPrefix = uColon == std::string::npos ? std::string() : sName.substr(0, uColon);
        sLocal = uColon == std::string::npos ? sName : sName.substr(uColon + 1);
    }

    bool ParseElement(XmlNode& kNode, const std::map<std::string, std::string>& kParentScope, std::string& sError)
    {
        ++muPos;
        const std::string sName = ReadName();
        if (sName.empty())
        {
            sError = "xml element without a name";
            return false;
        }
        kNode.bElement = true;
        kNode.bCData = false;
        bool bEmpty = false;
        for (;;)
        {
            SkipSpace();
            if (muPos >= msText.size())
            {
                sError = "xml ends inside <" + sName;
                return false;
            }
            if (Starts("/>"))
            {
                muPos += 2;
                bEmpty = true;
                break;
            }
            if (Starts(">"))
            {
                ++muPos;
                break;
            }
            const std::string sAttr = ReadName();
            SkipSpace();
            if (!Starts("="))
            {
                sError = "xml attribute " + sAttr + " without a value";
                return false;
            }
            ++muPos;
            SkipSpace();
            if (muPos >= msText.size() || (msText[muPos] != '"' && msText[muPos] != '\''))
            {
                sError = "xml attribute " + sAttr + " is not quoted";
                return false;
            }
            const char cQuote = msText[muPos];
            const size_t uEnd = msText.find(cQuote, muPos + 1);
            if (uEnd == std::string::npos)
            {
                sError = "xml attribute " + sAttr + " is not closed";
                return false;
            }
            std::string sValue;
            if (!Decode(msText.substr(muPos + 1, uEnd - muPos - 1), true, sValue, sError))
            {
                return false;
            }
            muPos = uEnd + 1;
            kNode.kAttrs.push_back(std::make_pair(sAttr, sValue));
        }
        std::map<std::string, std::string> kScope = kParentScope;
        for (size_t i = 0; i < kNode.kAttrs.size(); ++i)
        {
            const std::string& sAttr = kNode.kAttrs[i].first;
            if (sAttr == "xmlns")
            {
                kScope[""] = kNode.kAttrs[i].second;
            }
            else if (sAttr.compare(0, 6, "xmlns:") == 0)
            {
                kScope[sAttr.substr(6)] = kNode.kAttrs[i].second;
            }
        }
        std::string sPrefix;
        SplitName(sName, sPrefix, kNode.sLocal);
        const std::map<std::string, std::string>::const_iterator itNs = kScope.find(sPrefix);
        kNode.sNs = itNs == kScope.end() ? std::string() : itNs->second;
        if (bEmpty)
        {
            return true;
        }
        std::string sRaw;
        for (;;)
        {
            if (muPos >= msText.size())
            {
                sError = "xml ends inside <" + sName + ">";
                return false;
            }
            if (msText[muPos] != '<')
            {
                sRaw += msText[muPos++];
                continue;
            }
            if (!FlushText(kNode, sRaw, sError))
            {
                return false;
            }
            if (Starts("<!--"))
            {
                if (!SkipPast("-->", sError))
                {
                    return false;
                }
            }
            else if (Starts("<![CDATA["))
            {
                const size_t uEnd = msText.find("]]>", muPos);
                if (uEnd == std::string::npos)
                {
                    sError = "xml CDATA is not closed";
                    return false;
                }
                XmlNode kData;
                kData.bElement = false;
                kData.bCData = true;
                kData.sText = msText.substr(muPos + 9, uEnd - muPos - 9);
                kNode.kChildren.push_back(kData);
                muPos = uEnd + 3;
            }
            else if (Starts("<?"))
            {
                if (!SkipPast("?>", sError))
                {
                    return false;
                }
            }
            else if (Starts("</"))
            {
                muPos += 2;
                const std::string sClose = ReadName();
                SkipSpace();
                if (sClose != sName || !Starts(">"))
                {
                    sError = "xml </" + sClose + "> does not close <" + sName + ">";
                    return false;
                }
                ++muPos;
                return true;
            }
            else
            {
                XmlNode kChild;
                if (!ParseElement(kChild, kScope, sError))
                {
                    return false;
                }
                kNode.kChildren.push_back(kChild);
            }
        }
    }

    static bool FlushText(XmlNode& kNode, std::string& sRaw, std::string& sError)
    {
        if (sRaw.empty())
        {
            return true;
        }
        bool bBlank = true;
        for (size_t i = 0; i < sRaw.size() && bBlank; ++i)
        {
            bBlank = IsSpace(sRaw[i]);
        }
        if (!bBlank)
        {
            XmlNode kText;
            kText.bElement = false;
            kText.bCData = false;
            if (!Decode(sRaw, false, kText.sText, sError))
            {
                return false;
            }
            kNode.kChildren.push_back(kText);
        }
        sRaw.clear();
        return true;
    }

    const std::string& msText;
    size_t muPos;
};

}

const char* const kMsBuildNs = "http://schemas.microsoft.com/developer/msbuild/2003";

std::string Attr(const XmlNode& kNode, const char* pcName)
{
    for (size_t i = 0; i < kNode.kAttrs.size(); ++i)
    {
        if (kNode.kAttrs[i].first == pcName)
        {
            return kNode.kAttrs[i].second;
        }
    }
    return std::string();
}

void Elements(const XmlNode& kNode, std::vector<const XmlNode*>& kOut)
{
    if (!kNode.bElement)
    {
        return;
    }
    kOut.push_back(&kNode);
    for (size_t i = 0; i < kNode.kChildren.size(); ++i)
    {
        Elements(kNode.kChildren[i], kOut);
    }
}

bool IsMsb(const XmlNode& kNode, const char* pcLocal)
{
    return kNode.bElement && kNode.sNs == kMsBuildNs && kNode.sLocal == pcLocal;
}

std::string InnerText(const XmlNode& kNode)
{
    if (!kNode.bElement)
    {
        return kNode.sText;
    }
    std::string sOut;
    for (size_t i = 0; i < kNode.kChildren.size(); ++i)
    {
        sOut += InnerText(kNode.kChildren[i]);
    }
    return sOut;
}

bool ParseXml(const std::string& sText, const std::string& sWhat, XmlNode& kRoot, std::string& sError)
{
    XmlReader kReader(sText);
    if (!kReader.Parse(kRoot, sError))
    {
        sError = sWhat + ": " + sError;
        return false;
    }
    return true;
}

}
