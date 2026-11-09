#pragma once

class PblHandled;

class PblHandleRegistry
{
public:
    static bool IsLive(const PblHandled* pKey, unsigned int uId);
    static bool Lookup(const PblHandled* pKey, unsigned int* puId);
    static unsigned int CountInRange(const void* pStart, const void* pEnd);
    static void PurgeIf(bool (*pfnMatch)(const void* pKey, void* pArg), void* pArg);
    static unsigned int CountIf(bool (*pfnMatch)(const void* pKey, void* pArg), void* pArg);
    static unsigned int GetCount();
    static unsigned int GetCapacity();
    static unsigned int HomeSlot(const void* pKey);
#ifdef OPENSWBF2_TEST
    static void FailAllocForTest(bool bFail);
#endif

private:
    friend class PblHandled;

    static void Insert(const PblHandled* pKey, unsigned int uId);
    static void Erase(const PblHandled* pKey);
};

// VTABLE: SWBF2 0x00785b04
class PblHandled
{
public:

    PblHandled();
    PblHandled(const PblHandled& other);
    PblHandled& operator=(const PblHandled& other);

    // FUNCTION: SWBF2 0x0042cf20
    virtual ~PblHandled() { mHandleId = 0; PblHandleRegistry::Erase(this); }

    unsigned int GetHandleId() const;

    static unsigned int SetNextHandleId(unsigned int uId);
    static unsigned int GetNextHandleId();
    static unsigned int HandleIdOffset();

private:
    template <class> friend class PblHandle;

    unsigned int mHandleId;
};

template <class TYPE>
class PblHandle
{
public:


    TYPE& operator()() const { return *GetObjectA(); }

    TYPE* mObject;

private:
    unsigned int mSavedHandleId;

public:

    // TEMPLATE: SWBF2 0x00439a80
    // PblHandle<GameObject>::PblHandle<GameObject>
    PblHandle(TYPE* pObject) : mObject(pObject), mSavedHandleId(pObject ? pObject->mHandleId : 0) {}
    // TEMPLATE: SWBF2 0x0042e950
    // PblHandle<GameObject const >::PblHandle<GameObject const >
    PblHandle() : mObject(0), mSavedHandleId(0) {}

    // TEMPLATE: SWBF2 0x0042cd10
    // PblHandle<GameObject>::PblHandle<GameObject>

    // TEMPLATE: SWBF2 0x004479c0
    // PblHandle<FLEffectObject>::PblHandle<FLEffectObject>

    // TEMPLATE: SWBF2 0x004b97c0
    // PblHandle<ParticleEmitterObject>::PblHandle<ParticleEmitterObject>

    // TEMPLATE: SWBF2 0x004479d0
    // PblHandle<FLEffectObject>::~PblHandle<FLEffectObject>

    // TEMPLATE: SWBF2 0x004b97d0
    // PblHandle<ParticleEmitterObject>::~PblHandle<ParticleEmitterObject>

    // TEMPLATE: SWBF2 0x0042cd20
    // PblHandle<GameObject>::~PblHandle<GameObject>
    ~PblHandle() {}

    // TEMPLATE: SWBF2 0x005d3700
    // PblHandle<GameObject>::GetHandleId
    unsigned int GetHandleId() const { return mSavedHandleId; }

    // TEMPLATE: SWBF2 0x0043cd00
    // PblHandle<GameObject const >::operator==

    // TEMPLATE: SWBF2 0x00470880
    // PblHandle<GameObject>::operator==

    // TEMPLATE: SWBF2 0x00489c20
    // PblHandle<FLEffectObject>::operator==
    bool operator==(TYPE* pObject) const
    {
        return pObject == GetObjectA();
    }

    // TEMPLATE: SWBF2 0x00424b00
    // PblHandle<GameObject>::operator->
    // TEMPLATE: SWBF2 0x0043a120
    // PblHandle<GameObject const >::operator->
    TYPE* operator->() const { return GetObjectA(); }

    // TEMPLATE: SWBF2 0x0043a150
    // PblHandle<GameObject const >::operator GameObject const *

    // TEMPLATE: SWBF2 0x00428e60
    // PblHandle<ParticleEmitterObject>::operator ParticleEmitterObject *
    // TEMPLATE: SWBF2 0x00424b30
    // PblHandle<GameObject>::operator GameObject *
    operator TYPE*() const { return GetObjectA(); }

    // TEMPLATE: SWBF2 0x00447a10
    // PblHandle<FLEffectObject>::operator!=

    // TEMPLATE: SWBF2 0x004850c0
    // PblHandle<GameObject>::operator!=

    // TEMPLATE: SWBF2 0x0043a0f0
    // PblHandle<GameObject const >::operator!=
    bool operator!=(TYPE* pObject) const { return pObject != GetObjectA(); }

    void operator=(const PblHandle& other)
    {
        mObject = other.mObject;
        mSavedHandleId = other.mSavedHandleId;
    }

    // TEMPLATE: SWBF2 0x0043ccd0
    // PblHandle<GameObject const >::operator=

    // TEMPLATE: SWBF2 0x0042cd30
    // PblHandle<GameObject>::operator=
    void operator=(TYPE* pObject)
    {
        mObject = pObject;
        mSavedHandleId = pObject ? pObject->mHandleId : 0;
    }

    bool IsLive() const { return mObject && PblHandleRegistry::IsLive(mObject, mSavedHandleId); }

    // TEMPLATE: SWBF2 0x00428e40
    // PblHandle<ParticleEmitterObject>::GetObjectA
    TYPE* GetObjectA() const
    {
        if (mObject)
        {
            if (PblHandleRegistry::IsLive(mObject, mSavedHandleId))
            {
                return mObject;
            }
            PblHandle* pThis = const_cast<PblHandle*>(this);
            pThis->mObject = 0;
            pThis->mSavedHandleId = 0;
        }
        return 0;
    }
};
