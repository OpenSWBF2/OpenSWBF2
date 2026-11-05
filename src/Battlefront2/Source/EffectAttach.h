#pragma once

template <class EFFECT>
inline void DetachEffectFromTarget(EFFECT& rEffect)
{
    if (rEffect.m_hobjAttachedToObject.mObject || !(rEffect.m_uiAttachFlags & EFFECT::ATTACHED_DYNAMIC))
    {
        return;
    }
    rEffect._UpdateWorldMatrix();
    rEffect.m_uiAttachFlags &= ~(EFFECT::ATTACHED_DYNAMIC | EFFECT::ATTACHED_TO_VECTOR);
    rEffect.m_pmAttachedToMatrix = 0;
}
