#pragma once

template <class TYPE, class KEY>
class PblOrderedList
{
public:
    class Node
    {
    public:
        Node() : _pList(0), _pNext(0), _pPrev(0), _pObject(0), _iOrder(0) {}

        PblOrderedList* _pList;
        Node* _pNext;
        Node* _pPrev;
        TYPE* _pObject;
        KEY _iOrder;
    };

    class Itor
    {
    public:
        Itor() : _pNode(0) {}
        Itor(const PblOrderedList* pList) : _pNode(pList->GetFirst()) {}
        Itor(const PblOrderedList& list) : _pNode(list.GetFirst()) {}
        Itor(Node* pNode) : _pNode(pNode) {}

        TYPE* operator->() const { return _pNode->_pObject; }
        TYPE& operator()() const { return *_pNode->_pObject; }
        TYPE& operator*() const { return *_pNode->_pObject; }
        operator TYPE*() const { return _pNode->_pObject; }
        const Itor& operator++() { _pNode = _pNode->_pNext; return *this; }
        const Itor& operator--() { _pNode = _pNode->_pPrev; return *this; }
        Node* GetNode() { return _pNode; }

    private:
        Node* _pNode;
    };

    PblOrderedList()
    {
        _head._pPrev = &_head;
        _head._pNext = &_head;
        _iCount = 0;
    }

    void Add(Node* pNode, TYPE* pObject, KEY iOrder);
    void Clear();

    int GetCount() const { return _iCount; }
    Node* GetFirst() const { return _head._pNext; }
    Node* GetLast() const { return _head._pPrev; }
    Itor Begin() const { return Itor(GetFirst()); }
    Itor End() const { return Itor(const_cast<Node*>(&_head)); }

    void Remove(Node* pNode)
    {
        --_iCount;
        pNode->_pList = 0;
        pNode->_pObject = 0;
        pNode->_pNext->_pPrev = pNode->_pPrev;
        pNode->_pPrev->_pNext = pNode->_pNext;
    }

    Node _head;
    int _iCount;
};

template <class TYPE, class KEY>
inline void PblOrderedList<TYPE, KEY>::Add(Node* pNode, TYPE* pObject, KEY iOrder)
{
    Node* pAt = _head._pNext;
    ++_iCount;
    _head._iOrder = 0x7FFFFFFF;

    while (iOrder > pAt->_iOrder)
    {
        pAt = pAt->_pNext;
    }
    while (iOrder == pAt->_iOrder && pNode > pAt)
    {
        pAt = pAt->_pNext;
    }

    pNode->_pList = this;
    pNode->_iOrder = iOrder;
    pNode->_pObject = pObject;
    pNode->_pNext = pAt;
    pNode->_pPrev = pAt->_pPrev;
    pAt->_pPrev = pNode;
    pNode->_pPrev->_pNext = pNode;
}

template <class TYPE, class KEY>
void PblOrderedList<TYPE, KEY>::Clear()
{
    while (_iCount > 0)
    {
        Remove(_head._pNext);
    }
}

template <class TYPE>
class PblList
{
public:
    class Node
    {
    public:

        Node() : _pList(0), _pNext(0), _pPrev(0), _pObject(0) {}

        bool IsInList() const { return _pList != 0; }

        PblList<TYPE>* GetList() { return _pList; }

        PblList<TYPE>* _pList;
        Node* _pNext;
        Node* _pPrev;
        TYPE* _pObject;
    };

    class Itor
    {
    public:

        Itor() : _pNode(0) {}

        Itor(const Itor& itor) : _pNode(itor._pNode) {}

        // TEMPLATE: SWBF2 0x00437db0
        // PblList<Controllable>::Itor::Itor

        Itor(const PblList<TYPE>& list) { _pNode = list._head._pNext; }
        // FUNCTION: SWBF2 0x004433d0
        Itor(const PblList<TYPE>* pList) { _pNode = pList->_head._pNext; }

        // TEMPLATE: SWBF2 0x0049e1c0
        // PblList<Factory<Weapon,WeaponClass,WeaponDesc> >::Itor::Itor
        // TEMPLATE: SWBF2 0x005d3b50
        // PblList<Factory<Ordnance,OrdnanceClass,OrdnanceDesc> >::Itor::Itor
        // TEMPLATE: SWBF2 0x00446d80
        // PblList<Factory<Entity,EntityClass,EntityDesc> >::Itor::Itor
        Itor(Node* pNode) : _pNode(pNode) {}

        const Itor& operator=(Node* pNode)
        {
            _pNode = pNode;
            return *this;
        }

        // TEMPLATE: SWBF2 0x0049e1f0
        // PblList<Factory<Weapon,WeaponClass,WeaponDesc> >::Itor::operator Factory<Weapon,WeaponClass,WeaponDesc> *
        // TEMPLATE: SWBF2 0x005d3bb0
        // PblList<Factory<Ordnance,OrdnanceClass,OrdnanceDesc> >::Itor::operator Factory<Ordnance,OrdnanceClass,OrdnanceDesc> *
        // TEMPLATE: SWBF2 0x00487f80
        // PblList<FlagItem>::Itor::operator FlagItem *
        // TEMPLATE: SWBF2 0x00437de0
        // PblList<Controllable>::Itor::operator Controllable *
        // TEMPLATE: SWBF2 0x00446db0
        // PblList<Factory<Entity,EntityClass,EntityDesc> >::Itor::operator Factory<Entity,EntityClass,EntityDesc> *
        operator TYPE*() const { return _pNode->_pObject; }

        // TEMPLATE: SWBF2 0x0049e1e0
        // PblList<Factory<Weapon,WeaponClass,WeaponDesc> >::Itor::operator*
        // TEMPLATE: SWBF2 0x005d3b90
        // PblList<Factory<Ordnance,OrdnanceClass,OrdnanceDesc> >::Itor::operator*
        // TEMPLATE: SWBF2 0x00563920
        // PblList<FlagItem>::Itor::operator*
        // TEMPLATE: SWBF2 0x005d3720
        // PblList<Controllable>::Itor::operator*
        // TEMPLATE: SWBF2 0x00446da0
        // PblList<Factory<Entity,EntityClass,EntityDesc> >::Itor::operator*
        TYPE& operator*() const { return *_pNode->_pObject; }
        // TEMPLATE: SWBF2 0x0049e1d0
        // PblList<Factory<Weapon,WeaponClass,WeaponDesc> >::Itor::operator->
        // TEMPLATE: SWBF2 0x005d3b70
        // PblList<Factory<Ordnance,OrdnanceClass,OrdnanceDesc> >::Itor::operator->
        // TEMPLATE: SWBF2 0x00437dd0
        // PblList<Controllable>::Itor::operator->
        // TEMPLATE: SWBF2 0x00446d90
        // PblList<Factory<Entity,EntityClass,EntityDesc> >::Itor::operator->
        TYPE* operator->() const { return _pNode->_pObject; }

        // TEMPLATE: SWBF2 0x005d3bd0
        // PblList<Factory<Ordnance,OrdnanceClass,OrdnanceDesc> >::Itor::operator++
        // TEMPLATE: SWBF2 0x00487f90
        // PblList<FlagItem>::Itor::operator++
        // TEMPLATE: SWBF2 0x00437df0
        // PblList<Controllable>::Itor::operator++
        // TEMPLATE: SWBF2 0x00446dc0
        // PblList<Factory<Entity,EntityClass,EntityDesc> >::Itor::operator++
        Itor& operator++() { _pNode = _pNode->_pNext; return *this; }

        Itor& operator--() { _pNode = _pNode->_pPrev; return *this; }
        Node* GetNode() { return _pNode; }

        Node* _pNode;
    };

    PblList()
    {
        _head._pList = this;
        Node* pNode = &_head;
        _head._pPrev = pNode;
        _head._pNext = pNode;
        _head._pObject = 0;
        _iCount = 0;
    }

    int GetCount() const { return _iCount; }

    // TEMPLATE: SWBF2 0x0053bff0
    // PblList<Factory<Ordnance,OrdnanceClass,OrdnanceDesc> >::GetFirst
    // TEMPLATE: SWBF2 0x0049e1b0
    // PblList<Factory<Weapon,WeaponClass,WeaponDesc> >::GetFirst
    // TEMPLATE: SWBF2 0x00487fa0
    // PblList<FlagItem>::GetFirst
    // TEMPLATE: SWBF2 0x00446d70
    // PblList<Factory<Entity,EntityClass,EntityDesc> >::GetFirst
    Node* GetFirst() const { return _head._pNext; }

    Itor Begin() const { return Itor(GetFirst()); }

    Node* GetLast() const { return _head._pPrev; }

    Itor End() const { return Itor(const_cast<Node*>(&_head)); }

    void AddBefore(Node* pAt, Node* pNode, TYPE* pObject)
    {
        ++_iCount;
        pNode->_pObject = pObject;
        pNode->_pList = this;
        pNode->_pNext = pAt;
        pNode->_pPrev = pAt->_pPrev;
        pNode->_pNext->_pPrev = pNode;
        pNode->_pPrev->_pNext = pNode;
    }

    void AddLast(Node* pNode, TYPE* pObject) { AddBefore(&_head, pNode, pObject); }

    void Add(Node* pNode, TYPE* pObject) { AddBefore(&_head, pNode, pObject); }

    void AddFirst(Node* pNode, TYPE* pObject)
    {
        AddBefore(_head._pNext, pNode, pObject);
    }

    void AddAfter(Node* pAt, Node* pNode, TYPE* pObject)
    {
        AddBefore(pAt->_pNext, pNode, pObject);
    }

    // TEMPLATE: SWBF2 0x00460930
    // PblList<CollisionObject>::Remove
    void Remove(Node* pNode)
    {
        --_iCount;
        pNode->_pList = 0;
        pNode->_pObject = 0;
        pNode->_pNext->_pPrev = pNode->_pPrev;
        pNode->_pPrev->_pNext = pNode->_pNext;
    }

    // TEMPLATE: SWBF2 0x0043a180
    // PblList<Character>::Clear
    // TEMPLATE: SWBF2 0x0056fa50
    // PblList<LensFlare>::Clear
    // TEMPLATE: SWBF2 0x00695020
    // PblList<PrecipitationEffect::RainShadowRegion>::Clear
    // TEMPLATE: SWBF2 0x006edbe0
    // PblList<RedShadowProxy>::Clear
    // TEMPLATE: SWBF2 0x00526ab0
    // PblList<FlashyTextElement>::Clear
    void Clear()
    {
        while (GetCount() > 0)
        {
            Remove(GetFirst());
        }
    }

    Node _head;
    int _iCount;
};

// TEMPLATE: SWBF2 0x00542120
// PblList<GrassPatch>::Clear
// TEMPLATE: SWBF2 0x0056f380
// PblList<LeafPatch>::Clear
