#pragma once

class VideoEffect
{
public:
    static void Init();
    static void Deinit();
    static void Activate(bool);
    static bool IsActive();
};
