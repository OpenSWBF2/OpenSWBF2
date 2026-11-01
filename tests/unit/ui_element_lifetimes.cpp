#include "third_party/doctest/doctest.h"

#include <string.h>
#include <string>

#include "testutil.h"

namespace
{

std::string UiSource(const char* pcRel)
{
    std::string sText;
    REQUIRE(ReadRepoText(pcRel, sText));
    return sText;
}

std::string UiBody(const std::string& sText, const char* pcSignature)
{
    const std::string sOpen = std::string(pcSignature) + "\n{\n";
    size_t uiStart = sText.find(sOpen);
    REQUIRE_MESSAGE(uiStart != std::string::npos, pcSignature);
    REQUIRE_MESSAGE(sText.find(sOpen, uiStart + 1) == std::string::npos, pcSignature);
    size_t uiEnd = sText.find("\n}\n", uiStart);
    REQUIRE_MESSAGE(uiEnd != std::string::npos, pcSignature);
    return sText.substr(uiStart, uiEnd + 2 - uiStart);
}

bool UiHas(const std::string& sText, const char* pcNeedle)
{
    return sText.find(pcNeedle) != std::string::npos;
}

}

TEST_CASE("interface element destruction clears creation pointers" * doctest::test_suite("regression"))
{
    const std::string sText = UiSource("src\\RedEngineFL\\Interface\\RedInterfaceElement.cpp");
    const std::string sDtor = UiBody(sText, "RedInterfaceElement::~RedInterfaceElement()");
    CHECK(UiHas(sDtor,
                "    if (Lua_Callbacks::pCurCreateObject == this)\n"
                "    {\n"
                "        Lua_Callbacks::pCurCreateObject = 0;\n"
                "    }\n"));
    CHECK(UiHas(sDtor,
                "    if (static_cast<RedInterfaceElement*>(Lua_Callbacks::pCurCreateScreenGroup) == this)\n"
                "    {\n"
                "        Lua_Callbacks::pCurCreateScreenGroup = 0;\n"
                "    }\n"));
    CHECK(UiHas(sDtor, "\n    IFScreen::ForgetElement(this);\n"));

    const std::string sReset = UiBody(sText, "void RedInterfaceElement::Reset()");
    CHECK(UiHas(sReset, "\n    sLuaHandles.Clear();\n"));
    CHECK(UiHas(sReset, "\n    Lua_Callbacks::pCurCreateObject = 0;\n"));
    CHECK(UiHas(sReset, "\n    Lua_Callbacks::pCurCreateScreenGroup = 0;\n"));
    CHECK(UiHas(sReset, "\n    Red3DModelElement::sAttachHosts.Clear();\n"));
}

TEST_CASE("screen forget element compacts every screen" * doctest::test_suite("regression"))
{
    const std::string sText = UiSource("src\\RedEngineFL\\Interface\\RedIFScreen.cpp");
    const std::string sBody = UiBody(sText, "void IFScreen::ForgetElement(const RedInterfaceElement* pElement)");
    CHECK(UiHas(sBody, "sAllScreensList"));
    CHECK(UiHas(sBody, "static_cast<const RedInterfaceElement*>(pScreen->mElements[i]) != pElement"));
    CHECK(UiHas(sBody, "pScreen->mElements[j - 1] = pScreen->mElements[j];"));
    CHECK(UiHas(sBody, "--pScreen->mNumElements;"));
    CHECK(UiHas(UiSource("src\\RedEngineFL\\Interface\\RedIFScreen.h"),
                "static void ForgetElement(const RedInterfaceElement* pElement);"));
}

TEST_CASE("model attachment detaches on destruction" * doctest::test_suite("regression"))
{
    const std::string sText = UiSource("src\\RedEngineFL\\Interface\\Red3DModelElement.cpp");
    const std::string sDtor = UiBody(sText, "Red3DModelElement::~Red3DModelElement()");
    CHECK(UiHas(sDtor, "sAttachHosts.Remove(this);"));
    const size_t uiLoop = sDtor.find("sAttachHosts.mSlots[i]");
    REQUIRE(uiLoop != std::string::npos);
    const size_t uiClear = sDtor.find("pHost->m_AttachedObj = 0;", uiLoop);
    CHECK(uiClear != std::string::npos);
    CHECK(UiHas(sDtor, "pHost->m_AttachedObj == this"));

    const std::string sAttach =
        UiBody(sText, "bool Red3DModelElement::AttachModel(Red3DModelElement* pAttached, unsigned int uiHardpointHash)");
    CHECK(UiHas(sAttach, "if (!pAttached || !sAttachHosts.Insert(this))\n    {\n        return false;"));
    CHECK(UiHas(sAttach, "m_AttachedObj = pAttached;"));
    CHECK(UiHas(sAttach, "pAttached->m_bAmAttached = true;"));

    const std::string sIFace = UiSource("src\\Battlefront2\\Source\\LuaCallbacks_IFace.cpp");
    const std::string sCallback = UiBody(sIFace, "int ScriptCB_IFModel_AttachModel(lua_State* L)");
    CHECK(UiHas(sCallback, "->AttachModel("));
    CHECK_FALSE(UiHas(sCallback, "m_AttachedObj ="));
    CHECK_FALSE(UiHas(sCallback, "m_bAmAttached ="));
}
