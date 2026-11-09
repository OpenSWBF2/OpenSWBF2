#include "third_party/doctest/doctest.h"

#include <string>

#include "testutil.h"
#include "Battlefront2/Source/ComboArray.h"
#include "RedEngineFL/Memory/RedMemory.h"
#include "RedEngineFL/Memory/RedMemoryPool.h"

namespace
{

struct ComboProbe
{
    ComboProbe() : uValue(0xC0FFEEu), uTag(0) {}
    unsigned int uValue;
    unsigned int uTag;
};

ComboProbe* AddProbe(MemoryPool& rPool, ComboProbe*& raArray, unsigned int& ruCount, unsigned int uTag)
{
    ComboProbe* pProbe = ComboArrayAdd(rPool, raArray, ruCount);
    if (pProbe)
    {
        CHECK(pProbe == raArray + ruCount);
        CHECK(pProbe->uValue == 0xC0FFEEu);
        pProbe->uValue = ruCount;
        pProbe->uTag = uTag;
        ++ruCount;
    }
    return pProbe;
}

std::string ReadComboText(const char* pcRelPath)
{
    std::string sText;
    REQUIRE(ReadRepoText(pcRelPath, sText));
    std::string sOut;
    for (size_t i = 0; i < sText.size(); ++i)
    {
        if (sText[i] != '')
        {
            sOut += sText[i];
        }
    }
    return sOut;
}

int CountComboText(const std::string& sText, const char* pcNeedle)
{
    int iCount = 0;
    const std::string sNeedle(pcNeedle);
    for (size_t uiAt = sText.find(sNeedle); uiAt != std::string::npos; uiAt = sText.find(sNeedle, uiAt + 1))
    {
        ++iCount;
    }
    return iCount;
}

bool Holds(const ComboProbe* pArray, unsigned int uCount, unsigned int uTag)
{
    for (unsigned int i = 0; i < uCount; ++i)
    {
        if (pArray[i].uValue != i || pArray[i].uTag != uTag)
        {
            return false;
        }
    }
    return true;
}

}

TEST_CASE("combo array keeps the retail block order in a fresh pool" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1comboorder", sizeof(ComboProbe));
    kPool.Create(8, sizeof(ComboProbe), 0);
    ComboProbe* pA = 0;
    ComboProbe* pB = 0;
    unsigned int uA = 0;
    unsigned int uB = 0;
    for (int i = 0; i < 3; ++i)
    {
        REQUIRE(AddProbe(kPool, pA, uA, 1));
    }
    for (int i = 0; i < 3; ++i)
    {
        REQUIRE(AddProbe(kPool, pB, uB, 2));
    }
    CHECK((char*)pA == (char*)kPool.mPool);
    CHECK((char*)pB == (char*)kPool.mPool + 3 * sizeof(ComboProbe));
    CHECK(Holds(pA, 3, 1));
    CHECK(Holds(pB, 3, 2));
    CHECK(kPool.mUsed == 6);
    CHECK((char*)kPool.Allocate(sizeof(ComboProbe)) == (char*)kPool.mPool + 6 * sizeof(ComboProbe));
    kPool.Destroy();
}

TEST_CASE("combo arrays stay contiguous when another array takes the next pool block" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1combomix", sizeof(ComboProbe));
    kPool.Create(24, sizeof(ComboProbe), 0);
    ComboProbe* pA = 0;
    ComboProbe* pB = 0;
    unsigned int uA = 0;
    unsigned int uB = 0;
    for (int i = 0; i < 5; ++i)
    {
        REQUIRE(AddProbe(kPool, pA, uA, 1));
        REQUIRE(AddProbe(kPool, pB, uB, 2));
    }
    CHECK(Holds(pA, 5, 1));
    CHECK(Holds(pB, 5, 2));
    CHECK(kPool.mUsed == 10);
    CHECK(kPool.GetPeak() <= 24);
    kPool.Destroy();
}

TEST_CASE("combo array refuses a block it cannot place contiguously" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1combofull", sizeof(ComboProbe));
    kPool.Create(3, sizeof(ComboProbe), 0);
    ComboProbe* pA = 0;
    ComboProbe* pB = 0;
    unsigned int uA = 0;
    unsigned int uB = 0;
    REQUIRE(AddProbe(kPool, pA, uA, 1));
    REQUIRE(AddProbe(kPool, pB, uB, 2));
    ComboProbe* pOld = pA;
    CHECK(ComboArrayAdd(kPool, pA, uA) == (ComboProbe*)0);
    CHECK(pA == pOld);
    CHECK(Holds(pA, 1, 1));
    CHECK(Holds(pB, 1, 2));
    CHECK(kPool.mUsed == 2);
    kPool.Destroy();
}

#ifdef OPENSWBF2_TEST
TEST_CASE("combo array ignores a shuffled pool hand-out order" * doctest::test_suite("regression"))
{
    RedPerturbSet(1, kRedPerturbPool);
    MemoryPool kPool("t1comboshuffle", sizeof(ComboProbe));
    kPool.Create(16, sizeof(ComboProbe), 0);
    RedPerturbSet(0, 0);
    ComboProbe* pA = 0;
    unsigned int uA = 0;
    for (int i = 0; i < 6; ++i)
    {
        REQUIRE(AddProbe(kPool, pA, uA, 1));
    }
    CHECK((char*)pA == (char*)kPool.mPool);
    CHECK(Holds(pA, 6, 1));
    kPool.Destroy();
}
#endif

TEST_CASE("combo array relocation picks the lowest run after a reverse free" * doctest::test_suite("regression"))
{
    MemoryPool kPool("t1comboreverse", sizeof(ComboProbe));
    kPool.Create(12, sizeof(ComboProbe), 0);
    for (int i = 0; i < 12; ++i)
    {
        REQUIRE(kPool.Allocate(sizeof(ComboProbe)));
    }
    ComboProbe* pBlocks = (ComboProbe*)kPool.mPool;
    const int aiFree[] = { 0, 1, 2, 6, 7, 8, 9, 10, 11 };
    for (int i = 0; i < 9; ++i)
    {
        kPool.Free(pBlocks + aiFree[i]);
    }
    ComboProbe* pA = pBlocks + 3;
    for (unsigned int i = 0; i < 2; ++i)
    {
        pA[i].uValue = i;
        pA[i].uTag = 1;
    }
    unsigned int uA = 2;
    REQUIRE(AddProbe(kPool, pA, uA, 1));
    CHECK(pA == pBlocks);
    CHECK(uA == 3);
    CHECK(Holds(pA, 3, 1));
    CHECK(kPool.mUsed == 4);
    kPool.Destroy();
}

TEST_CASE("combo arrays route every pool array through ComboArrayAdd (source pin)" * doctest::test_suite("regression"))
{
    std::string sCombo = ReadComboText("src\\Battlefront2\\Source\\Combo.cpp");
    CHECK(CountComboText(sCombo, "ComboArrayAdd(") == 6);
    CHECK(CountComboText(sCombo, "ComboArrayAdd(State::sMemoryPool, m_aState, uiCount)") == 1);
    CHECK(CountComboText(sCombo, "ComboArrayAdd(Transition::sMemoryPool, m_aTransition, m_uiNumTransitions)") == 1);
    CHECK(CountComboText(sCombo, "ComboArrayAdd(Attack::sMemoryPool, m_aAttack, m_uiNumAttacks)") == 1);
    CHECK(CountComboText(sCombo, "ComboArrayAdd(Condition::sMemoryPool, m_aCondition, m_uiNumConditions)") == 1);
    CHECK(CountComboText(sCombo, "ComboArrayAdd(Condition::sMemoryPool, m_aAnimatedMoveCondition, m_uiNumAnimatedMoveConditions)") == 1);
    CHECK(CountComboText(sCombo, "ComboArrayAdd(DamageSample::sMemoryPool, m_aDamageSample, m_uiNumDamageSamples)") == 1);
    CHECK(CountComboText(sCombo, "_AllocState()") == 1);
    CHECK(CountComboText(sCombo, "_AllocTransition()") == 1);
    CHECK(CountComboText(sCombo, "_AllocAttack()") == 1);
    CHECK(CountComboText(sCombo, "_AllocCondition()") == 1);
    CHECK(CountComboText(sCombo, "            m_pCatchState = m_aState + (m_pCatchState - pOld);\n") == 1);
    CHECK(CountComboText(sCombo, "            m_pTakeKneeState = m_aState + (m_pTakeKneeState - pOld);\n") == 1);
}
