package org.openswbf2.riderlink

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.nio.ByteBuffer
import java.nio.channels.FileChannel
import java.nio.file.Files
import java.nio.file.StandardOpenOption

class PortFileTest {
    @get:Rule
    val temp = TemporaryFolder()

    private val token = "3f9a0c2e7b1d4e8f9a0b1c2d3e4f5a6b"

    @Test
    fun readsPortAndTokenAndThePidFromTheName() {
        assertEquals(PortEntry(24108, 53557, token), PortFile.parse("24108.port", "53557 $token\n"))
        assertEquals(PortEntry(7, 1, token), PortFile.parse("7.port", "1 $token"))
    }

    @Test
    fun ignoresBadFiles() {
        assertNull(PortFile.parse("24108.port", "0 $token\n"))
        assertNull(PortFile.parse("24108.port", "65536 $token\n"))
        assertNull(PortFile.parse("24108.port", "port $token\n"))
        assertNull(PortFile.parse("24108.port", "53557 ${token.dropLast(1)}\n"))
        assertNull(PortFile.parse("24108.port", "53557 ${token.uppercase()}\n"))
        assertNull(PortFile.parse("24108.port", "53557 $token extra\n"))
        assertNull(PortFile.parse("24108.port", "53557  $token\n"))
        assertNull(PortFile.parse("24108.port", ""))
        assertNull(PortFile.parse("game.port", "53557 $token\n"))
        assertNull(PortFile.parse("24108.port.tmp", "53557 $token\n"))
        assertNull(PortFile.parse("24108.txt", "53557 $token\n"))
        assertNull(PortFile.parse("-1.port", "53557 $token\n"))
    }

    @Test
    fun liveDropsDeadPidsAndChoosesWaitingOneOrSeveral() {
        val a = PortEntry(100, 5000, token)
        val b = PortEntry(200, 5001, token)
        val c = PortEntry(300, 5002, token)
        assertEquals(Discovery.Waiting, PortFile.live(emptyList()) { true })
        assertEquals(Discovery.Waiting, PortFile.live(listOf(a, b)) { false })
        assertEquals(Discovery.One(b), PortFile.live(listOf(a, b)) { it == 200L })
        assertEquals(Discovery.Several(listOf(a, c)), PortFile.live(listOf(c, b, a)) { it != 200L })
    }

    @Test
    fun listsOnlyValidPortFilesInTheFolder() {
        val folder = temp.root.toPath()
        Files.writeString(folder.resolve("100.port"), "5000 $token\n")
        Files.writeString(folder.resolve("200.port"), "bad\n")
        Files.writeString(folder.resolve("300.txt"), "5002 $token\n")
        assertEquals(listOf(PortEntry(100, 5000, token)), PortFile.list(folder))
        assertEquals(emptyList<PortEntry>(), PortFile.list(folder.resolve("missing")))
    }

    @Test
    fun readsAFileHeldOpenWithDeleteOnClose() {
        val file = temp.root.toPath().resolve("4242.port")
        FileChannel.open(
            file,
            StandardOpenOption.CREATE_NEW,
            StandardOpenOption.WRITE,
            StandardOpenOption.DELETE_ON_CLOSE,
        ).use { channel ->
            channel.write(ByteBuffer.wrap("53591 $token\n".toByteArray(Charsets.US_ASCII)))
            channel.force(true)
            assertEquals(PortEntry(4242, 53591, token), PortFile.read(file))
        }
        assertFalse(Files.exists(file))
        assertNull(PortFile.read(file))
    }
}
