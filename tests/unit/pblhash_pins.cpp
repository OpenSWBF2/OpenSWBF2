#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "PebbleFL/Common/PblHashTable.h"
#include "RedEngineFL/Graphics/PC/pcDisplaySettings.h"

TEST_CASE_NAMED(FPblhashPinsTest1, "Unit::PblhashPins::hash pin ai", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("ai") == 0x4424F79F);
}

TEST_CASE_NAMED(FPblhashPinsTest2, "Unit::PblhashPins::hash pin autoaim", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("autoaim") == 0x68F63ADB);
}

TEST_CASE_NAMED(FPblhashPinsTest3, "Unit::PblhashPins::hash pin autonet", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("autonet") == 0xC23937B9);
}

TEST_CASE_NAMED(FPblhashPinsTest4, "Unit::PblhashPins::hash pin bandwidth", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("bandwidth") == 0x91739772);
}

TEST_CASE_NAMED(FPblhashPinsTest5, "Unit::PblhashPins::hash pin bootlagger", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("bootlagger") == 0x5D62BEB3);
}

TEST_CASE_NAMED(FPblhashPinsTest6, "Unit::PblhashPins::hash pin bots", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("bots") == 0xE181E077);
}

TEST_CASE_NAMED(FPblhashPinsTest7, "Unit::PblhashPins::hash pin connect", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("connect") == 0xAAE0CCF9);
}

TEST_CASE_NAMED(FPblhashPinsTest8, "Unit::PblhashPins::hash pin difficulty", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("difficulty") == 0x09CFDCF0);
}

TEST_CASE_NAMED(FPblhashPinsTest9, "Unit::PblhashPins::hash pin email", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("email") == 0x8A8753C7);
}

TEST_CASE_NAMED(FPblhashPinsTest10, "Unit::PblhashPins::hash pin firstperson", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("firstperson") == 0x6363D774);
}

TEST_CASE_NAMED(FPblhashPinsTest11, "Unit::PblhashPins::hash pin fixedfunction", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("fixedfunction") == 0xF5DD1FBD);
}

TEST_CASE_NAMED(FPblhashPinsTest12, "Unit::PblhashPins::hash pin fixedrate", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("fixedrate") == 0x7AC8BB2B);
}

TEST_CASE_NAMED(FPblhashPinsTest13, "Unit::PblhashPins::hash pin framelock", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("framelock") == 0xE3D8AB51);
}

TEST_CASE_NAMED(FPblhashPinsTest14, "Unit::PblhashPins::hash pin gamename", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("gamename") == 0x4F0C4904);
}

TEST_CASE_NAMED(FPblhashPinsTest15, "Unit::PblhashPins::hash pin gameslice", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("gameslice") == 0x98BF5C17);
}

TEST_CASE_NAMED(FPblhashPinsTest16, "Unit::PblhashPins::hash pin heroes", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("heroes") == 0xA8C4C105);
}

TEST_CASE_NAMED(FPblhashPinsTest17, "Unit::PblhashPins::hash pin heroruleplayer", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("heroruleplayer") == 0xECD7701E);
}

TEST_CASE_NAMED(FPblhashPinsTest18, "Unit::PblhashPins::hash pin herorulerespawn", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("herorulerespawn") == 0x3EC8DA95);
}

TEST_CASE_NAMED(FPblhashPinsTest19, "Unit::PblhashPins::hash pin heroruleteam", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("heroruleteam") == 0xF546DAFE);
}

TEST_CASE_NAMED(FPblhashPinsTest20, "Unit::PblhashPins::hash pin heroruleunlock", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("heroruleunlock") == 0x202DB217);
}

TEST_CASE_NAMED(FPblhashPinsTest21, "Unit::PblhashPins::hash pin install", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("install") == 0x8463E1A0);
}

TEST_CASE_NAMED(FPblhashPinsTest22, "Unit::PblhashPins::hash pin jitter", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("jitter") == 0x02049853);
}

TEST_CASE_NAMED(FPblhashPinsTest23, "Unit::PblhashPins::hash pin lan", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("lan") == 0x56608010);
}

TEST_CASE_NAMED(FPblhashPinsTest24, "Unit::PblhashPins::hash pin lang", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("lang") == 0x70EA3B55);
}

TEST_CASE_NAMED(FPblhashPinsTest25, "Unit::PblhashPins::hash pin latency", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("latency") == 0xF2FB8359);
}

TEST_CASE_NAMED(FPblhashPinsTest26, "Unit::PblhashPins::hash pin loadspam", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("loadspam") == 0xC53B7844);
}

TEST_CASE_NAMED(FPblhashPinsTest27, "Unit::PblhashPins::hash pin logfinalscore", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("logfinalscore") == 0x09C3E0FF);
}

TEST_CASE_NAMED(FPblhashPinsTest28, "Unit::PblhashPins::hash pin loss", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("loss") == 0xE7347B1A);
}

TEST_CASE_NAMED(FPblhashPinsTest29, "Unit::PblhashPins::hash pin name", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("name") == 0x8D39BDE6);
}

TEST_CASE_NAMED(FPblhashPinsTest30, "Unit::PblhashPins::hash pin netupdatesize", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("netupdatesize") == 0x1503F968);
}

TEST_CASE_NAMED(FPblhashPinsTest31, "Unit::PblhashPins::hash pin noai", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("noai") == 0x81BA46B8);
}

TEST_CASE_NAMED(FPblhashPinsTest32, "Unit::PblhashPins::hash pin noawards", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("noawards") == 0x2E325FB4);
}

TEST_CASE_NAMED(FPblhashPinsTest33, "Unit::PblhashPins::hash pin nodrag", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("nodrag") == 0x718E7AD4);
}

TEST_CASE_NAMED(FPblhashPinsTest34, "Unit::PblhashPins::hash pin noframelock", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("noframelock") == 0xCD154944);
}

TEST_CASE_NAMED(FPblhashPinsTest35, "Unit::PblhashPins::hash pin nointro", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("nointro") == 0x77E5E744);
}

TEST_CASE_NAMED(FPblhashPinsTest36, "Unit::PblhashPins::hash pin nojournal", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("nojournal") == 0x204F0D0D);
}

TEST_CASE_NAMED(FPblhashPinsTest37, "Unit::PblhashPins::hash pin nolegal", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("nolegal") == 0x3F0ECDEB);
}

TEST_CASE_NAMED(FPblhashPinsTest38, "Unit::PblhashPins::hash pin nomovies", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("nomovies") == 0xDABA84C3);
}

TEST_CASE_NAMED(FPblhashPinsTest39, "Unit::PblhashPins::hash pin nopoolerrors", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("nopoolerrors") == 0xD43981B7);
}

TEST_CASE_NAMED(FPblhashPinsTest40, "Unit::PblhashPins::hash pin nopregame", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("nopregame") == 0xF6976B61);
}

TEST_CASE_NAMED(FPblhashPinsTest41, "Unit::PblhashPins::hash pin norender", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("norender") == 0x75E8450E);
}

TEST_CASE_NAMED(FPblhashPinsTest42, "Unit::PblhashPins::hash pin nosound", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("nosound") == 0xF4553EF1);
}

TEST_CASE_NAMED(FPblhashPinsTest43, "Unit::PblhashPins::hash pin nostartupmusic", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("nostartupmusic") == 0x31694A02);
}

TEST_CASE_NAMED(FPblhashPinsTest44, "Unit::PblhashPins::hash pin noteamdamage", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("noteamdamage") == 0xE51023BA);
}

TEST_CASE_NAMED(FPblhashPinsTest45, "Unit::PblhashPins::hash pin nowait", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("nowait") == 0xBC494AFF);
}

TEST_CASE_NAMED(FPblhashPinsTest46, "Unit::PblhashPins::hash pin nowaitlate", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("nowaitlate") == 0x4BE2EE33);
}

TEST_CASE_NAMED(FPblhashPinsTest47, "Unit::PblhashPins::hash pin oneworld", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("oneworld") == 0x2264B8ED);
}

TEST_CASE_NAMED(FPblhashPinsTest48, "Unit::PblhashPins::hash pin password", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("password") == 0x364B5F18);
}

TEST_CASE_NAMED(FPblhashPinsTest49, "Unit::PblhashPins::hash pin playback", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("playback") == 0xCF8A43EC);
}

TEST_CASE_NAMED(FPblhashPinsTest50, "Unit::PblhashPins::hash pin playerlimit", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("playerlimit") == 0x3A00D703);
}

TEST_CASE_NAMED(FPblhashPinsTest51, "Unit::PblhashPins::hash pin poolerrors", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("poolerrors") == 0x351FC388);
}

TEST_CASE_NAMED(FPblhashPinsTest52, "Unit::PblhashPins::hash pin ppass", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("ppass") == 0xE0EBCA12);
}

TEST_CASE_NAMED(FPblhashPinsTest53, "Unit::PblhashPins::hash pin profile", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("profile") == 0x4674CAEE);
}

TEST_CASE_NAMED(FPblhashPinsTest54, "Unit::PblhashPins::hash pin randomize", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("randomize") == 0x331843BC);
}

TEST_CASE_NAMED(FPblhashPinsTest55, "Unit::PblhashPins::hash pin record", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("record") == 0x593058CC);
}

TEST_CASE_NAMED(FPblhashPinsTest56, "Unit::PblhashPins::hash pin resolution", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("resolution") == 0x1D215C8F);
}

TEST_CASE_NAMED(FPblhashPinsTest57, "Unit::PblhashPins::hash pin safemode", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("safemode") == 0x0451542B);
}

TEST_CASE_NAMED(FPblhashPinsTest58, "Unit::PblhashPins::hash pin showallsessions", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("showallsessions") == 0x4EAE0006);
}

TEST_CASE_NAMED(FPblhashPinsTest59, "Unit::PblhashPins::hash pin showlegal", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("showlegal") == 0x0481D555);
}

TEST_CASE_NAMED(FPblhashPinsTest60, "Unit::PblhashPins::hash pin skill", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("skill") == 0xCA15D036);
}

TEST_CASE_NAMED(FPblhashPinsTest61, "Unit::PblhashPins::hash pin skiplegal", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("skiplegal") == 0xBBB3B003);
}

TEST_CASE_NAMED(FPblhashPinsTest62, "Unit::PblhashPins::hash pin soundwarning", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("soundwarning") == 0xC16650C8);
}

TEST_CASE_NAMED(FPblhashPinsTest63, "Unit::PblhashPins::hash pin spaceassaultscorelimit", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("spaceassaultscorelimit") == 0xD5C0B6F5);
}

TEST_CASE_NAMED(FPblhashPinsTest64, "Unit::PblhashPins::hash pin team", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("team") == 0xA2FD7D0C);
}

TEST_CASE_NAMED(FPblhashPinsTest65, "Unit::PblhashPins::hash pin throttle", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("throttle") == 0x378F32DD);
}

TEST_CASE_NAMED(FPblhashPinsTest66, "Unit::PblhashPins::hash pin voicemode", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("voicemode") == 0xDADFA856);
}

TEST_CASE_NAMED(FPblhashPinsTest67, "Unit::PblhashPins::hash pin win", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("win") == 0x0BF7CB59);
}

TEST_CASE_NAMED(FPblhashPinsTest68, "Unit::PblhashPins::hash pin borderless", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("borderless") == 0x49DA825A);
    CHECK((unsigned int)PblHash("borderless") == DisplayCfg::kHashBorderless);
}

TEST_CASE_NAMED(FPblhashPinsTest69, "Unit::PblhashPins::hash pin adapter", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("adapter") == 0xE7CAE794);
    CHECK((unsigned int)PblHash("adapter") == DisplayCfg::kHashAdapter);
}

TEST_CASE_NAMED(FPblhashPinsTest70, "Unit::PblhashPins::hash pin refresh", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("refresh") == 0xD4F26634);
    CHECK((unsigned int)PblHash("refresh") == DisplayCfg::kHashRefresh);
}

TEST_CASE_NAMED(FPblhashPinsTest71, "Unit::PblhashPins::hash pin vsync", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("vsync") == 0x06F8F066);
    CHECK((unsigned int)PblHash("vsync") == DisplayCfg::kHashVsync);
}

TEST_CASE_NAMED(FPblhashPinsTest72, "Unit::PblhashPins::hash pin fps", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("fps") == 0xBDD34AA8);
    CHECK((unsigned int)PblHash("fps") == DisplayCfg::kHashFps);
}

TEST_CASE_NAMED(FPblhashPinsTest73, "Unit::PblhashPins::hash pin nowaitquery", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("nowaitquery") == 0xFF222165);
    CHECK((unsigned int)PblHash("nowaitquery") == DisplayCfg::kHashNoWaitQuery);
}

TEST_CASE_NAMED(FPblhashPinsTest74, "Unit::PblhashPins::hash pin nodpi", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("nodpi") == 0x5B1931B5);
    CHECK((unsigned int)PblHash("nodpi") == DisplayCfg::kHashNoDpi);
}

TEST_CASE_NAMED(FPblhashPinsTest75, "Unit::PblhashPins::hash pin swcursor", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("swcursor") == 0x21567C6D);
    CHECK((unsigned int)PblHash("swcursor") == DisplayCfg::kHashSwCursor);
}

TEST_CASE_NAMED(FPblhashPinsTest76, "Unit::PblhashPins::hash pin displayini", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("displayini") == 0x8CDD8845);
    CHECK((unsigned int)PblHash("displayini") == DisplayCfg::kHashDisplayIni);
}

TEST_CASE_NAMED(FPblhashPinsTest77, "Unit::PblhashPins::hash pin is case insensitive", "[SmokeFilter]")
{
    CHECK((unsigned int)PblHash("NoRender") == (unsigned int)PblHash("norender"));
}
