#pragma once

enum PblJournalMode
{
    PJM_DISABLED = 0,
    PJM_WRITING = 1,
    PJM_READING = 2
};

enum PblJournalType
{
    PJT_FULL = 0,
    PJT_MOVES = 1
};

class PblJournal
{
public:

    static PblJournal* Get(PblJournalType type);

    template <class T>
    static void Record(T& value, PblJournalType type)
    {
        (void)value;
        (void)type;
    }

    // FUNCTION: SWBF2 0x005b4230
    void RecordLow(void*, int) {}

    PblJournalMode mMode;
};

void JournalString(char* pcString, int iMax);
void __cdecl JournalBytes(char* bytes, int len);
