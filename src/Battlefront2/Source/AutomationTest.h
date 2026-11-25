#pragma once

#ifdef OPENSWBF2_TEST

namespace EAutomationTestFlags
{
enum Type
{
    SmokeFilter = 0x01000000,
    EngineFilter = 0x02000000,
    ProductFilter = 0x04000000,
    PerfFilter = 0x08000000,
    StressFilter = 0x10000000,
    NegativeFilter = 0x20000000
};
}

class FAutomationTestBase
{
public:
    FAutomationTestBase(char const* pcName, int iFlags, char const* pcFile, int iLine);
    virtual ~FAutomationTestBase() {}

    virtual bool RunTest(char const* pcParameters) = 0;

    void AddError(char const* pcFormat, ...);
    bool TestTrue(char const* pcWhat, bool bValue);
    bool TestFalse(char const* pcWhat, bool bValue);
    bool TestEqual(char const* pcWhat, int iActual, int iExpected);
    bool TestEqual(char const* pcWhat, char const* pcActual, char const* pcExpected);

    char const* GetTestFullName() const { return mpcName; }
    int GetTestFlags() const { return miFlags; }
    char const* GetTestSourceFileName() const { return mpcFile; }
    int GetTestSourceFileLine() const { return miLine; }
    bool HasAnyErrors() const { return miErrors != 0; }

    static FAutomationTestBase* sFirst;
    FAutomationTestBase* mpNext;
    int miErrors;

private:
    char const* mpcName;
    int miFlags;
    char const* mpcFile;
    int miLine;
};

namespace AutomationTest
{
int RunRequested(int iExitCode);
bool Select(char const* pcFilter, char const* pcName, char* pcParameters, int iParametersSize);
}

#define IMPLEMENT_SIMPLE_AUTOMATION_TEST(TClass, PrettyName, TFlags)                                   \
    class TClass : public FAutomationTestBase                                                          \
    {                                                                                                  \
    public:                                                                                            \
        TClass() : FAutomationTestBase(PrettyName, TFlags, __FILE__, __LINE__) {}                      \
        virtual bool RunTest(char const* pcParameters);                                                \
    };                                                                                                 \
    namespace                                                                                          \
    {                                                                                                  \
    TClass TClass##AutomationTestInstance;                                                             \
    }

#endif
