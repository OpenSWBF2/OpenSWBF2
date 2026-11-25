#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include <string>

#include "testutil.h"
#include "Battlefront2/Source/Combo.h"
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

struct ComboShort
{
    ComboShort() : uValue(0xBEEF), uTag(0), uPad(0x5A5A) {}
    unsigned short uValue;
    unsigned short uTag;
    unsigned short uPad;
};

ComboShort* AddShort(MemoryPool& rPool, ComboShort*& raArray, unsigned int& ruCount, unsigned short uTag)
{
    ComboShort* pShort = ComboArrayAdd(rPool, raArray, ruCount);
    if (pShort)
    {
        CHECK(pShort == raArray + ruCount);
        CHECK(pShort->uValue == 0xBEEF);
        pShort->uValue = (unsigned short)ruCount;
        pShort->uTag = uTag;
        ++ruCount;
    }
    return pShort;
}

bool HoldsShort(const ComboShort* pArray, unsigned int uCount, unsigned short uTag)
{
    for (unsigned int i = 0; i < uCount; ++i)
    {
        if (pArray[i].uValue != i || pArray[i].uTag != uTag || pArray[i].uPad != 0x5A5A)
        {
            return false;
        }
    }
    return true;
}

}

TEST_CASE_NAMED(FComboArrayTest1, "Unit::ComboArray::combo array packs elements across pool blocks", "[EngineFilter]")
{
    MemoryPool kPool("t1combopack", sizeof(ComboShort));
    kPool.Create(16, sizeof(ComboShort), 0);
    REQUIRE(sizeof(ComboShort) == 6);
    REQUIRE(kPool.GetSize() == 8);
    ComboShort* pA = 0;
    unsigned int uA = 0;
    for (int i = 0; i < 9; ++i)
    {
        REQUIRE(AddShort(kPool, pA, uA, 7));
        CHECK(HoldsShort(pA, uA, 7));
    }
    CHECK((char*)pA == (char*)kPool.mPool);
    CHECK(kPool.mUsed == 7);
    CHECK((char*)kPool.Allocate(8) == (char*)kPool.mPool + 7 * 8);
    kPool.Destroy();
}

template <class T>
void CheckComboPoolBlock(const char* pcLabel)
{
    MemoryPool kPool(pcLabel, sizeof(T));
    const unsigned int uiAlign = (unsigned int)sizeof(void*);
    const unsigned int uiElement = (unsigned int)sizeof(T);
    CAPTURE(pcLabel);
    CHECK(kPool.GetSize() == (uiElement + uiAlign - 1) / uiAlign * uiAlign);
    if (sizeof(void*) == 4)
    {
        CHECK(uiElement % 4 == 0);
        CHECK(kPool.GetSize() == uiElement);
    }
}

TEST_CASE_NAMED(FComboArrayTest2, "Unit::ComboArray::combo pools give each combo type a block equal to its size on x86", "[EngineFilter]")
{
    CheckComboPoolBlock<Combo::State>("Combo::State");
    CheckComboPoolBlock<Combo::Transition>("Combo::Transition");
    CheckComboPoolBlock<Combo::Condition>("Combo::Condition");
    CheckComboPoolBlock<Combo::Attack>("Combo::Attack");
    CheckComboPoolBlock<Combo::DamageSample>("Combo::DamageSample");
    std::string sCombo = ReadComboText("src\\Battlefront2\\Source\\Combo.cpp");
    CHECK(CountComboText(sCombo, "MemoryPool Combo::State::sMemoryPool(\"Combo::State\", sizeof(Combo::State));") == 1);
    CHECK(CountComboText(sCombo, "MemoryPool Combo::Transition::sMemoryPool(\"Combo::Transition\", sizeof(Combo::Transition));") == 1);
    CHECK(CountComboText(sCombo, "MemoryPool Combo::Condition::sMemoryPool(\"Combo::Condition\", sizeof(Combo::Condition));") == 1);
    CHECK(CountComboText(sCombo, "MemoryPool Combo::Attack::sMemoryPool(\"Combo::Attack\", sizeof(Combo::Attack));") == 1);
    CHECK(CountComboText(sCombo, "MemoryPool Combo::DamageSample::sMemoryPool(\"Combo::DamageSample\", sizeof(Combo::DamageSample));") == 1);
}

TEST_CASE_NAMED(FComboArrayTest3, "Unit::ComboArray::combo array keeps packed elements across a run move", "[EngineFilter]")
{
    MemoryPool kPool("t1combomove", sizeof(ComboShort));
    kPool.Create(16, sizeof(ComboShort), 0);
    ComboShort* pA = 0;
    ComboShort* pB = 0;
    unsigned int uA = 0;
    unsigned int uB = 0;
    for (int i = 0; i < 4; ++i)
    {
        REQUIRE(AddShort(kPool, pA, uA, 1));
    }
    CHECK(kPool.mUsed == 3);
    REQUIRE(AddShort(kPool, pB, uB, 2));
    CHECK((char*)pB == (char*)kPool.mPool + 3 * 8);
    REQUIRE(AddShort(kPool, pA, uA, 1));
    CHECK((char*)pA == (char*)kPool.mPool + 4 * 8);
    CHECK(HoldsShort(pA, 5, 1));
    CHECK(HoldsShort(pB, 1, 2));
    CHECK(kPool.mUsed == 5);
    CHECK((char*)kPool.AllocateRun(3, 8) == (char*)kPool.mPool);
    CHECK(kPool.mUsed == 8);
    kPool.Destroy();
}

TEST_CASE_NAMED(FComboArrayTest4, "Unit::ComboArray::combo array keeps the retail block order in a fresh pool", "[EngineFilter]")
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

TEST_CASE_NAMED(FComboArrayTest5, "Unit::ComboArray::combo arrays stay contiguous when another array takes the next pool block", "[EngineFilter]")
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

TEST_CASE_NAMED(FComboArrayTest6, "Unit::ComboArray::combo array refuses a block it cannot place contiguously", "[EngineFilter]")
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
TEST_CASE_NAMED(FComboArrayTest7, "Unit::ComboArray::combo array ignores a shuffled pool hand-out order", "[EngineFilter]")
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

TEST_CASE_NAMED(FComboArrayTest8, "Unit::ComboArray::combo array relocation picks the lowest run after a reverse free", "[EngineFilter]")
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

TEST_CASE_NAMED(FComboArrayTest9, "Unit::ComboArray::combo arrays route every pool array through ComboArrayAdd (source pin)", "[EngineFilter]")
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
