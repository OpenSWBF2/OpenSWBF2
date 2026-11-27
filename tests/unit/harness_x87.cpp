#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <climits>
#include <string>
#include <vector>

#include "gate/gate_keys.h"
#include "hardening/hardening_binary.h"
#include "x87/x87_inventory.h"

namespace
{

class Asm
{
public:
    Asm() : miAddr(0x401000)
    {
    }

    Asm& Text(const std::string& sLine)
    {
        msText += sLine + "\r\n";
        return *this;
    }

    Asm& Fn(const std::string& sName, long long iAddr)
    {
        miAddr = iAddr;
        return Text(sName + ":");
    }

    Asm& Op(const std::string& sMn, const std::string& sOps = std::string())
    {
        std::string s = "  " + X87::Hex8(miAddr) + ": " + sMn;
        if (!sOps.empty())
        {
            s.append(sMn.size() < 12 ? 12 - sMn.size() : 1, ' ');
            s += sOps;
        }
        miAddr += 2;
        return Text(s);
    }

    const std::string& Str() const
    {
        return msText;
    }

private:
    std::string msText;
    long long miAddr;
};

std::string Sha16(const std::string& sTokens)
{
    return Gate::LowerText(Gate::Sha256Hex(sTokens.data(), sTokens.size())).substr(0, 16);
}

void AddMap(X87::MapObjects& kMap, long long iAddr, const std::string& sName, const std::string& sObj,
            bool bFunc = true)
{
    X87::MapEntry e;
    e.sName = sName;
    e.sObj = sObj;
    e.bFunc = bFunc;
    kMap[(unsigned int)iAddr].push_back(e);
}

X87::Measure MeasureOne(const std::string& sText)
{
    std::vector<X87::DisasmFunc> kFuncs;
    std::string sError;
    REQUIRE(X87::ReadDisasmFunctions(sText, 0, kFuncs, sError));
    REQUIRE(kFuncs.size() == 1);
    X87::Function f;
    REQUIRE(X87::ConvertFunction(kFuncs[0], LLONG_MAX, f));
    return X87::MeasureFunction(f, X87::RetKinds(), std::map<std::string, long long>(), std::set<long long>());
}

bool BuildFixture(const std::string& sText, const X87::MapObjects& kMap, const X87::SourceRules& kRules,
                  std::vector<X87::Row>& kRows, std::string& sError)
{
    std::set<long long> kBounds;
    std::vector<X87::DisasmFunc> kFuncs;
    if (!X87::ReadDisasmFunctions(sText, &kBounds, kFuncs, sError))
    {
        return false;
    }
    return X87::BuildInventory(kRules, kMap, kFuncs, kBounds, X87::DecodeFn(), kRows, sError);
}

const X87::Row* FindRow(const std::vector<X87::Row>& kRows, const std::string& sName)
{
    for (size_t i = 0; i < kRows.size(); ++i)
    {
        if (kRows[i].sFunction == sName)
        {
            return &kRows[i];
        }
    }
    return 0;
}

X87::Row MakeRow(const std::string& sObj, const std::string& sFunction, const std::string& sFlags,
                 const std::string& sPattern, const std::string& sVerdict, const std::string& sEvidence)
{
    X87::Row r;
    r.sObj = sObj;
    r.sFunction = sFunction;
    r.sFlags = sFlags;
    r.sPattern = sPattern;
    r.sVerdict = sVerdict;
    r.sEvidence = sEvidence;
    return r;
}

std::string Project(const std::vector<std::string>& kItems)
{
    std::string s = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
                    "<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\r\n"
                    "  <ItemGroup>\r\n";
    for (size_t i = 0; i < kItems.size(); ++i)
    {
        s += "    " + kItems[i] + "\r\n";
    }
    return s + "  </ItemGroup>\r\n</Project>\r\n";
}

std::string Compile(const std::string& sInclude, const std::string& sCondition = std::string())
{
    return "<ClCompile Include=\"" + sInclude + "\"" + (sCondition.empty() ? "" : " Condition=\"" + sCondition + "\"") +
           " />";
}

std::string Reference(const std::string& sInclude, const std::string& sLink)
{
    return "<ProjectReference Include=\"" + sInclude + "\">\r\n      <LinkLibraryDependencies>" + sLink +
           "</LinkLibraryDependencies>\r\n    </ProjectReference>";
}

const char* const kRoot = "C:\\fx";
const char* const kMain = "C:\\fx\\BattlefrontII.vcxproj";

std::vector<std::string> AllSimExceptionItems()
{
    std::vector<std::string> k;
    for (size_t i = 0; i < X87::kSimExceptionCount; ++i)
    {
        k.push_back(Compile(X87::kSimExceptions[i]));
    }
    return k;
}

std::string DoubleBody(Asm& a)
{
    a.Op("fld", "qword ptr [a]").Op("fmul", "qword ptr [b]").Op("fstp", "qword ptr [c]").Op("ret");
    return a.Str();
}

}

TEST_CASE_NAMED(FX87UnitTest1, "Unit::X87::functions are split at map boundaries", "[EngineFilter]")
{
    SUBCASE("labels open functions and other unindented lines close them")
    {
        const std::string sText = "\r\n"
                                  "Dump of file x.exe\r\n"
                                  "\r\n"
                                  "File Type: EXECUTABLE IMAGE\r\n"
                                  "\r\n"
                                  "A:\r\n"
                                  "  00401000: fld         dword ptr [a]\r\n"
                                  "  00401006: call        B\r\n"
                                  "  0040100B: call        dword ptr [__imp_x]\r\n"
                                  "  00401011: jmp         00401020\r\n"
                                  "  00401016: CC CC                                            II\r\n"
                                  "SECTION .text:\r\n"
                                  "  00401018: nop\r\n"
                                  "B:\r\n"
                                  "  00401020: push        ebp\r\n"
                                  "  00401021: jmp         C\r\n"
                                  "C:\r\n"
                                  "D:\r\n"
                                  "  00401030: ret\r\n"
                                  "\r\n"
                                  "  Summary\r\n"
                                  "\r\n"
                                  "        1000 .text\r\n";
        std::set<long long> kBounds;
        std::vector<X87::DisasmFunc> k;
        std::string sError;
        REQUIRE(X87::ReadDisasmFunctions(sText, &kBounds, k, sError));
        REQUIRE(k.size() == 4);
        CHECK(k[0].sName == "A");
        CHECK(k[0].iStart == 0x401000);
        CHECK(k[0].bHasF);
        CHECK(k[0].kLines.size() == 5);
        REQUIRE(k[0].kCalls.size() == 1);
        CHECK(k[0].kCalls[0] == "B");
        CHECK(k[1].sName == "B");
        CHECK(k[1].iStart == 0x401020);
        CHECK_FALSE(k[1].bHasF);
        CHECK(k[1].kLines.empty());
        REQUIRE(k[1].kCalls.size() == 1);
        CHECK(k[1].kCalls[0] == "C");
        CHECK(k[2].sName == "C");
        CHECK(k[2].iStart == -1);
        CHECK(k[3].sName == "D");
        CHECK(k[3].iStart == 0x401030);
        const std::set<long long> kWant = {0x401000, 0x401006, 0x40100B, 0x401011, 0x401016, 0x401020, 0x401021, 0x401030};
        CHECK(kBounds == kWant);
    }
    SUBCASE("map lines give the object of each address")
    {
        const std::string sMap =
            " 0001:00000000 000090a0H .text$di                CODE\r\n"
            " 0001:000090a0       ?CreateCompressedGeometry@BirdParticle@@IAEPAVpcRedPrimitive@@XZ 0040a0a0 f   "
            "pcBirdEffect.obj\r\n"
            " 0001:0000b100       ?Release@PblRef@@QBEGXZ    0040c100 f i pcBlurEffect.obj\r\n"
            " 0001:0042ee32       __tailMerge_steam_api_dll  0082fe32     steam_api:steam_api.dll\r\n"
            " 0002:00000000       __imp_x 00880000     kernel32:KERNEL32.dll\r\n";
        X87::MapObjects kMap;
        std::string sError;
        REQUIRE(X87::ReadMapObjects(sMap, kMap, sError));
        REQUIRE(kMap.size() == 3);
        REQUIRE(kMap.count(0x40a0a0));
        CHECK(kMap[0x40a0a0][0].sObj == "pcBirdEffect.obj");
        CHECK(kMap[0x40a0a0][0].bFunc);
        CHECK(kMap[0x40c100][0].sName == "?Release@PblRef@@QBEGXZ");
        CHECK(kMap[0x40c100][0].sObj == "pcBlurEffect.obj");
        CHECK(kMap[0x40c100][0].bFunc);
        CHECK(kMap[0x82fe32][0].sObj == "steam_api:steam_api.dll");
        CHECK_FALSE(kMap[0x82fe32][0].bFunc);
    }
    SUBCASE("a function takes the object of its own map name or else the first object")
    {
        X87::MapObjects kMap;
        AddMap(kMap, 0x401000, "?A@@", "z.obj");
        AddMap(kMap, 0x401000, "A", "a.obj");
        AddMap(kMap, 0x401020, "?B@@YAXXZ", "m.obj");
        AddMap(kMap, 0x401020, "?B2@@", "k.obj");
        std::string sObj;
        std::string sError;
        REQUIRE(X87::FunctionObj(kMap, 0x401000, "A", sObj, sError));
        CHECK(sObj == "a.obj");
        REQUIRE(X87::FunctionObj(kMap, 0x401020, "B", sObj, sError));
        CHECK(sObj == "k.obj");
        REQUIRE(X87::FunctionObj(kMap, 0x401030, "D", sObj, sError));
        CHECK(sObj == "?");
        CHECK_FALSE(X87::FunctionObj(kMap, -1, "C", sObj, sError));
        CHECK(sError == "x87-inventory: function C has no decoded instruction, so it has no map address");
    }
    SUBCASE("a function ends at the next function and only its own addresses mark data")
    {
        Asm a;
        a.Fn("E", 0x401000).Op("fld", "dword ptr [400FF0h]").Op("fstp", "dword ptr [401006h]");
        a.Op("call", "F").Op("fld1").Op("fldz");
        std::vector<X87::DisasmFunc> k;
        std::string sError;
        REQUIRE(X87::ReadDisasmFunctions(a.Str(), 0, k, sError));
        REQUIRE(k.size() == 1);
        X87::Function f;
        REQUIRE(X87::ConvertFunction(k[0], 0x401020, f));
        REQUIRE(f.kIns.size() == 3);
        CHECK(f.kIns[2].sM == "call");
        REQUIRE(X87::ConvertFunction(k[0], 0x401004, f));
        CHECK(f.kIns.size() == 2);
    }
}

TEST_CASE_NAMED(FX87UnitTest2, "Unit::X87::jump table bytes do not cut a function short", "[EngineFilter]")
{
    SUBCASE("a table line that names an address inside the function is data")
    {
        const std::string sText =
            "?GetAnimationBlendDuration@EntityWalkerClass@@IBEMW4eState@EntityWalker@@0@Z:\r\n"
            "  005261D0: push        ebp\r\n"
            "  005261D1: mov         ebp,esp\r\n"
            "  005261D3: cmp         byte ptr [ecx+604h],0\r\n"
            "  005261DA: push        esi\r\n"
            "  005261DB: je          00526258\r\n"
            "  005261DD: mov         esi,dword ptr [ebp+0Ch]\r\n"
            "  005261E0: cmp         dword ptr [ebp+8],esi\r\n"
            "  005261E3: je          00526258\r\n"
            "  005261E5: or          ecx,0FFFFFFFFh\r\n"
            "  005261E8: or          edx,ecx\r\n"
            "  005261EA: cmp         esi,0Bh\r\n"
            "  005261ED: jl          005261F3\r\n"
            "  005261EF: xor         ecx,ecx\r\n"
            "  005261F1: jmp         00526216\r\n"
            "  005261F3: cmp         esi,0Ah\r\n"
            "  005261F6: ja          00526216\r\n"
            "  005261F8: movzx       eax,byte ptr [esi+526270h]\r\n"
            "  005261FF: jmp         dword ptr [eax*4+526260h]\r\n"
            "  00526206: xor         ecx,ecx\r\n"
            "  00526208: jmp         00526216\r\n"
            "  0052620A: mov         ecx,1\r\n"
            "  0052620F: jmp         00526216\r\n"
            "  00526211: mov         ecx,2\r\n"
            "  00526216: cmp         dword ptr [ebp+8],0Bh\r\n"
            "  0052621A: jl          00526220\r\n"
            "  0052621C: xor         edx,edx\r\n"
            "  0052621E: jmp         00526244\r\n"
            "  00526220: dec         esi\r\n"
            "  00526221: cmp         esi,9\r\n"
            "  00526224: ja          00526244\r\n"
            "  00526226: movzx       eax,byte ptr [esi+52628Ch]\r\n"
            "  0052622D: jmp         dword ptr [eax*4+52627Ch]\r\n"
            "  00526234: xor         edx,edx\r\n"
            "  00526236: jmp         00526244\r\n"
            "  00526238: mov         edx,1\r\n"
            "  0052623D: jmp         00526244\r\n"
            "  0052623F: mov         edx,2\r\n"
            "  00526244: cmp         ecx,edx\r\n"
            "  00526246: jne         0052624D\r\n"
            "  00526248: cmp         ecx,0FFFFFFFFh\r\n"
            "  0052624B: jne         00526258\r\n"
            "  0052624D: fld         dword ptr [__real@3e4ccccd]\r\n"
            "  00526253: pop         esi\r\n"
            "  00526254: pop         ebp\r\n"
            "  00526255: ret         8\r\n"
            "  00526258: fldz\r\n"
            "  0052625A: pop         esi\r\n"
            "  0052625B: pop         ebp\r\n"
            "  0052625C: ret         8\r\n"
            "  0052625F: nop\r\n"
            "  00526260: push        es\r\n"
            "  00526261: bound       edx,qword ptr [edx]\r\n"
            "  00526264: or          ah,byte ptr [edx+52h]\r\n"
            "  00526267: add         byte ptr [ecx],dl\r\n"
            "  00526269: bound       edx,qword ptr [edx]\r\n"
            "  0052626C: push        ss\r\n"
            "  0052626D: bound       edx,qword ptr [edx]\r\n"
            "  00526270: add         byte ptr [eax],al\r\n"
            "  00526272: add         byte ptr [eax],al\r\n"
            "  00526274: add         eax,dword ptr [eax]\r\n"
            "  00526276: add         byte ptr [ecx],al\r\n"
            "  00526278: add         al,byte ptr [ecx]\r\n"
            "  0052627A: add         dl,byte ptr [eax+526234h]\r\n"
            "  00526280: cmp         byte ptr [edx+52h],ah\r\n"
            "  00526283: add         byte ptr [edi],bh\r\n"
            "  00526285: bound       edx,qword ptr [edx]\r\n"
            "  00526288: inc         esp\r\n"
            "  00526289: bound       edx,qword ptr [edx]\r\n"
            "  0052628C: add         byte ptr [eax],al\r\n"
            "  0052628E: add         eax,dword ptr [ebx]\r\n"
            "  00526290: add         dword ptr [edx],eax\r\n"
            "  00526292: add         al,byte ptr [ecx]\r\n"
            "  00526294: add         byte ptr [eax],al\r\n"
            "  00526296: CC CC CC CC CC CC CC CC CC CC                    IIIIIIIIII\r\n"
            "?GetAttachOffset@EntityWalker@@UBEABVPblVector3@@XZ:\r\n"
            "  005262A0: mov         eax,ecx\r\n"
            "  005262A2: ret\r\n";
        std::vector<X87::DisasmFunc> k;
        std::string sError;
        REQUIRE(X87::ReadDisasmFunctions(sText, 0, k, sError));
        REQUIRE(k.size() == 2);
        X87::Function f;
        REQUIRE(X87::ConvertFunction(k[0], k[1].iStart, f));
        std::vector<long long> kF;
        for (size_t i = 0; i < f.kIns.size(); ++i)
        {
            if (f.kIns[i].sM[0] == 'f')
            {
                kF.push_back(f.kIns[i].iA);
            }
            CHECK(f.kIns[i].iA < 0x526260);
        }
        const std::vector<long long> kWant = {0x52624D, 0x526258};
        CHECK(kF == kWant);
        X87::MapObjects kMap;
        AddMap(kMap, 0x5261D0, "?GetAnimationBlendDuration@EntityWalkerClass@@IBEMW4eState@EntityWalker@@0@Z",
               "EntityWalker.obj");
        AddMap(kMap, 0x5262A0, "?GetAttachOffset@EntityWalker@@UBEABVPblVector3@@XZ", "EntityWalker.obj");
        X87::SourceRules kRules;
        kRules.kSources["entitywalker.obj"] = "src\\Battlefront2\\Source\\EntityWalker.cpp";
        std::vector<X87::Row> kRows;
        REQUIRE_MESSAGE(BuildFixture(sText, kMap, kRules, kRows, sError), sError);
        REQUIRE(kRows.size() == 1);
        CHECK(kRows[0].sObj == "EntityWalker.obj");
        CHECK(kRows[0].sFlags == "-");
        CHECK(kRows[0].sPattern == "2:fe6647fd53edca5e");
        CHECK(kRows[0].sVerdict == "match:exact");
    }
    SUBCASE("a reference from code before the data start still marks the data start")
    {
        const std::string sText = "?F2@@YAXXZ:\r\n"
                                  "  00402000: fld         dword ptr [__real@3f800000]\r\n"
                                  "  00402006: fstp        dword ptr [esp]\r\n"
                                  "  00402009: jmp         dword ptr [eax*4+402010h]\r\n"
                                  "  00402010: fadd        dword ptr [eax]\r\n"
                                  "  00402012: add         dl,byte ptr [eax+402004h]\r\n"
                                  "?G@@YAXXZ:\r\n"
                                  "  00402020: ret\r\n";
        std::vector<X87::DisasmFunc> k;
        std::string sError;
        REQUIRE(X87::ReadDisasmFunctions(sText, 0, k, sError));
        REQUIRE(k.size() == 2);
        X87::Function f;
        REQUIRE(X87::ConvertFunction(k[0], k[1].iStart, f));
        REQUIRE(f.kIns.size() == 3);
        CHECK(f.kIns[2].sM == "jmp");
        const X87::Measure m =
            X87::MeasureFunction(f, X87::RetKinds(), std::map<std::string, long long>(), std::set<long long>());
        CHECK(m.sPattern == "2:" + Sha16("fld m32\nfstp m32"));
    }
}

TEST_CASE_NAMED(FX87UnitTest3, "Unit::X87::a lost function is decoded again or reported", "[EngineFilter]")
{
    const std::string sText = "A:\r\n"
                              "  00401000: fld1\r\n"
                              "  00401002: ret\r\n"
                              "Empty:\r\n"
                              "B:\r\n"
                              "  00401010: fldz\r\n"
                              "  00401012: ret\r\n";
    std::set<long long> kBounds;
    std::vector<X87::DisasmFunc> kFuncs;
    std::string sError;
    REQUIRE(X87::ReadDisasmFunctions(sText, &kBounds, kFuncs, sError));
    REQUIRE(kFuncs.size() == 3);
    X87::MapObjects kMap;
    AddMap(kMap, 0x400F00, "Before", "a.obj");
    AddMap(kMap, 0x401000, "A", "a.obj");
    AddMap(kMap, 0x401004, "data", "a.obj", false);
    AddMap(kMap, 0x401008, "Lost", "a.obj");
    AddMap(kMap, 0x401010, "B", "a.obj");
    AddMap(kMap, 0x401100, "Far", "a.obj");
    SUBCASE("without a decoder the lost function is reported")
    {
        CHECK_FALSE(X87::AddLostFunctions(kFuncs, kMap, kBounds, X87::DecodeFn(), sError));
        CHECK(sError == "x87-inventory: map function Lost at 00401008 starts inside a decoded instruction, and "
                        "-Disasm cannot decode it again");
    }
    SUBCASE("a decode that gives nothing or starts elsewhere is reported")
    {
        const X87::DecodeFn fnNothing = [](long long, long long, std::vector<X87::DisasmFunc>&, std::string&) {
            return true;
        };
        CHECK_FALSE(X87::AddLostFunctions(kFuncs, kMap, kBounds, fnNothing, sError));
        CHECK(sError == "x87-inventory: dumpbin did not decode map function Lost at 00401008");
        const X87::DecodeFn fnElsewhere = [](long long, long long, std::vector<X87::DisasmFunc>& kOut,
                                             std::string& sWhy) {
            return X87::ReadDisasmFunctions("Lost:\r\n  00401009: fld1\r\n", 0, kOut, sWhy);
        };
        sError.clear();
        CHECK_FALSE(X87::AddLostFunctions(kFuncs, kMap, kBounds, fnElsewhere, sError));
        CHECK(sError == "x87-inventory: dumpbin did not decode map function Lost at 00401008");
    }
    SUBCASE("a decoded function joins the list in address order")
    {
        long long iFrom = 0;
        long long iTo = 0;
        int iCalls = 0;
        const X87::DecodeFn fnDecode = [&](long long a, long long b, std::vector<X87::DisasmFunc>& kOut,
                                           std::string& sWhy) {
            iFrom = a;
            iTo = b;
            ++iCalls;
            return X87::ReadDisasmFunctions("Lost:\r\n"
                                            "  00401008: fld1\r\n"
                                            "  0040100A: ret\r\n"
                                            "B:\r\n"
                                            "  00401010: fldz\r\n",
                                            0, kOut, sWhy);
        };
        REQUIRE_MESSAGE(X87::AddLostFunctions(kFuncs, kMap, kBounds, fnDecode, sError), sError);
        CHECK(iCalls == 1);
        CHECK(iFrom == 0x401008);
        CHECK(iTo == 0x40100F);
        REQUIRE(kFuncs.size() == 3);
        CHECK(kFuncs[0].sName == "A");
        CHECK(kFuncs[1].sName == "Lost");
        CHECK(kFuncs[1].iStart == 0x401008);
        CHECK(kFuncs[1].bHasF);
        CHECK(kFuncs[2].sName == "B");
    }
    SUBCASE("nothing is decoded when every map function starts on an instruction")
    {
        kMap.erase(0x401008);
        int iCalls = 0;
        const X87::DecodeFn fnDecode = [&iCalls](long long, long long, std::vector<X87::DisasmFunc>&, std::string&) {
            ++iCalls;
            return true;
        };
        REQUIRE(X87::AddLostFunctions(kFuncs, kMap, kBounds, fnDecode, sError));
        CHECK(iCalls == 0);
        CHECK(kFuncs.size() == 3);
        CHECK(kFuncs[1].sName == "Empty");
    }
}

TEST_CASE_NAMED(FX87UnitTest4, "Unit::X87::operand kinds", "[EngineFilter]")
{
    CHECK(X87::OperandKind("st") == "st(0)");
    CHECK(X87::OperandKind(" st(3) ") == "st(3)");
    CHECK(X87::OperandKind("tbyte ptr [esp]") == "m80");
    CHECK(X87::OperandKind("qword ptr [__real@3ff0000000000000]") == "m64");
    CHECK(X87::OperandKind("dword ptr [eax+4]") == "m32");
    CHECK(X87::OperandKind("word ptr [esp+2]") == "m16");
    CHECK(X87::OperandKind("ax") == "ax");
    CHECK(X87::OperandKind("eax") == "eax");
    CHECK(X87::OperandKind("xmmword ptr [eax]") == "m");
    CHECK(X87::OperandKind("st(10)") == "m");
    CHECK(X87::OperandKind("EAX") == "m");
    CHECK(X87::StIndex("st(0)") == 0);
    CHECK(X87::StIndex("st(7)") == 7);
    CHECK(X87::StIndex("m32") == -1);
    CHECK(X87::StIndex("st") == -1);
}

TEST_CASE_NAMED(FX87UnitTest5, "Unit::X87::stack simulation", "[EngineFilter]")
{
    SUBCASE("match:exact for loads and stores")
    {
        Asm a;
        a.Fn("X", 0x401000).Op("fld", "dword ptr [a]").Op("fstp", "dword ptr [b]").Op("ret");
        const X87::Measure m = MeasureOne(a.Str());
        CHECK(m.sFlags == "-");
        CHECK(m.sPattern == "2:b7c35fe36e30caa1");
        CHECK(m.sRule == "match:exact");
        CHECK(m.cRet == '-');
    }
    SUBCASE("match:single-rounding for one float product stored to a dword")
    {
        Asm a;
        a.Fn("X", 0x401000).Op("fld", "dword ptr [a]").Op("fmul", "dword ptr [b]").Op("fstp", "dword ptr [c]");
        a.Op("ret");
        const X87::Measure m = MeasureOne(a.Str());
        CHECK(m.sFlags == "arith");
        CHECK(m.sPattern == "3:" + Sha16("fld m32\nfmul m32\nfstp m32"));
        CHECK(m.sRule == "match:single-rounding");
    }
    SUBCASE("no rule for a double product")
    {
        Asm a;
        a.Fn("X", 0x401000);
        const X87::Measure m = MeasureOne(DoubleBody(a));
        CHECK(m.sFlags == "arith,store_double_precise");
        CHECK(m.sPattern == "3:" + Sha16("fld m64\nfmul m64\nfstp m64"));
        CHECK(m.sRule.empty());
    }
    SUBCASE("an empty stack slot is read as in")
    {
        Asm a;
        a.Fn("X", 0x401000).Op("fstp", "dword ptr [x]").Op("ret");
        const X87::Measure m = MeasureOne(a.Str());
        CHECK(m.sPattern == "1:9e736df4cdd28af8");
        CHECK(m.sRule == "match:exact");
    }
    SUBCASE("labels and conditional jumps")
    {
        Asm a;
        a.Fn("X", 0x401000).Op("fld", "dword ptr [a]").Op("jne", "00401006").Op("fchs").Op("fstp", "dword ptr [b]");
        a.Op("call", "dword ptr [__imp_y]").Op("fldz").Op("fstp", "dword ptr [c]").Op("ret");
        const X87::Measure m = MeasureOne(a.Str());
        CHECK(m.sFlags == "-");
        CHECK(m.sPattern == "5:" + Sha16("fld m32\njcc\nfchs\nlabel\nfstp m32\nfldz\nfstp m32"));
        CHECK(m.sRule == "match:exact");
    }
    SUBCASE("transcendental compare and conversion flags")
    {
        Asm a;
        a.Fn("X", 0x401000).Op("fld", "dword ptr [a]").Op("fsin").Op("fcomp", "dword ptr [b]");
        a.Op("fild", "dword ptr [c]").Op("fistp", "dword ptr [d]").Op("fld", "qword ptr [e]").Op("fsqrt");
        a.Op("fistp", "dword ptr [f]").Op("fxtract").Op("ret");
        const X87::Measure m = MeasureOne(a.Str());
        CHECK(m.sFlags == "arith,cmp_precise,cvt_precise,transc,unknown");
        CHECK(m.sPattern ==
              "9:" + Sha16("fld m32\nfsin\nfcomp m32\nfild m32\nfistp m32\nfld m64\nfsqrt\nfistp m32\nfxtract"));
        CHECK(m.sRule.empty());
    }
    SUBCASE("a chain through a rounded result")
    {
        Asm a;
        a.Fn("X", 0x401000).Op("fld", "dword ptr [a]").Op("fld", "dword ptr [b]").Op("fmulp", "st(1),st");
        a.Op("fadd", "qword ptr [c]").Op("fxch", "st(1)").Op("fstp", "st(0)").Op("fstp", "dword ptr [d]").Op("ret");
        const X87::Measure m = MeasureOne(a.Str());
        CHECK(m.sFlags == "arith,chain");
        CHECK(m.sPattern == "7:" + Sha16("fld m32\nfld m32\nfmulp st(1),st(0)\nfadd m64\nin\nfxch st(1)\nfstp "
                                         "st(0)\nfstp m32"));
    }
    SUBCASE("return kinds pass through calls")
    {
        Asm a;
        a.Fn("E", 0x401000).Op("fld", "dword ptr [a]").Op("ret");
        a.Fn("P", 0x401010).Op("fld", "qword ptr [a]").Op("fmul", "qword ptr [b]").Op("ret");
        a.Fn("CE", 0x401020).Op("call", "E").Op("fstp", "dword ptr [x]").Op("ret");
        a.Fn("CP", 0x401030).Op("call", "00401010").Op("fstp", "dword ptr [x]").Op("ret");
        X87::MapObjects kMap;
        AddMap(kMap, 0x401000, "E", "g.obj");
        AddMap(kMap, 0x401010, "P", "g.obj");
        AddMap(kMap, 0x401020, "CE", "g.obj");
        AddMap(kMap, 0x401030, "CP", "g.obj");
        X87::SourceRules kRules;
        kRules.kSources["g.obj"] = "src\\Game\\g.cpp";
        std::vector<X87::Row> kRows;
        std::string sError;
        REQUIRE_MESSAGE(BuildFixture(a.Str(), kMap, kRules, kRows, sError), sError);
        REQUIRE(kRows.size() == 4);
        CHECK(FindRow(kRows, "E")->sVerdict == "match:exact");
        CHECK(FindRow(kRows, "P")->sFlags == "arith,ret_precise");
        CHECK(FindRow(kRows, "CE")->sFlags == "-");
        CHECK(FindRow(kRows, "CE")->sPattern == "1:e37a302203cc1e4e");
        CHECK(FindRow(kRows, "CP")->sFlags == "callret_precise");
        CHECK(FindRow(kRows, "CP")->sPattern == "1:e37a302203cc1e4e");
        CHECK(FindRow(kRows, "CP")->sVerdict.empty());
    }
    SUBCASE("each offpath verdict and the sim-called nosource object")
    {
        Asm a;
        X87::MapObjects kMap;
        X87::SourceRules kRules;
        const char* const apcObjs[] = {"R0.OBJ", "r1.obj", "r2.obj", "r3.obj", "lgc.obj", "r5.obj", "r6.obj"};
        const char* const apcSrcs[] = {"src\\RedEngineFL\\Graphics\\r0.cpp", "src\\RedEngineFL\\Interface\\r1.cpp",
                                       "src\\RedEngineFL\\movie\\r2.cpp",    "src\\SoundFL\\source\\r3.cpp",
                                       "src\\LuaFL\\src\\lgc.cpp",           "src\\lglcd\\r5.cpp",
                                       "src\\Steam\\r6.cpp"};
        REQUIRE(X87::kOffPathDirCount == 7);
        for (size_t i = 0; i < 7; ++i)
        {
            const std::string sName = "R" + std::to_string(i);
            const long long iAddr = 0x401000 + (long long)i * 0x10;
            a.Fn(sName, iAddr);
            DoubleBody(a);
            AddMap(kMap, iAddr, sName, apcObjs[i]);
            kRules.kSources[Gate::LowerText(apcObjs[i])] = apcSrcs[i];
        }
        a.Fn("S", 0x401100);
        DoubleBody(a);
        AddMap(kMap, 0x401100, "S", "s.obj");
        kRules.kSources["s.obj"] = "src\\RedEngineFL\\Graphics\\s.cpp";
        kRules.kExceptions.insert("src\\redenginefl\\graphics\\s.cpp");
        a.Fn("N", 0x401200);
        DoubleBody(a);
        AddMap(kMap, 0x401200, "N", "crt.obj");
        a.Fn("D", 0x401300);
        DoubleBody(a);
        AddMap(kMap, 0x401300, "D", "steam_api:steam_api.dll");
        a.Fn("G", 0x401400).Op("call", "M").Op("ret");
        AddMap(kMap, 0x401400, "G", "g.obj");
        kRules.kSources["g.obj"] = "src\\Game\\g.cpp";
        a.Fn("M", 0x401500);
        DoubleBody(a);
        AddMap(kMap, 0x401500, "M", "libm.obj");
        a.Fn("X", 0x401600).Op("fld", "dword ptr [a]").Op("fstp", "dword ptr [b]").Op("ret");
        AddMap(kMap, 0x401600, "X", "r1.obj");
        std::vector<X87::Row> kRows;
        std::string sError;
        REQUIRE_MESSAGE(BuildFixture(a.Str(), kMap, kRules, kRows, sError), sError);
        REQUIRE(kRows.size() == 12);
        for (size_t i = 0; i < 7; ++i)
        {
            const X87::Row* r = FindRow(kRows, "R" + std::to_string(i));
            REQUIRE(r != (const X87::Row*)0);
            CHECK(r->sVerdict == std::string("offpath:") + X87::kOffPathDirs[i].pcName);
            CHECK(r->sEvidence == std::string(apcSrcs[i]) + ": " + X87::kOffPathDirs[i].pcReason);
            CHECK(X87::IsRuleVerdict(r->sVerdict));
        }
        CHECK(FindRow(kRows, "S")->sVerdict.empty());
        CHECK(FindRow(kRows, "N")->sVerdict == "offpath:nosource");
        CHECK(FindRow(kRows, "N")->sEvidence == std::string("crt.obj: ") + X87::kNoSourceReason);
        CHECK(FindRow(kRows, "D")->sVerdict == "offpath:nosource");
        CHECK(FindRow(kRows, "M")->sVerdict.empty());
        CHECK(FindRow(kRows, "X")->sVerdict == "match:exact");
        CHECK(FindRow(kRows, "X")->sEvidence == X87::kExactEvidence);
    }
    SUBCASE("a kept hand verdict")
    {
        std::vector<X87::Row> kRows = {MakeRow("a.obj", "F", "arith", "2:aa", "", ""),
                                       MakeRow("a.obj", "G", "arith", "2:bb", "", ""),
                                       MakeRow("a.obj", "H", "arith", "2:cc", "", ""),
                                       MakeRow("a.obj", "K", "-", "1:dd", "match:exact", "E")};
        const std::vector<X87::Row> kOld = {MakeRow("a.obj", "F", "arith", "2:aa", "match:callret", "checked"),
                                            MakeRow("a.obj", "G", "arith", "2:b0", "match:callret", "checked"),
                                            MakeRow("a.obj", "H", "arith", "2:cc", "offpath:sound", "S"),
                                            MakeRow("a.obj", "K", "-", "1:dd", "match:hand", "H")};
        std::string sError;
        REQUIRE(X87::KeepHandVerdicts(kRows, kOld, sError));
        CHECK(kRows[0].sVerdict == "match:callret");
        CHECK(kRows[0].sEvidence == "checked");
        CHECK(kRows[1].sVerdict.empty());
        CHECK(kRows[2].sVerdict.empty());
        CHECK(kRows[3].sVerdict == "match:exact");
        CHECK(X87::CountsLine(kRows) == "x87-inventory: rows=4 exact=1 single-rounding=0 offpath=0 kept=1 open=2");
    }
}

TEST_CASE_NAMED(FX87UnitTest6, "Unit::X87::return kinds settle", "[EngineFilter]")
{
    const auto Chain = [](int n, X87::MapObjects& kMap) {
        Asm a;
        for (int k = 0; k < n; ++k)
        {
            const long long iAddr = 0x401000 + (long long)k * 0x10;
            const std::string sName = "F" + std::to_string(k);
            a.Fn(sName, iAddr).Op("fldz");
            if (k + 1 < n)
            {
                a.Op("jmp", "F" + std::to_string(k + 1));
            }
            else
            {
                a.Op("ret");
            }
            AddMap(kMap, iAddr, sName, "g.obj");
        }
        return a.Str();
    };
    X87::SourceRules kRules;
    kRules.kSources["g.obj"] = "src\\Game\\g.cpp";
    SUBCASE("a chain settles in 50 passes")
    {
        X87::MapObjects kMap;
        const std::string sText = Chain(49, kMap);
        std::vector<X87::Row> kRows;
        std::string sError;
        REQUIRE_MESSAGE(BuildFixture(sText, kMap, kRules, kRows, sError), sError);
        REQUIRE(kRows.size() == 49);
        CHECK(kRows[0].sPattern == "1:" + Sha16("fldz\njmp"));
        CHECK(kRows[48].sPattern == "1:" + Sha16("fldz\nret"));
    }
    SUBCASE("a chain one longer does not")
    {
        X87::MapObjects kMap;
        const std::string sText = Chain(50, kMap);
        std::vector<X87::Row> kRows;
        std::string sError;
        CHECK_FALSE(BuildFixture(sText, kMap, kRules, kRows, sError));
        CHECK(sError == "x87-inventory: return kinds did not settle in 50 passes");
    }
}

TEST_CASE_NAMED(FX87UnitTest7, "Unit::X87::project sources", "[EngineFilter]")
{
    Hardening::MemoryFiles kFiles;
    std::string sError;
    SUBCASE("linked references are followed and other platforms are left out")
    {
        std::string sMain = Project({Compile("src\\Game\\a.cpp"), Compile("src\\Game\\b.cpp", "'$(Platform)'=='Win32'"),
                                     Compile("src\\Game\\c.cpp", " '$(Platform)' == 'x64' "),
                                     Reference("src\\Lib\\Lib.vcxproj", "true"),
                                     Reference("src\\Tool\\Tool.vcxproj", " false ")});
        sMain.insert(sMain.find("</Project>"), "  <ItemGroup Condition=\"'$(Platform)'=='x64'\">\r\n"
                                               "    <ClCompile Include=\"src\\Game\\d.cpp\" />\r\n"
                                               "  </ItemGroup>\r\n"
                                               "  <Target Name=\"T\">\r\n"
                                               "    <ItemGroup><ClCompile Include=\"src\\Game\\e.cpp\" /></ItemGroup>\r\n"
                                               "  </Target>\r\n");
        kFiles.Add(kMain, sMain);
        kFiles.Add("C:\\fx\\src\\Lib\\Lib.vcxproj",
                   Project({Compile("..\\Shared\\f.cpp"), Reference("..\\..\\BattlefrontII.vcxproj", "true")}));
        X87::SourceMap kSources;
        REQUIRE_MESSAGE(X87::ProjectSources(kFiles, kRoot, kMain, kSources, sError), sError);
        const X87::SourceMap kWant = {{"a.obj", "src\\Game\\a.cpp"},
                                      {"b.obj", "src\\Game\\b.cpp"},
                                      {"f.obj", "src\\Shared\\f.cpp"}};
        CHECK(kSources == kWant);
    }
    SUBCASE("a condition the reader does not know fails")
    {
        kFiles.Add(kMain, Project({Compile("src\\Game\\a.cpp", "'$(Configuration)'=='Debug'")}));
        X87::SourceMap kSources;
        CHECK_FALSE(X87::ProjectSources(kFiles, kRoot, kMain, kSources, sError));
        CHECK(sError == "x87-inventory: unknown ClCompile condition '$(Configuration)'=='Debug'");
    }
    SUBCASE("an object built from two sources fails")
    {
        kFiles.Add(kMain, Project({Compile("src\\Game\\a.cpp"), Compile("src\\Game\\a.cpp"),
                                   Compile("src\\Other\\A.cpp")}));
        X87::SourceMap kSources;
        CHECK_FALSE(X87::ProjectSources(kFiles, kRoot, kMain, kSources, sError));
        CHECK(sError == "x87-inventory: A.obj is built from src\\Game\\a.cpp and src\\Other\\A.cpp");
    }
    SUBCASE("a sim exception that is not a ClCompile item fails")
    {
        kFiles.Add(kMain, Project({Compile("src\\Game\\a.cpp")}));
        X87::SourceRules kRules;
        CHECK_FALSE(X87::PrepareSources(kFiles, kRoot, kMain, "", kRules, sError));
        CHECK(sError == "x87-inventory: sim exception src\\RedEngineFL\\Application\\RedTerrain.cpp is not a ClCompile "
                        "item of the game projects");
    }
    SUBCASE("an off-path entry that builds nothing fails")
    {
        kFiles.Add(kMain, Project(AllSimExceptionItems()));
        X87::SourceRules kRules;
        CHECK_FALSE(X87::PrepareSources(kFiles, kRoot, kMain, "", kRules, sError));
        CHECK(sError == "x87-inventory: off-path entry src\\RedEngineFL\\Interface\\ builds no ClCompile item");
    }
    SUBCASE("cause lines add exception files")
    {
        std::vector<std::string> kItems = AllSimExceptionItems();
        const char* const apcMore[] = {"src\\RedEngineFL\\Interface\\RedFont.cpp", "src\\RedEngineFL\\movie\\m.cpp",
                                       "src\\LuaFL\\src\\lgc.cpp",                 "src\\lglcd\\lgLcd.cpp",
                                       "src\\Steam\\gf.cpp",                       "src\\Game\\a.cpp",
                                       "src\\Game\\b.cpp",                         "src\\Game\\zlib.c"};
        for (size_t i = 0; i < sizeof(apcMore) / sizeof(apcMore[0]); ++i)
        {
            kItems.push_back(Compile(apcMore[i]));
        }
        kFiles.Add(kMain, Project(kItems));
        const std::string sCauses = "# Causes\r\n"
                                    "- `x` | C2 | A.cpp drifts; a.cpps is not a file\r\n"
                                    "- `y` | C-X87-INVENTORY-RESYNC | b.cpp\r\n"
                                    "- `z` | C-X64-FOO | zlib.c and gone.cpp\r\n"
                                    "b.cpp outside a cause line\r\n";
        X87::SourceRules kRules;
        REQUIRE_MESSAGE(X87::PrepareSources(kFiles, kRoot, kMain, sCauses, kRules, sError), sError);
        CHECK(kRules.kExceptions.size() == X87::kSimExceptionCount + 2);
        CHECK(kRules.kExceptions.count("src\\game\\a.cpp"));
        CHECK(kRules.kExceptions.count("src\\game\\zlib.c"));
        CHECK_FALSE(kRules.kExceptions.count("src\\game\\b.cpp"));
        CHECK(X87::GetOffPath("a.obj", kRules).bOff == false);
        CHECK(X87::GetOffPath("RedSpotLight.obj", kRules).bOff == false);
        CHECK(X87::GetOffPath("RedFont.obj", kRules).sVerdict == "offpath:ui");
        CHECK(X87::GetOffPath("b.obj", kRules).bOff == false);
    }
}

TEST_CASE_NAMED(FX87UnitTest8, "Unit::X87::inventory file", "[EngineFilter]")
{
    const std::string sHead = std::string(X87::kHeader) + "\r\n";
    std::vector<X87::Row> kRows;
    std::string sError;
    SUBCASE("a missing file and a file without the header fail")
    {
        CHECK_FALSE(X87::ReadInventoryText(false, "", "p.tsv", kRows, sError));
        CHECK(sError == "x87-inventory: p.tsv not found");
        CHECK_FALSE(X87::ReadInventoryText(true, "", "p.tsv", kRows, sError));
        CHECK(sError == "x87-inventory: p.tsv has no header row");
        CHECK_FALSE(X87::ReadInventoryText(true, "obj\tfunction\r\n", "p.tsv", kRows, sError));
        CHECK(sError == "x87-inventory: p.tsv has no header row");
        CHECK_FALSE(X87::ReadInventoryText(true, "OBJ\tfunction\tflags\tpattern\tverdict\tevidence\r\n", "p.tsv", kRows,
                                           sError));
        CHECK(sError == "x87-inventory: p.tsv has no header row");
    }
    SUBCASE("a line without six columns fails")
    {
        const std::string sText = sHead + "a.obj\tf\t-\t1:aa\tmatch:exact\tE\r\nb.obj\tg\t-\t1:bb\tv\r\n";
        CHECK_FALSE(X87::ReadInventoryText(true, sText, "p.tsv", kRows, sError));
        CHECK(sError == "x87-inventory: p.tsv line 3 has 5 columns");
        CHECK_FALSE(X87::ReadInventoryText(true, sHead + "a\tb\tc\td\te\tf\tg\r\n", "p.tsv", kRows, sError));
        CHECK(sError == "x87-inventory: p.tsv line 2 has 7 columns");
    }
    SUBCASE("a row twice fails")
    {
        const std::string sText = sHead + "a.obj\tf\t-\t1:aa\t\t\r\na.obj\tf\t-\t1:ab\t\t\r\n";
        REQUIRE(X87::ReadInventoryText(true, sText, "p.tsv", kRows, sError));
        std::map<std::string, const X87::Row*> kIndex;
        CHECK_FALSE(X87::RowIndex(kRows, kIndex, sError));
        CHECK(sError == "x87-inventory: row a.obj f appears twice");
    }
    SUBCASE("two functions of one name in one object fail")
    {
        Asm a;
        a.Fn("A", 0x401000).Op("fld1").Op("ret").Fn("A", 0x401010).Op("fldz").Op("ret");
        X87::MapObjects kMap;
        AddMap(kMap, 0x401000, "A", "a.obj");
        AddMap(kMap, 0x401010, "A", "a.obj");
        X87::SourceRules kRules;
        CHECK_FALSE(BuildFixture(a.Str(), kMap, kRules, kRows, sError));
        CHECK(sError == "x87-inventory: two functions named A in a.obj");
    }
    SUBCASE("write then read gives the same rows in key order")
    {
        const std::vector<X87::Row> kIn = {MakeRow("b.obj", "g", "arith", "2:bb", "match:single-rounding", "S"),
                                           MakeRow("a.obj", "z", "-", "1:aa", "", ""),
                                           MakeRow("a.obj", "f", "-", "1:ab", "match:exact", "E")};
        const std::string sText = X87::InventoryText(kIn);
        CHECK(sText == sHead + "a.obj\tf\t-\t1:ab\tmatch:exact\tE\r\na.obj\tz\t-\t1:aa\t\t\r\n"
                               "b.obj\tg\tarith\t2:bb\tmatch:single-rounding\tS\r\n");
        REQUIRE(X87::ReadInventoryText(true, sText, "p.tsv", kRows, sError));
        REQUIRE(kRows.size() == 3);
        CHECK(kRows[0].sFunction == "f");
        CHECK(kRows[1].sFunction == "z");
        CHECK(kRows[1].sVerdict.empty());
        CHECK(kRows[2].sObj == "b.obj");
        CHECK(X87::InventoryText(kRows) == sText);
    }
}

TEST_CASE_NAMED(FX87UnitTest9, "Unit::X87::compare", "[EngineFilter]")
{
    const std::vector<X87::Row> kPinned = {MakeRow("a.obj", "f", "-", "1:aa", "match:exact", "E"),
                                           MakeRow("b.obj", "g", "arith", "2:bb", "match:callret", "checked"),
                                           MakeRow("c.obj", "h", "arith", "1:cc", "offpath:sound", "S")};
    std::vector<X87::Row> kFresh = {MakeRow("a.obj", "f", "-", "1:aa", "match:exact", "E"),
                                    MakeRow("b.obj", "g", "arith", "2:bb", "", ""),
                                    MakeRow("c.obj", "h", "arith", "1:cc", "offpath:sound", "S")};
    std::vector<std::string> kBad;
    std::string sError;
    SUBCASE("equal with a hand verdict kept")
    {
        REQUIRE(X87::CompareInventory(kPinned, kFresh, kBad, sError));
        CHECK(kBad.empty());
    }
    SUBCASE("an added row")
    {
        kFresh.push_back(MakeRow("d.obj", "k", "-", "1:dd", "match:exact", "E"));
        REQUIRE(X87::CompareInventory(kPinned, kFresh, kBad, sError));
        const std::vector<std::string> kWant = {"new d.obj k flags=- pattern=1:dd"};
        CHECK(kBad == kWant);
    }
    SUBCASE("a removed row")
    {
        kFresh.pop_back();
        REQUIRE(X87::CompareInventory(kPinned, kFresh, kBad, sError));
        const std::vector<std::string> kWant = {"removed c.obj h"};
        CHECK(kBad == kWant);
    }
    SUBCASE("a changed pattern")
    {
        kFresh[0].sPattern = "1:ab";
        REQUIRE(X87::CompareInventory(kPinned, kFresh, kBad, sError));
        const std::vector<std::string> kWant = {"changed a.obj f flags - -> - pattern 1:aa -> 1:ab"};
        CHECK(kBad == kWant);
    }
    SUBCASE("a rule verdict that differs and a pinned row without a verdict")
    {
        std::vector<X87::Row> kPin = kPinned;
        kPin.push_back(MakeRow("e.obj", "m", "-", "1:ee", "", ""));
        kFresh.push_back(MakeRow("e.obj", "m", "-", "1:ee", "", ""));
        kFresh[1].sVerdict = "match:exact";
        kFresh[1].sEvidence = "E";
        kFresh[2].sVerdict.clear();
        REQUIRE(X87::CompareInventory(kPin, kFresh, kBad, sError));
        const std::vector<std::string> kWant = {"rule verdict b.obj g pinned 'match:callret' generated 'match:exact'",
                                                "rule verdict c.obj h pinned 'offpath:sound' generated ''",
                                                "no verdict e.obj m"};
        CHECK(kBad == kWant);
    }
    SUBCASE("a row twice in either table fails")
    {
        kFresh.push_back(kFresh[0]);
        CHECK_FALSE(X87::CompareInventory(kPinned, kFresh, kBad, sError));
        CHECK(sError == "x87-inventory: row a.obj f appears twice");
    }
    SUBCASE("rule verdicts are the rule names only")
    {
        CHECK(X87::IsRuleVerdict("match:exact"));
        CHECK(X87::IsRuleVerdict("match:single-rounding"));
        CHECK(X87::IsRuleVerdict("offpath:lua-gc"));
        CHECK(X87::IsRuleVerdict("offpath:nosource"));
        CHECK_FALSE(X87::IsRuleVerdict("offpath:render"));
        CHECK_FALSE(X87::IsRuleVerdict("match:callret"));
        CHECK_FALSE(X87::IsRuleVerdict("Match:exact"));
        CHECK_FALSE(X87::IsRuleVerdict("match:exactly"));
        CHECK_FALSE(X87::IsRuleVerdict(""));
    }
}
