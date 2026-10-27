#include "third_party/doctest/doctest.h"

#include "testutil.h"
#include "RedEngineFL/Application/pc/pcMessageDrain.h"

using namespace MessageDrain;

namespace
{

struct FakeQueue
{
    unsigned int uiLeft;
    unsigned int uiQuitAt;
    unsigned int uiPeeked;
    unsigned int uiDispatched;
    bool bQuitDispatched;
    __int64 iNow;
    __int64 iStep;
};

bool FakePeek(void* p, MSG* pMsg)
{
    FakeQueue* q = (FakeQueue*)p;
    if (q->uiLeft == 0)
    {
        return false;
    }
    q->uiLeft--;
    q->uiPeeked++;
    pMsg->message = (q->uiQuitAt != 0 && q->uiPeeked == q->uiQuitAt) ? WM_QUIT : WM_USER;
    return true;
}

void FakeDispatch(void* p, const MSG* pMsg)
{
    FakeQueue* q = (FakeQueue*)p;
    q->uiDispatched++;
    if (pMsg->message == WM_QUIT)
    {
        q->bQuitDispatched = true;
    }
}

__int64 FakeNow(void* p)
{
    FakeQueue* q = (FakeQueue*)p;
    __int64 r = q->iNow;
    q->iNow += q->iStep;
    return r;
}

FakeQueue Queue(unsigned int uiCount, __int64 iStep)
{
    FakeQueue q;
    q.uiLeft = uiCount;
    q.uiQuitAt = 0;
    q.uiPeeked = 0;
    q.uiDispatched = 0;
    q.bQuitDispatched = false;
    q.iNow = 0;
    q.iStep = iStep;
    return q;
}

DrainOps Ops(FakeQueue& q)
{
    DrainOps o;
    o.Peek = FakePeek;
    o.Dispatch = FakeDispatch;
    o.Now = FakeNow;
    o.pCtx = &q;
    return o;
}

}

TEST_CASE("message drain stops at the message bound" * doctest::test_suite("unit"))
{
    FakeQueue q = Queue(1000, 0);
    DrainResult r = DrainMessages(Ops(q), 64, 1000000);
    CHECK(r.uiCount == 64);
    CHECK(r.bHitCount);
    CHECK(!r.bHitTime);
    CHECK(!r.bQuit);
    CHECK(q.uiDispatched == 64);
}

TEST_CASE("message drain stops at the time bound" * doctest::test_suite("unit"))
{
    FakeQueue q = Queue(1000, 1);
    DrainResult r = DrainMessages(Ops(q), 1000, 10);
    CHECK(r.bHitTime);
    CHECK(!r.bHitCount);
    CHECK(r.uiCount < 1000);
}

TEST_CASE("message drain reports WM_QUIT and stops" * doctest::test_suite("unit"))
{
    FakeQueue q = Queue(10, 0);
    q.uiQuitAt = 3;
    DrainResult r = DrainMessages(Ops(q), 64, 1000000);
    CHECK(r.uiCount == 3);
    CHECK(r.bQuit);
    CHECK(q.bQuitDispatched);
    CHECK(q.uiLeft == 7);
}

TEST_CASE("message drain empty queue returns zero" * doctest::test_suite("unit"))
{
    FakeQueue q = Queue(0, 0);
    DrainResult r = DrainMessages(Ops(q), 64, 1000000);
    CHECK(r.uiCount == 0);
    CHECK(!r.bQuit);
    CHECK(!r.bHitCount);
    CHECK(!r.bHitTime);
    CHECK(q.uiDispatched == 0);
}
