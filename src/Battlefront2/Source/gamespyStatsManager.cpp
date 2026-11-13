#include <string.h>
#include <unordered_set>
#include <vector>
#include <new>
#include "GameMemory.h"
#include "RedEngineFL/Memory/RedMemory.h"

#include <string.h>
#include "GameLoop.h"
#include "LuaCallbacks_IFace.h"
#include "StatsManager.h"
#include "PebbleFL/Common/PblUnicode.h"

namespace StatsManager
{

// FUNCTION: SWBF2 0x005307a0
void Update()
{
}

}

namespace StatsManager
{
extern int m_state;
}
extern unsigned char sStatsMatchData[0x40];
class NetPktGroup;

namespace StatsManager
{
void WriteStatsChallenge(int iPlayer, NetPktGroup& group);
void WriteStatsResponse(int iLocal, NetPktGroup& group);
void BeginMatch();
}

namespace StatsManager
{

// FUNCTION: SWBF2 0x00530890
void ReadStatsChallenge(int player, NetPktGroup& group)
{
}
}

namespace StatsManager
{

// FUNCTION: SWBF2 0x005308b0
void ReadStatsResponse(int player, NetPktGroup& group)
{
}
}

namespace StatsManager
{

// FUNCTION: SWBF2 0x005307b0
int GetState()
{
    return StatsManager::m_state;
}
}

// FUNCTION: SWBF2 0x005307c0
void StatsManagerSetStoppedState()
{
    StatsManager::m_state = 2;
}

namespace StatsManager
{

// FUNCTION: SWBF2 0x005308c0
bool ReadStatsGetResults(unsigned int iPlayer, LeaderBoardPlayerStatsT* pStats)
{
    memset(pStats, 0, sizeof(*pStats));
    pStats->factionStats[2].wins = 10;
    pStats->factionStats[2].losses = 10;
    return true;
}
}

namespace StatsManager
{

// FUNCTION: SWBF2 0x005308f0
bool FindServicePlayerID(const char*, int, unsigned __int64&)
{
    return false;
}
}

namespace StatsManager
{

// FUNCTION: SWBF2 0x00530900
int GetLeaderBoardId()
{
    return 1;
}
}

namespace StatsManager
{

// FUNCTION: SWBF2 0x00530910
bool ResetPlayer(unsigned __int64&)
{
    return false;
}
}

// FUNCTION: SWBF2 0x00530880
void StatsManager::WriteStatsChallenge(int, NetPktGroup&) {}

// FUNCTION: SWBF2 0x005308a0
void StatsManager::WriteStatsResponse(int, NetPktGroup&) {}

// FUNCTION: SWBF2 0x00530860
void StatsManager::BeginMatch()
{
    strlcpy((char*)sStatsMatchData, GameLoop::mMissionScript, sizeof(sStatsMatchData));
}

// SYNTHETIC: SWBF2 0x00403c20
// `dynamic initializer for 'sStatsLeaderboard''
// SYNTHETIC: SWBF2 0x00768910
// `dynamic atexit destructor for 'sStatsLeaderboard''

// TEMPLATE: SWBF2 0x00530920
// std::_Hash<std::_Uset_traits<unsigned __int64,std::_Uhash_compare<unsigned __int64,std::hash<unsigned __int64>,std::equal_to<unsigned __int64> >,std::allocator<unsigned __int64>,0> >::_Hash<std::_Uset_traits<unsigned __int64,std::_Uhash_compare<unsigned __int64,std::hash<unsigned __int64>,std::equal_to<unsigned __int64> >,std::allocator<unsigned __int64>,0> >

// TEMPLATE: SWBF2 0x005309e0
// std::list<unsigned __int64,std::allocator<unsigned __int64> >::~list<unsigned __int64,std::allocator<unsigned __int64> >

// TEMPLATE: SWBF2 0x00530a00
// std::list<unsigned __int64,std::allocator<unsigned __int64> >::clear

// TEMPLATE: SWBF2 0x00530a90
// std::vector<std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0>,std::_Wrap_alloc<std::allocator<std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0> > > >::_Destroy

// TEMPLATE: SWBF2 0x00530aa0
// std::_Wrap_alloc<std::allocator<std::_List_node<unsigned __int64,void *> > >::_Wrap_alloc<std::allocator<std::_List_node<unsigned __int64,void *> > >

// TEMPLATE: SWBF2 0x00530ab0
// std::vector<std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0>,std::_Wrap_alloc<std::allocator<std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0> > > >::_Insert_n

// TEMPLATE: SWBF2 0x00530c70
// std::_Wrap_alloc<std::_Wrap_alloc<std::allocator<std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0> > > >::_Wrap_alloc<std::_Wrap_alloc<std::allocator<std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0> > > >

// TEMPLATE: SWBF2 0x00530c80
// std::vector<std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0>,std::_Wrap_alloc<std::allocator<std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0> > > >::_Grow_to

// TEMPLATE: SWBF2 0x00530cc0
// std::_List_alloc<0,std::_List_base_types<unsigned __int64,std::allocator<unsigned __int64> > >::_Buynode0

// TEMPLATE: SWBF2 0x00530cf0
// std::_Wrap_alloc<std::allocator<std::_List_node<unsigned __int64,void *> > >::destroy<std::_List_node<unsigned __int64,void *> *>

// TEMPLATE: SWBF2 0x00530d00
// std::fill<std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0> *,std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0> >

// TEMPLATE: SWBF2 0x00530d40
// std::_Uninit_fill_n<std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0> *,unsigned int,std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0>,std::_Wrap_alloc<std::allocator<std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0> > >,std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0> >

// TEMPLATE: SWBF2 0x00530d70
// std::_Uninit_move<std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0> *,std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0> *,std::_Wrap_alloc<std::allocator<std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0> > >,std::_List_unchecked_const_iterator<std::_List_val<std::_List_simple_types<unsigned __int64> >,std::_Iterator_base0> >

// GLOBAL: SWBF2 0x01ec0c20
// sStatsLeaderboard
std::unordered_set<unsigned __int64> sStatsLeaderboard;

namespace StatsManager
{

// GLOBAL: SWBF2 0x007e6668
int m_state = 1;
}

// GLOBAL: SWBF2 0x01e56008
bool sStatsRequestBusy;

// GLOBAL: SWBF2 0x01e55fc0
unsigned char sStatsMatchData[0x40];

namespace StatsManager
{

// FUNCTION: SWBF2 0x00530640
void Init()
{
    StatsManager::m_state = 1;
    sStatsRequestBusy = false;

    sStatsLeaderboard.clear();
    memset(sStatsMatchData, 0, 0x40);
}
}

void StatsManagerSetStoppedState();

namespace StatsManager
{

// FUNCTION: SWBF2 0x005307d0
void Stop()
{
    if (StatsManager::m_state == 7 || StatsManager::m_state == 3)
    {
        StatsManagerSetStoppedState();
    }
    else if (StatsManager::m_state == 0)
    {
        StatsManager::m_state = 1;
    }
    sStatsRequestBusy = false;

    sStatsLeaderboard.clear();
    if (StatsManager::m_state == 1)
    {
        StatsManager::Init();
    }
}
}

// GLOBAL: SWBF2 0x01e55fb7
bool sStatsHeapAllocated;

// GLOBAL: SWBF2 0x01e55fbc
void* sStatsHeapBlockA;

// GLOBAL: SWBF2 0x01e56000
void* sStatsHeapBlockB;

// GLOBAL: SWBF2 0x01e56004
void* sStatsHeapBlockC;

// GLOBAL: SWBF2 0x01e5600c
void* sStatsHeapBlockD;

// GLOBAL: SWBF2 0x01e56010
void* sStatsHeapBlockE;

// FUNCTION: SWBF2 0x00530620
void __fastcall FreeGamespyBlock(void* pBlock)
{
    RedFreeToHeap(GameMemory::GamespyHeap, pBlock, 0);
}

namespace StatsManager
{

// FUNCTION: SWBF2 0x005306b0
void Cleanup()
{
    StatsManager::m_state = 1;
    sStatsRequestBusy = false;

    sStatsLeaderboard.clear();

    if (sStatsHeapAllocated)
    {
        sStatsHeapAllocated = false;
        if (sStatsHeapBlockB)
        {
            FreeGamespyBlock(sStatsHeapBlockB);
            sStatsHeapBlockB = 0;
        }
        if (sStatsHeapBlockE)
        {
            FreeGamespyBlock(sStatsHeapBlockE);
            sStatsHeapBlockE = 0;
        }
        if (sStatsHeapBlockD)
        {
            FreeGamespyBlock(sStatsHeapBlockD);
            sStatsHeapBlockD = 0;
        }
        if (sStatsHeapBlockC)
        {
            FreeGamespyBlock(sStatsHeapBlockC);
            sStatsHeapBlockC = 0;
        }
        if (sStatsHeapBlockA)
        {
            FreeGamespyBlock(sStatsHeapBlockA);
            sStatsHeapBlockA = 0;
        }
    }
}
}
// SYNTHETIC: SWBF2 0x00403c50
// `dynamic initializer for 'sStatsScratchBytes''
// SYNTHETIC: SWBF2 0x007689a0
// `dynamic atexit destructor for 'sStatsScratchBytes''

// TEMPLATE: SWBF2 0x00530a70
// std::vector<unsigned char,std::allocator<unsigned char> >::_Destroy

// TEMPLATE: SWBF2 0x00530a80
// std::_Wrap_alloc<std::allocator<unsigned char> >::_Wrap_alloc<std::allocator<unsigned char> >

// GLOBAL: SWBF2 0x01ec0c14
// sStatsScratchBytes
std::vector<unsigned char> sStatsScratchBytes;
