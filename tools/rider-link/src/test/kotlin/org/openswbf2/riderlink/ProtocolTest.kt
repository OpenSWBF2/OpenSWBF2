package org.openswbf2.riderlink

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class ProtocolTest {
    @Test
    fun parsesTheGameHello() {
        assertEquals(Message.Hello(1, 123, "x64", "Test"), Protocol.parse("HELLO 1 123 x64 Test"))
        assertEquals(Message.Hello(1, 24108, "Win32", "Debug"), Protocol.parse("HELLO 1 24108 Win32 Debug"))
        assertEquals(Message.Hello(2, 0, "", ""), Protocol.parse("HELLO 2 123 x64 Test"))
    }

    @Test
    fun parsesEveryState() {
        assertEquals(Message.State(PlayState.Idle), Protocol.parse("STATE Idle"))
        assertEquals(Message.State(PlayState.Play), Protocol.parse("STATE Play"))
        assertEquals(Message.State(PlayState.Pause), Protocol.parse("STATE Pause"))
    }

    @Test
    fun keepsLogTextByteForByte() {
        val line = "[2026.11.25-16.57.28:673][ 85]LogNet: Inform: RiderLink step sim_frames=1"
        assertEquals(Message.Log(line), Protocol.parse("LOG $line"))
        assertEquals(Message.Log("   leading spaces kept  "), Protocol.parse("LOG    leading spaces kept  "))
        assertEquals(Message.Log(""), Protocol.parse("LOG "))
        assertEquals(
            Message.Log("TEST - critical system destroyd:    rep_cap2_bridge    pointValue:    18"),
            Protocol.parse("LOG TEST - critical system destroyd:    rep_cap2_bridge    pointValue:    18"),
        )
    }

    @Test
    fun parsesDropped() {
        assertEquals(Message.Dropped(7), Protocol.parse("DROPPED 7"))
        assertEquals(Message.Dropped(4294967295), Protocol.parse("DROPPED 4294967295"))
    }

    @Test
    fun badLinesAreUnknownAndNeverThrow() {
        for (line in listOf(
            "", "STATE Other", "STATE", "STATE play", "DROPPED x", "DROPPED", "DROPPED -1", "DROPPED 7 8",
            "HELLO", "HELLO x", "HELLO 1", "HELLO 1 123 x64", "HELLO 1 123 ARM Test", "HELLO 1 123 x64 Release",
            "HELLO 1 abc x64 Test", "HELLO 1 123 x64 Test extra", "HELLO 99999999999 1 x64 Test",
            "log text", "PING", " LOG x", "��",
        )) {
            assertEquals(line, Message.Unknown(line), Protocol.parse(line))
        }
    }

    @Test
    fun formatsTheClientLines() {
        assertEquals("HELLO 1 0123456789abcdef0123456789abcdef", Protocol.hello("0123456789abcdef0123456789abcdef"))
        assertEquals("PLAY geo1c_con", Protocol.play("geo1c_con"))
        assertEquals("PAUSE", Protocol.PAUSE)
        assertEquals("RESUME", Protocol.RESUME)
        assertEquals("STEP", Protocol.STEP)
        assertEquals("STOP", Protocol.STOP)
    }

    @Test
    fun refusesToFormatABadMission() {
        assertEquals("PLAY " + "a".repeat(63), Protocol.play("a".repeat(63)))
        for (mission in listOf("", "a".repeat(64), "geo1c con", "geo1c-con", "geo1c_con\n", "géo", "geo1c_con;STOP")) {
            assertNull(mission, Protocol.play(mission))
        }
    }
}
