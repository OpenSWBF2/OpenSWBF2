#pragma once

template <class T, int N>
class RedPipe
{
public:

    RedPipe() : mReadPos(0) {}

    T Peek(unsigned int uiIndex) const
    {
        unsigned int uiPos = mReadPos + uiIndex;
        if (uiPos >= N)
        {
            uiPos -= N;
        }
        return mElements[uiPos];
    }

    T Flow(const T& in)
    {
        T out = Peek(0);
        mElements[mReadPos] = in;
        if (++mReadPos == N)
        {
            mReadPos = 0;
        }
        return out;
    }

    bool Exists(const T& element) const
    {
        for (unsigned int i = 0; i < N; ++i)
        {
            if (mElements[i] == element)
            {
                return true;
            }
        }
        return false;
    }

    void Clear()
    {
        for (unsigned int i = 0; i < N; ++i)
        {
            mElements[i] = 0;
        }
    }

    T mElements[N];
    unsigned short mReadPos;
};

template <class T, int N>
class RedPipePointer : public RedPipe<T, N>
{
public:
    T GetRecent() const
    {
        T recent = 0;
        for (unsigned int i = N; i-- > 0 && !recent;)
        {
            recent = RedPipe<T, N>::Peek(i);
        }
        return recent;
    }

    RedPipe<T, N> mPipe;
};
