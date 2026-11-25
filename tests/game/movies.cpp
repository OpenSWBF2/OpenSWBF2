#include "third_party/doctest/doctest.h"
#include "harness/tags.h"

#include "harness/compare.h"
#include "harness/gamerun.h"
#include "game/standard_case.h"
#include "testutil.h"

namespace
{

void Put(std::string& sBytes, size_t uAt, unsigned int uValue)
{
    for (int i = 0; i < 4; ++i)
    {
        sBytes[uAt + i] = (char)((uValue >> (8 * i)) & 0xFF);
    }
}

std::string MovieBytes(bool bMagic, bool bWidth)
{
    std::string sBik(68, '\0');
    sBik.replace(0, 4, bMagic ? "XYZi" : "BIKi");
    Put(sBik, 4, 60);
    Put(sBik, 8, 1);
    Put(sBik, 12, 16);
    Put(sBik, 20, bWidth ? 1025 : 800);
    Put(sBik, 24, 600);
    Put(sBik, 28, 30);
    Put(sBik, 32, 1);
    Put(sBik, 44, 53);
    Put(sBik, 48, 68);
    std::string sFile(6144, '\0');
    sFile.replace(0, 4, "ucfb");
    Put(sFile, 4, 6136);
    Put(sFile, 8, 0x2F1F7060u);
    Put(sFile, 12, 6128);
    Put(sFile, 16, 0xF9F40518u);
    Put(sFile, 20, 6120);
    Put(sFile, 24, 0x0FB40705u);
    Put(sFile, 28, 2008);
    static const unsigned int sauWords[] = {0x8D39BDE6u, 0x0BADF11Eu, 0x40FBDEBDu, 1,           0xB969BE96u,
                                            0xF3C342E6u, 0x8D39BDE6u, 0xB4A21ED9u, 0x83D03615u, 68,
                                            0x809608B6u, 4028,        0x7DB1D043u};
    for (size_t i = 0; i < sizeof(sauWords) / sizeof(sauWords[0]); ++i)
    {
        Put(sFile, 32 + 4 * i, sauWords[i]);
    }
    Put(sFile, 2040, 0xD872E2A5u);
    Put(sFile, 2044, 4096);
    sFile.replace(2048, sBik.size(), sBik);
    return sFile;
}

const char* const kCampaignEvents =
    "opens:1..,closes:1..,rejects:0,ends:1..,mismatch:0,endseg:0x2AB9757F,endframes:1369,endshown:1369,endheader:1369,"
    "state:mission";

void RunMovieCase(const char* pcName, const std::string& sArgs, const char* pcEvents, unsigned int uSoak,
                  const char* pcFile = 0, const std::string& sFileBytes = std::string())
{
    GameRun kRun(pcName);
    kRun.AddArg(StandardArgs(pcName, sArgs));
    if (pcFile)
    {
        kRun.WriteWorkerFile(pcFile, sFileBytes);
    }
    AddStandardTests(kRun);
    kRun.AddInGameTest("Game.Movie.Events", pcEvents);
    kRun.Start();
    kRun.Wait();
    CheckStandardRun(kRun, pcName, uSoak);
}

std::string Shell(const std::string& sExtra)
{
    return RenderTail() + " " + sExtra;
}

}

TEST_CASE_NAMED(FMoviesTest1, "Game::Standard::campaign-movies", "[ProductFilter][Standard]")
{
    RunMovieCase("campaign-movies", "nab2g_c " + RenderTail() + " -handsoff -win -exitonmovieclose", kCampaignEvents, 0);
}

TEST_CASE_NAMED(FMoviesTest2, "Game::Standard::ingame-movie-digest", "[ProductFilter][Standard]")
{
    GameRun kRun("ingame-movie-digest");
    kRun.AddArg(StandardArgs("ingame-movie-digest", "nab2g_c " + RenderTail() + " -handsoff -win -soakturns 1500"));
    AddStandardTests(kRun);
    kRun.AddInGameTest("Game.Movie.Events", kCampaignEvents);
    kRun.Start();
    kRun.Wait();
    CheckStandardRun(kRun, "ingame-movie-digest", 1500);
    std::string sTable;
    std::string sRow;
    std::string sWhy;
    REQUIRE(ReadRepoText("tests\\baseline\\v143\\movie-goldens.txt", sTable));
    REQUIRE(FindTableRow(sTable, "binkw32 ingame digest", "ingame-movie-digest", sRow));
    CHECK_MESSAGE(CompareDigestRow(ReadRunLog(kRun), sRow, sWhy), "ingame-movie-digest against movie-goldens.txt: " << sWhy);
}

TEST_CASE_NAMED(FMoviesTest3, "Game::Standard::movie-loop", "[ProductFilter][Standard]")
{
    RunMovieCase("movie-loop", Shell("-movieloops 2 -handsoff -win"),
                 "opens:2..,closes:2..,ends:2..,notify:2..,rejects:0,mismatch:0,endseg:0xB4A21ED9,endframes:1349,"
                 "endshown:1349,endheader:1349,state:shell",
                 0);
}

TEST_CASE_NAMED(FMoviesTest4, "Game::Standard::movie-reject-width", "[ProductFilter][Standard]")
{
    RunMovieCase("movie-reject-width", Shell("-movietest reject-width.mvs:0xB4A21ED9:30 -movielocal -handsoff -win"),
                 "rejects:1,reject:width,opens:0,notify:1..,testframes:0,state:shell", 0, "reject-width.mvs",
                 MovieBytes(false, true));
}

TEST_CASE_NAMED(FMoviesTest5, "Game::Standard::movie-reject-magic", "[ProductFilter][Standard]")
{
    RunMovieCase("movie-reject-magic", Shell("-movietest reject-magic.mvs:0xB4A21ED9:30 -movielocal -handsoff -win"),
                 "rejects:1,reject:magic,opens:0,notify:1..,testframes:0,state:shell", 0, "reject-magic.mvs",
                 MovieBytes(true, false));
}

TEST_CASE_NAMED(FMoviesTest6, "Game::Standard::movie-reject-loop", "[ProductFilter][Standard]")
{
    RunMovieCase("movie-reject-loop", Shell("-movieloops 2 -movielocal -handsoff -win"),
                 "rejects:1,reject:width,opens:0,notify:2..,state:shell", 0, "shell.mvs", MovieBytes(false, true));
}

TEST_CASE_NAMED(FMoviesTest7, "Game::Standard::movie-texture-wait", "[ProductFilter][Standard]")
{
    RunMovieCase("movie-texture-wait", Shell("-movietest shell.mvs:0xB4A21ED9:30 -moviefault hold -handsoff -win"),
                 "opens:1..,testframes:30,waits:1..,rejects:0,state:shell", 0);
}

TEST_CASE_NAMED(FMoviesTest8, "Game::Standard::movie-texture-reject", "[ProductFilter][Standard]")
{
    RunMovieCase("movie-texture-reject", Shell("-movietest shell.mvs:0xB4A21ED9:30 -moviefault texture -handsoff -win"),
                 "rejects:1,reject:texture,opens:0,notify:1..,testframes:0,state:shell", 0);
}

TEST_CASE_NAMED(FMoviesTest9, "Game::Standard::movie-surface-fail", "[ProductFilter][Standard]")
{
    RunMovieCase("movie-surface-fail", Shell("-movietest shell.mvs:0xB4A21ED9:30 -moviefault surface -handsoff -win"),
                 "opens:1..,ends:1..,endseg:0xB4A21ED9,endframes:1349,endshown:0,endheader:1349,endwaits:0,hashed:0,"
                 "notify:1..,testframes:0,rejects:0,state:shell",
                 0);
}

TEST_CASE_NAMED(FMoviesTest10, "Game::Standard::movie-skip", "[ProductFilter][Standard]")
{
    RunMovieCase("movie-skip", "nab2g_c " + RenderTail() + " -handsoff -win -movieskip 300 -soakturns 300",
                 "opens:1..,frame300:0x2AB9757F/43BDE01AE68ADC34,skip:300,closes:1..,ends:0,rejects:0,state:mission", 300);
}
