#include "x87/x87_sim.h"

#include <ctype.h>

#include <deque>

#include "gate/gate_keys.h"

namespace X87
{

const OffPathDir kOffPathDirs[] = {
    {"src\\RedEngineFL\\Graphics\\", "renderer",
     "D3D9 renderer: turns sim state into GPU work and never writes sim state"},
    {"src\\RedEngineFL\\Interface\\", "ui", "UI toolkit and fonts: screen layout and glyph metrics, not sim state"},
    {"src\\RedEngineFL\\movie\\", "movie", "Bink movie playback, outside the mission simulation"},
    {"src\\SoundFL\\", "sound", "audio engine: its float results go to the sound device, not to sim state"},
    {"src\\LuaFL\\src\\lgc.cpp", "lua-gc",
     "Lua collector pacing (fidiv on the GC threshold): decides when to collect, never a script value"},
    {"src\\lglcd\\", "lcd", "Logitech LCD applet output"},
    {"src\\Steam\\", "platform-services", "Steam and Galaxy services: auth, lobby and transport, no sim arithmetic"},
};
const size_t kOffPathDirCount = sizeof(kOffPathDirs) / sizeof(kOffPathDirs[0]);

const char* const kNoSourceName = "nosource";
const char* const kNoSourceReason =
    "no ClCompile item of the game projects builds this object (CRT, import library or vendor code), and no game "
    "function outside the off-path directories calls it, directly or through other such objects";

const char* const kSimExceptions[] = {
    "src\\RedEngineFL\\Application\\RedTerrain.cpp",
    "src\\Battlefront2\\Source\\WeaponAreaEffect.cpp",
    "src\\Battlefront2\\Source\\EntityLight.cpp",
    "src\\PebbleFL\\Common\\PblQuaternion.cpp",
    "src\\Battlefront2\\Source\\AIUtil.cpp",
    "src\\Battlefront2\\Source\\Cloth.cpp",
    "src\\Battlefront2\\Source\\CombatUtil.cpp",
    "src\\Battlefront2\\Source\\Combo.cpp",
    "src\\Battlefront2\\Source\\EntityDroideka.cpp",
    "src\\Battlefront2\\Source\\EntitySoldier.cpp",
    "src\\Battlefront2\\Source\\EntityWalker.cpp",
    "src\\Battlefront2\\Source\\GameLoop.cpp",
    "src\\Battlefront2\\Source\\MountedTurret.cpp",
    "src\\Battlefront2\\Source\\OrdnanceEmitter.cpp",
    "src\\Battlefront2\\Source\\SoldierAnimator.cpp",
    "src\\Battlefront2\\Source\\SoldierAnimatorLowRes.cpp",
    "src\\Battlefront2\\Source\\SpaceAssault.cpp",
    "src\\Battlefront2\\Source\\TentacleSimulator.cpp",
    "src\\Battlefront2\\Source\\Trackable.cpp",
    "src\\Battlefront2\\Source\\WalkerLegPair.cpp",
    "src\\Battlefront2\\Source\\WeaponCannon.cpp",
    "src\\RedEngineFL\\Graphics\\RedSpotLight.cpp",
    "src\\RedEngineFL\\Application\\RedWater.cpp",
    "src\\RedEngineFL\\Graphics\\PC\\pcRedRenderer.cpp",
    "src\\RedEngineFL\\Graphics\\RedCamera.cpp",
    "src\\Battlefront2\\Platform\\Pc\\pcWeatherEffect.cpp",
    "src\\Battlefront2\\Source\\FirstPerson.cpp",
    "src\\Battlefront2\\Source\\CameraManager.cpp",
    "src\\Battlefront2\\Source\\FreeCamera.cpp",
    "src\\Battlefront2\\Source\\HUDElementMap.cpp",
    "src\\Battlefront2\\Source\\HUDElementTarget.cpp",
    "src\\SoundFL\\source\\snddsp.cpp",
    "src\\Battlefront2\\Platform\\Pc\\pcColorRamp.cpp",
    "src\\Battlefront2\\Source\\ChaseCamera.cpp",
    "src\\RedEngineFL\\Graphics\\PC\\Shaders\\pcRefractionShader.cpp",
    "src\\RedEngineFL\\Util\\RedScreenShot.cpp",
    "src\\LuaFL\\src\\lib\\lmathlib.cpp",
    "src\\Battlefront2\\Source\\RepairHelper.cpp",
    "src\\PebbleFL\\Common\\PblMath.cpp",
    "src\\Battlefront2\\Source\\ParticleGeometry.cpp",
    "src\\Battlefront2\\Platform\\Pc\\pcHeatShimmerEffect.cpp",
};
const size_t kSimExceptionCount = sizeof(kSimExceptions) / sizeof(kSimExceptions[0]);

const char* const kExactEvidence = "x87 only loads (float, double, integer), stores, moves and sign ops: the only "
                                   "rounding is a store, which rounds once like the SSE load, store or conversion, "
                                   "at every precision";
const char* const kSingleEvidence = "each x87 result is one + - * / or sqrt of two float operands loaded in its block "
                                    "and stored once to a dword: 53 >= 2*24+2, so the double rounding equals one "
                                    "float rounding (SSE single)";

namespace
{

const char* const kFlagOrder[] = {"arith",  "callret_precise",      "chain",  "cmp_precise", "cvt_precise",
                                  "ret_precise", "store_double_precise", "transc", "unknown"};

const char* const kArith2F[] = {"fadd", "faddp", "fsub", "fsubp", "fsubr", "fsubrp",
                                "fmul", "fmulp", "fdiv", "fdivp", "fdivr", "fdivrp"};
const char* const kArith2I[] = {"fiadd", "fisub", "fisubr", "fimul", "fidiv", "fidivr"};
const char* const kArith1[] = {"fsqrt", "frndint", "fscale", "fprem", "fprem1"};
const char* const kPassive[] = {"ffree",  "fincstp", "fdecstp", "fnop",   "fwait",   "fnstcw", "fstcw", "fldcw",
                                "fnstsw", "fstsw",   "fninit",  "fnclex", "fclex",   "fxsave", "fxrstor", "fnsave",
                                "fsave",  "frstor",  "fldenv",  "fnstenv", "fstenv", "femms"};
const char* const kExact[] = {"fld",  "fild", "fst",  "fstp", "fxch",  "fldz",    "fld1",
                              "fchs", "fabs", "ffree", "fnop", "fwait", "fincstp", "fdecstp"};
const char* const kSingleExtra[] = {"fadd", "faddp", "fsub", "fsubp", "fsubr",  "fsubrp", "fmul",
                                    "fmulp", "fdiv", "fdivp", "fdivr", "fdivrp", "fsqrt"};

struct NamedInt
{
    const char* pcName;
    int iValue;
};

const NamedInt kTransc[] = {{"fsin", 0},   {"fcos", 0},   {"f2xm1", 0}, {"fsincos", 1},
                            {"fptan", 2},  {"fpatan", -1}, {"fyl2x", -1}, {"fyl2xp1", -1}};
const NamedInt kConsts[] = {{"fldz", 'E'},   {"fld1", 'E'},   {"fldpi", 'K'}, {"fldl2e", 'K'},
                            {"fldl2t", 'K'}, {"fldlg2", 'K'}, {"fldln2", 'K'}};
const NamedInt kCompare[] = {{"fcom", 0},  {"fcomp", 1},  {"fcompp", 2}, {"fucom", 0},  {"fucomp", 1},
                             {"fucompp", 2}, {"fcomi", 0}, {"fcomip", 1}, {"fucomi", 0}, {"fucomip", 1},
                             {"ftst", 0},  {"ficom", 0},  {"ficomp", 1}};

template <size_t N>
bool InList(const char* const (&apcList)[N], const std::string& s)
{
    for (size_t i = 0; i < N; ++i)
    {
        if (s == apcList[i])
        {
            return true;
        }
    }
    return false;
}

template <size_t N>
bool InListNoCase(const char* const (&apcList)[N], const std::string& s)
{
    for (size_t i = 0; i < N; ++i)
    {
        if (Gate::EqualNoCase(s, apcList[i]))
        {
            return true;
        }
    }
    return false;
}

template <size_t N>
bool FindNoCase(const NamedInt (&akList)[N], const std::string& s, int& iValue)
{
    for (size_t i = 0; i < N; ++i)
    {
        if (Gate::EqualNoCase(s, akList[i].pcName))
        {
            iValue = akList[i].iValue;
            return true;
        }
    }
    return false;
}

bool InExact(const std::string& s)
{
    return InList(kExact, s);
}

bool InSingle(const std::string& s)
{
    return InList(kExact, s) || InList(kSingleExtra, s);
}

bool IsStOperand(const std::string& s)
{
    return s.size() == 5 && s.compare(0, 3, "st(") == 0 && s[3] >= '0' && s[3] <= '9' && s[4] == ')';
}

bool IsRegisterName(const std::string& s)
{
    if (s.size() < 2 || s.size() > 3)
    {
        return false;
    }
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] < 'a' || s[i] > 'z')
        {
            return false;
        }
    }
    return true;
}

struct Slot
{
    char cKind;
    int iBlock;
};

Slot MakeSlot(char cKind, int iBlock)
{
    Slot s;
    s.cKind = cKind;
    s.iBlock = iBlock;
    return s;
}

const Slot kUnknown = MakeSlot('U', -1);

class Stack
{
public:
    Stack() : mbIn(false)
    {
    }

    Slot Get(int i)
    {
        if (i < (int)mkSlots.size())
        {
            return mkSlots[(size_t)i];
        }
        mbIn = true;
        return kUnknown;
    }

    void Set(int i, const Slot& v)
    {
        while ((int)mkSlots.size() <= i)
        {
            mkSlots.push_back(kUnknown);
        }
        mkSlots[(size_t)i] = v;
    }

    void Push(const Slot& v)
    {
        mkSlots.push_front(v);
    }

    void Pop()
    {
        mkSlots.pop_front();
    }

    bool Empty() const
    {
        return mkSlots.empty();
    }

    void Clear()
    {
        mkSlots.clear();
    }

    const Slot& Top() const
    {
        return mkSlots.front();
    }

    bool HasR() const
    {
        for (size_t i = 0; i < mkSlots.size(); ++i)
        {
            if (mkSlots[i].cKind == 'R')
            {
                return true;
            }
        }
        return false;
    }

    bool mbIn;

private:
    std::deque<Slot> mkSlots;
};

bool Precise(const Slot& s)
{
    return s.cKind == 'R' || s.cKind == 'P';
}

bool FloatInBlock(const Slot& s, int iBlock)
{
    return s.cKind == 'E' && s.iBlock == iBlock;
}

std::vector<std::string> SplitComma(const std::string& s)
{
    std::vector<std::string> k;
    size_t uStart = 0;
    for (;;)
    {
        const size_t u = s.find(',', uStart);
        if (u == std::string::npos)
        {
            k.push_back(s.substr(uStart));
            return k;
        }
        k.push_back(s.substr(uStart, u - uStart));
        uStart = u + 1;
    }
}

long long HexOf(const std::string& s)
{
    long long v = 0;
    for (size_t i = 0; i < s.size(); ++i)
    {
        const char c = s[i];
        v = v * 16 + (c >= 'A' ? c - 'A' + 10 : c - '0');
    }
    return v;
}

bool RetIs(const RetKinds& kRet, long long t, char c)
{
    RetKinds::const_iterator it = kRet.find(t);
    return it != kRet.end() && it->second == c;
}

}

std::string OperandKind(const std::string& sOp)
{
    const std::string o = TrimSpace(sOp);
    if (o == "st")
    {
        return "st(0)";
    }
    if (IsStOperand(o))
    {
        return o;
    }
    if (o.compare(0, 9, "tbyte ptr") == 0)
    {
        return "m80";
    }
    if (o.compare(0, 9, "qword ptr") == 0)
    {
        return "m64";
    }
    if (o.compare(0, 9, "dword ptr") == 0)
    {
        return "m32";
    }
    if (o.compare(0, 8, "word ptr") == 0)
    {
        return "m16";
    }
    if (IsRegisterName(o))
    {
        return o;
    }
    return "m";
}

int StIndex(const std::string& sKind)
{
    if (IsStOperand(sKind))
    {
        return sKind[3] - '0';
    }
    return -1;
}

Measure MeasureFunction(const Function& kFn, const RetKinds& kRet, const std::map<std::string, long long>& kByName,
                        const std::set<long long>& kByAddr)
{
    std::set<std::string> kFlags;
    bool bRetE = true;
    bool bRetSeen = false;
    std::vector<std::string> kTokens;
    Stack st;
    int iBlock = 0;
    bool bSingle = true;
    bool bExact = true;
    int iCount = 0;
    bool bFirst = true;
    for (size_t n = 0; n < kFn.kIns.size(); ++n)
    {
        const Ins& p = kFn.kIns[n];
        if (!bFirst && kFn.kLabels.count(p.iA))
        {
            if (st.HasR())
            {
                bSingle = false;
            }
            if (!st.Empty())
            {
                kTokens.push_back("label");
            }
            ++iBlock;
        }
        bFirst = false;
        const std::string& mn = p.sM;
        const std::string& ops = p.sO;
        if (tolower((unsigned char)mn[0]) != 'f')
        {
            if (mn == "call")
            {
                if (st.HasR())
                {
                    bSingle = false;
                }
                if (!st.Empty())
                {
                    kTokens.push_back("call");
                }
                st.Clear();
                bool bT = false;
                long long t = 0;
                std::map<std::string, long long>::const_iterator it = kByName.find(ops);
                if (it != kByName.end())
                {
                    bT = true;
                    t = it->second;
                }
                else if (IsHex8Upper(ops) && kByAddr.count(HexOf(ops)))
                {
                    bT = true;
                    t = HexOf(ops);
                }
                if (bT && RetIs(kRet, t, 'P'))
                {
                    st.Push(MakeSlot('P', iBlock));
                    kFlags.insert("callret_precise");
                }
                else if (bT && RetIs(kRet, t, 'E'))
                {
                    st.Push(MakeSlot('E', iBlock));
                }
            }
            else if (mn == "ret")
            {
                if (!st.Empty() && Precise(st.Top()))
                {
                    kFlags.insert("ret_precise");
                }
                if (!st.Empty())
                {
                    kTokens.push_back("ret");
                    bRetSeen = true;
                    if (!FloatInBlock(st.Top(), iBlock))
                    {
                        bRetE = false;
                    }
                }
                st.Clear();
                ++iBlock;
            }
            else if (mn == "jmp")
            {
                if (st.HasR())
                {
                    bSingle = false;
                }
                if (!st.Empty())
                {
                    kTokens.push_back("jmp");
                }
                bool bT = false;
                long long t = 0;
                std::map<std::string, long long>::const_iterator it = kByName.find(ops);
                if (it != kByName.end())
                {
                    bT = true;
                    t = it->second;
                }
                else if (IsHex8Upper(ops))
                {
                    const long long a = HexOf(ops);
                    if (!kFn.kLabels.count(a) && kByAddr.count(a))
                    {
                        bT = true;
                        t = a;
                    }
                }
                if (bT && RetIs(kRet, t, 'P'))
                {
                    kFlags.insert("ret_precise");
                }
                if (!bT || !RetIs(kRet, t, 'E'))
                {
                    bRetE = false;
                }
                else
                {
                    bRetSeen = true;
                }
                st.Clear();
                ++iBlock;
            }
            else
            {
                if (st.HasR())
                {
                    bSingle = false;
                }
                if (!st.Empty())
                {
                    kTokens.push_back("jcc");
                }
            }
            continue;
        }
        ++iCount;
        std::vector<std::string> kParts;
        if (!ops.empty())
        {
            const std::vector<std::string> kRaw = SplitComma(ops);
            for (size_t i = 0; i < kRaw.size(); ++i)
            {
                kParts.push_back(OperandKind(kRaw[i]));
            }
        }
        std::string sTok = mn;
        if (!kParts.empty())
        {
            sTok += " " + Gate::JoinText(kParts, ",");
        }
        if (!InExact(mn))
        {
            bExact = false;
        }
        if (!InSingle(mn))
        {
            bSingle = false;
        }
        st.mbIn = false;
        int iValue = 0;
        if (mn == "fld")
        {
            const int i = kParts.empty() ? -1 : StIndex(kParts[0]);
            if (i >= 0)
            {
                st.Push(st.Get(i));
            }
            else if (!kParts.empty() && kParts[0] == "m32")
            {
                st.Push(MakeSlot('E', iBlock));
            }
            else
            {
                st.Push(MakeSlot('D', iBlock));
            }
        }
        else if (mn == "fild" || mn == "fbld")
        {
            st.Push(MakeSlot('I', iBlock));
        }
        else if (FindNoCase(kConsts, mn, iValue))
        {
            st.Push(MakeSlot((char)iValue, iBlock));
        }
        else if (mn == "fst" || mn == "fstp")
        {
            const Slot v = st.Get(0);
            const int i = kParts.empty() ? -1 : StIndex(kParts[0]);
            if (i >= 0)
            {
                st.Set(i, v);
            }
            else if (Precise(v) && !kParts.empty() && (kParts[0] == "m64" || kParts[0] == "m80"))
            {
                kFlags.insert("store_double_precise");
            }
            else if (v.cKind == 'R' && !(!kParts.empty() && kParts[0] == "m32"))
            {
                bSingle = false;
            }
            if (mn == "fstp" && !st.Empty())
            {
                st.Pop();
            }
        }
        else if (mn == "fist" || mn == "fistp" || mn == "fisttp" || mn == "fbstp")
        {
            if (Precise(st.Get(0)))
            {
                kFlags.insert("cvt_precise");
            }
            if (mn != "fist" && !st.Empty())
            {
                st.Pop();
            }
        }
        else if (InListNoCase(kArith2F, mn) || InListNoCase(kArith2I, mn))
        {
            kFlags.insert("arith");
            const bool bPop = !mn.empty() && mn[mn.size() - 1] == 'p';
            std::vector<int> kSi;
            for (size_t i = 0; i < kParts.size(); ++i)
            {
                kSi.push_back(StIndex(kParts[i]));
            }
            int d = 0;
            Slot a = kUnknown;
            Slot b = kUnknown;
            if (kParts.size() == 2 && kSi[0] >= 0 && kSi[1] >= 0)
            {
                d = kSi[0];
                a = st.Get(kSi[0]);
                b = st.Get(kSi[1]);
            }
            else if (kParts.empty())
            {
                d = 1;
                a = st.Get(0);
                b = st.Get(1);
            }
            else if (kParts.size() == 1 && kSi[0] >= 0)
            {
                d = 0;
                a = st.Get(0);
                b = st.Get(kSi[0]);
            }
            else
            {
                d = 0;
                a = st.Get(0);
                if (InListNoCase(kArith2I, mn))
                {
                    b = MakeSlot('I', iBlock);
                }
                else if (kParts[0] == "m32")
                {
                    b = MakeSlot('E', iBlock);
                }
                else
                {
                    b = MakeSlot('D', iBlock);
                }
            }
            if (Precise(a) || Precise(b))
            {
                kFlags.insert("chain");
            }
            Slot r = kUnknown;
            if (FloatInBlock(a, iBlock) && FloatInBlock(b, iBlock))
            {
                r = MakeSlot('R', iBlock);
            }
            else
            {
                r = MakeSlot('P', iBlock);
                bSingle = false;
            }
            st.Set(d, r);
            if (bPop && !st.Empty())
            {
                st.Pop();
            }
        }
        else if (InList(kArith1, mn))
        {
            kFlags.insert("arith");
            const Slot a = st.Get(0);
            if (Precise(a))
            {
                kFlags.insert("chain");
            }
            if (mn == "fsqrt" && FloatInBlock(a, iBlock))
            {
                st.Set(0, MakeSlot('R', iBlock));
            }
            else
            {
                st.Set(0, MakeSlot('P', iBlock));
                bSingle = false;
            }
        }
        else if (FindNoCase(kTransc, mn, iValue))
        {
            kFlags.insert("transc");
            st.Get(0);
            if (iValue == -1)
            {
                st.Get(1);
                st.Set(1, MakeSlot('P', iBlock));
                st.Pop();
            }
            else
            {
                st.Set(0, MakeSlot('P', iBlock));
                if (iValue == 1)
                {
                    st.Push(MakeSlot('P', iBlock));
                }
                else if (iValue == 2)
                {
                    st.Push(MakeSlot('E', iBlock));
                }
            }
        }
        else if (mn == "fchs" || mn == "fabs")
        {
            st.Set(0, st.Get(0));
        }
        else if (mn == "fxch")
        {
            int i = kParts.empty() ? 1 : StIndex(kParts[0]);
            if (i < 0)
            {
                i = 1;
            }
            const Slot a = st.Get(0);
            const Slot b = st.Get(i);
            st.Set(0, b);
            st.Set(i, a);
        }
        else if (FindNoCase(kCompare, mn, iValue))
        {
            int i = 1;
            bool bReg = true;
            if (!kParts.empty())
            {
                i = StIndex(kParts[kParts.size() - 1]);
                if (i < 0)
                {
                    bReg = false;
                    i = 1;
                }
            }
            if (mn == "ftst" || mn == "ficom" || mn == "ficomp")
            {
                bReg = false;
            }
            const Slot a = st.Get(0);
            bool bHit = Precise(a);
            if (bReg && Precise(st.Get(i)))
            {
                bHit = true;
            }
            if (bHit)
            {
                kFlags.insert("cmp_precise");
            }
            for (int k = 0; k < iValue; ++k)
            {
                if (!st.Empty())
                {
                    st.Pop();
                }
            }
        }
        else if (mn.compare(0, 5, "fcmov") == 0)
        {
            int i = kParts.empty() ? 1 : StIndex(kParts[kParts.size() - 1]);
            if (i < 0)
            {
                i = 1;
            }
            if (Precise(st.Get(i)))
            {
                st.Set(0, MakeSlot('P', iBlock));
            }
        }
        else if (InList(kPassive, mn))
        {
        }
        else
        {
            kFlags.insert("unknown");
        }
        if (st.mbIn)
        {
            kTokens.push_back("in");
        }
        kTokens.push_back(sTok);
    }
    if (st.HasR())
    {
        bSingle = false;
    }
    std::vector<std::string> kNames;
    for (size_t i = 0; i < sizeof(kFlagOrder) / sizeof(kFlagOrder[0]); ++i)
    {
        if (kFlags.count(kFlagOrder[i]))
        {
            kNames.push_back(kFlagOrder[i]);
        }
    }
    Measure r;
    r.sFlags = kNames.empty() ? std::string("-") : Gate::JoinText(kNames, ",");
    const std::string sJoined = Gate::JoinText(kTokens, "\n");
    const std::string sHex = Gate::LowerText(Gate::Sha256Hex(sJoined.data(), sJoined.size())).substr(0, 16);
    r.sPattern = std::to_string(iCount) + ":" + sHex;
    if (r.sFlags == "-" && bExact)
    {
        r.sRule = "match:exact";
    }
    else if (r.sFlags == "arith" && bSingle)
    {
        r.sRule = "match:single-rounding";
    }
    r.cRet = '-';
    if (kFlags.count("ret_precise"))
    {
        r.cRet = 'P';
    }
    else if (bRetE && bRetSeen)
    {
        r.cRet = 'E';
    }
    return r;
}

bool SettleReturns(const std::vector<Function>& kFns, const std::map<std::string, long long>& kByName,
                   const std::set<long long>& kByAddr, int iPasses, std::vector<Measure>& kOut, std::string& sError)
{
    kOut.assign(kFns.size(), Measure());
    RetKinds kRet;
    bool bChanged = false;
    for (int it = 0; it < iPasses; ++it)
    {
        bChanged = false;
        for (size_t i = 0; i < kFns.size(); ++i)
        {
            const Measure r = MeasureFunction(kFns[i], kRet, kByName, kByAddr);
            kOut[i] = r;
            RetKinds::iterator f = kRet.find(kFns[i].iStart);
            if (f == kRet.end() || f->second != r.cRet)
            {
                kRet[kFns[i].iStart] = r.cRet;
                bChanged = true;
            }
        }
        if (!bChanged)
        {
            break;
        }
    }
    if (bChanged)
    {
        sError = "x87-inventory: return kinds did not settle in " + std::to_string(iPasses) + " passes";
        return false;
    }
    return true;
}

bool PrepareSources(const Hardening::BuildFiles& kFiles, const std::string& sRoot, const std::string& sProject,
                    const std::string& sCauses, SourceRules& kOut, std::string& sError)
{
    kOut = SourceRules();
    if (!ProjectSources(kFiles, sRoot, sProject, kOut.kSources, sError) ||
        !CauseFiles(sCauses, kOut.kSources, kOut.kExceptions, sError))
    {
        return false;
    }
    for (size_t e = 0; e < kSimExceptionCount; ++e)
    {
        bool bBuilt = false;
        for (SourceMap::const_iterator it = kOut.kSources.begin(); it != kOut.kSources.end() && !bBuilt; ++it)
        {
            bBuilt = Gate::EqualNoCase(it->second, kSimExceptions[e]);
        }
        if (!bBuilt)
        {
            sError = std::string("x87-inventory: sim exception ") + kSimExceptions[e] +
                     " is not a ClCompile item of the game projects";
            return false;
        }
        kOut.kExceptions.insert(Gate::LowerText(kSimExceptions[e]));
    }
    for (size_t d = 0; d < kOffPathDirCount; ++d)
    {
        const std::string sPath = kOffPathDirs[d].pcPath;
        const bool bDir = sPath[sPath.size() - 1] == '\\';
        bool bOk = false;
        for (SourceMap::const_iterator it = kOut.kSources.begin(); it != kOut.kSources.end() && !bOk; ++it)
        {
            bOk = bDir ? Gate::StartsWithNoCase(it->second, sPath) : Gate::EqualNoCase(it->second, sPath);
        }
        if (!bOk)
        {
            sError = "x87-inventory: off-path entry " + sPath + " builds no ClCompile item";
            return false;
        }
    }
    return true;
}

OffPath GetOffPath(const std::string& sObj, const SourceRules& kRules)
{
    OffPath k;
    k.bOff = false;
    k.bNoSource = false;
    SourceMap::const_iterator it = kRules.kSources.find(Gate::LowerText(sObj));
    if (sObj.find(':') != std::string::npos || it == kRules.kSources.end())
    {
        k.bOff = true;
        k.sVerdict = std::string("offpath:") + kNoSourceName;
        k.sEvidence = sObj + ": " + kNoSourceReason;
        k.bNoSource = true;
        return k;
    }
    const std::string& sSrc = it->second;
    if (kRules.kExceptions.count(Gate::LowerText(sSrc)))
    {
        return k;
    }
    for (size_t d = 0; d < kOffPathDirCount; ++d)
    {
        const std::string sPath = kOffPathDirs[d].pcPath;
        const bool bHit = sPath[sPath.size() - 1] == '\\' ? Gate::StartsWithNoCase(sSrc, sPath)
                                                           : Gate::EqualNoCase(sSrc, sPath);
        if (bHit)
        {
            k.bOff = true;
            k.sVerdict = std::string("offpath:") + kOffPathDirs[d].pcName;
            k.sEvidence = sSrc + ": " + kOffPathDirs[d].pcReason;
            return k;
        }
    }
    return k;
}

std::set<std::string> SimCalledNoSource(const std::vector<DisasmFunc>& kFuncs, const std::vector<std::string>& kObjs,
                                        const SourceRules& kRules)
{
    std::map<std::string, std::vector<size_t> > kByName;
    std::map<std::string, OffPath> kOff;
    std::deque<size_t> kQueue;
    for (size_t i = 0; i < kFuncs.size(); ++i)
    {
        kByName[kFuncs[i].sName].push_back(i);
        const std::string sKey = Gate::LowerText(kObjs[i]);
        if (!kOff.count(sKey))
        {
            kOff[sKey] = GetOffPath(kObjs[i], kRules);
        }
        if (!kOff[sKey].bOff)
        {
            kQueue.push_back(i);
        }
    }
    std::set<size_t> kReached;
    while (!kQueue.empty())
    {
        const DisasmFunc& f = kFuncs[kQueue.front()];
        kQueue.pop_front();
        for (size_t c = 0; c < f.kCalls.size(); ++c)
        {
            std::map<std::string, std::vector<size_t> >::const_iterator it = kByName.find(f.kCalls[c]);
            if (it == kByName.end())
            {
                continue;
            }
            for (size_t n = 0; n < it->second.size(); ++n)
            {
                const size_t j = it->second[n];
                const OffPath& o = kOff[Gate::LowerText(kObjs[j])];
                if (o.bOff && o.bNoSource && kReached.insert(j).second)
                {
                    kQueue.push_back(j);
                }
            }
        }
    }
    std::set<std::string> kNames;
    for (std::set<size_t>::const_iterator it = kReached.begin(); it != kReached.end(); ++it)
    {
        kNames.insert(kFuncs[*it].sName);
    }
    return kNames;
}

}
