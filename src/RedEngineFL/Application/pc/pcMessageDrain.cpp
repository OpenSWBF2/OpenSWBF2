#include "RedEngineFL/Application/pc/pcMessageDrain.h"

namespace MessageDrain
{

DrainResult DrainMessages(const DrainOps& ops, unsigned int uiMaxMsgs, __int64 iMaxTicks)
{
    DrainResult r;
    r.uiCount = 0;
    r.bQuit = false;
    r.bHitCount = false;
    r.bHitTime = false;
    const __int64 iStart = ops.Now(ops.pCtx);
    for (;;)
    {
        if (r.uiCount >= uiMaxMsgs)
        {
            r.bHitCount = true;
            break;
        }
        MSG msg;
        if (!ops.Peek(ops.pCtx, &msg))
        {
            break;
        }
        ops.Dispatch(ops.pCtx, &msg);
        r.uiCount++;
        if (msg.message == WM_QUIT)
        {
            r.bQuit = true;
            break;
        }
        const __int64 iNow = ops.Now(ops.pCtx);
        if (iNow - iStart >= iMaxTicks)
        {
            r.bHitTime = true;
            break;
        }
    }
    return r;
}

}
