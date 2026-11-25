package org.openswbf2.riderlink

import org.junit.Assert.assertEquals
import org.junit.Test

class LogLineTest {
    private fun unparsed(text: String) = LogLine("", "", null, null, text)

    @Test
    fun splitsTheTimedForm() {
        assertEquals(
            LogLine("2026.11.25-16.57.30:463", "584", "LogSim", Verbosity.Inform,
                "Sim digest turns=1500 time=50.000 rand=234993775/2651921600 snd=49.923 state=0x65CD7716"),
            LogLine.split("[2026.11.25-16.57.30:463][584]LogSim: Inform: Sim digest turns=1500 time=50.000 rand=234993775/2651921600 snd=49.923 state=0x65CD7716"),
        )
        assertEquals(
            LogLine("2026.11.25-16.57.28:664", "  2", "LogNet", Verbosity.Warning,
                "RiderLink refused PAUSE: it is not valid in this state"),
            LogLine.split("[2026.11.25-16.57.28:664][  2]LogNet: Warning: RiderLink refused PAUSE: it is not valid in this state"),
        )
        assertEquals(
            LogLine("2026.11.25-16.57.28:056", "  0", "LogMemory", Verbosity.Warning,
                "Memory pool \"ParticleTransformer::SizeTransf\" is full; raise count to at least 1025"),
            LogLine.split("[2026.11.25-16.57.28:056][  0]LogMemory: Warning: Memory pool \"ParticleTransformer::SizeTransf\" is full; raise count to at least 1025"),
        )
    }

    @Test
    fun splitsTheNoLogTimesForm() {
        assertEquals(
            LogLine("", "", "LogNet", Verbosity.Warning, "RiderLink: no mission named no_such_mission_51_2"),
            LogLine.split("LogNet: Warning: RiderLink: no mission named no_such_mission_51_2"),
        )
        for (v in Verbosity.entries) {
            assertEquals(LogLine("", "", "LogCore", v, "x"), LogLine.split("LogCore: ${v.name}: x"))
        }
    }

    @Test
    fun matchesTheCppParserVectors() {
        assertEquals(
            LogLine("2026.11.25-00.00.00:000", "  7", "LogSim", Verbosity.Warning, "pool full"),
            LogLine.split("[2026.11.25-00.00.00:000][  7]LogSim: Warning: pool full"),
        )
        assertEquals(
            LogLine("", "", "LogMemory", Verbosity.Error, "alloc failure heap Runtime (3) size 268435456"),
            LogLine.split("LogMemory: Error: alloc failure heap Runtime (3) size 268435456"),
        )
        for (line in listOf(
            "[2026.11.25-00.00.00:000][  7]: Error: no category",
            "Error: no category",
            "LogSim: Loud: unknown severity",
            "[t=1.000] [f=7] LogSim Warning x.cpp:1: old layout",
            "[2026.11.25-00.00.00:0x0][  7]LogSim: Warning: bad timestamp",
            "FATAL: exception 0xC0000005 at 0x00401000 (BattlefrontII.exe+0x1000)",
        )) {
            assertEquals(unparsed(line), LogLine.split(line))
        }
        val long = LogLine.split("LogCore: Error: " + "x".repeat(3000))
        assertEquals(LogLine("", "", "LogCore", Verbosity.Error, "x".repeat(LogLine.CAP - 16)), long)
    }

    @Test
    fun aLuaPrintLineIsWholeWithNoCategory() {
        val lua = "TEST - critical system destroyd:    rep_cap2_bridge    pointValue:    18"
        assertEquals(unparsed(lua), LogLine.split(lua))
        assertEquals(unparsed(""), LogLine.split(""))
    }
}
