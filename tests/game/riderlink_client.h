#pragma once
#include <functional>
#include <stdint.h>
#include <string>
#include <vector>

class RiderLinkClient
{
public:
    enum Wake
    {
        WakeData,
        WakeClosed,
        WakeEnded
    };

    RiderLinkClient();
    ~RiderLinkClient();

    const std::string& Folder() const;
    std::string Args() const;

    bool WaitForPortFile(const std::string& sRunDir);
    bool PortFileExists() const;
    const std::string& PortFileText() const;
    unsigned long Pid() const;
    unsigned int Port() const;
    const std::string& Token() const;

    static std::vector<unsigned long> LocalAddresses();
    int ConnectTo(unsigned long uAddr);
    bool Connect();
    void Disconnect();
    bool SendLine(const std::string& sLine);
    bool SendBytes(const std::string& sBytes);
    Wake ReadLineUntil(const std::function<bool(const std::string&)>& fnWant, std::string& sLine);
    Wake ReadUntilClosed(std::string& sAll);

private:
    RiderLinkClient(const RiderLinkClient&);
    RiderLinkClient& operator=(const RiderLinkClient&);

    void FindChild();
    bool ReadPortFile();
    Wake Pump();

    std::string mFolder;
    std::string mPortText;
    std::string mToken;
    std::string mBuffer;
    unsigned long mPid;
    unsigned int mPort;
    void* mProcess;
    void* mEvent;
    uintptr_t mSocket;
    bool mConnected;
    bool mWsa;
};
