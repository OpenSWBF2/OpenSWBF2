#pragma once

class GameObject;

inline int MergeSendScopes(GameObject** ppSends, GameObject* (*ppScopes)[64])
{
    int count = 0;
    for (int i = 0; i < 64; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            GameObject* pObject = ppScopes[j][i];
            if (pObject && count < 64)
            {
                int k = 0;
                while (k < count && ppSends[k] != pObject)
                {
                    ++k;
                }
                if (k == count)
                {
                    ppSends[count++] = pObject;
                }
            }
        }
    }
    return count;
}
