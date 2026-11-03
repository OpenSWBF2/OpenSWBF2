#include "sndobjectlist.h"

namespace Snd
{

// SYNTHETIC: SWBF2 0x0040bae0
// `dynamic initializer for 'Snd::ObjectList::Node::smFreeList''
// SYNTHETIC: SWBF2 0x0076a360
// `dynamic atexit destructor for 'Snd::ObjectList::Node::smFreeList''

// GLOBAL: SWBF2 0x007e3634
// ObjectList::Node::smFreeList
FreeList ObjectList::Node::smFreeList(8, 1, true, 16);

// FUNCTION: SWBF2 0x007388d0
void ObjectList::Remove(void* pObject)
{
    delete FindByObject(pObject);
}

// FUNCTION: SWBF2 0x007388a0
ObjectList::Node* ObjectList::FindByObject(void* pObject)
{
    Itor itor(*this);
    void* pStored;
    while ((pStored = itor.GetObjectA(this)) != 0)
    {
        if (pStored == pObject)
        {
            return itor.GetNode();
        }
        ++itor;
    }
    return 0;
}

}
