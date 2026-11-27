package org.openswbf2.riderlink

import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import java.io.BufferedReader
import java.io.InputStreamReader
import java.net.InetAddress
import java.net.ServerSocket
import java.net.Socket
import java.util.concurrent.LinkedBlockingQueue
import kotlin.concurrent.thread

class LinkSessionTest {
    private val token = "3f9a0c2e7b1d4e8f9a0b1c2d3e4f5a6b"
    private val events = LinkedBlockingQueue<Any>()
    private lateinit var server: ServerSocket
    private lateinit var session: LinkSession
    private lateinit var runner: Thread

    private data class End(val end: SessionEnd)

    private val listener = object : LinkListener {
        override fun hello(hello: Message.Hello) {
            events.put(hello)
        }

        override fun state(state: PlayState) {
            events.put(state)
        }

        override fun log(text: String) {
            events.put(Message.Log(text))
        }

        override fun dropped(count: Long) {
            events.put(Message.Dropped(count))
        }
    }

    @Before
    fun start() {
        server = ServerSocket(0, 1, InetAddress.getLoopbackAddress())
        session = LinkSession(PortEntry(4242, server.localPort, token), listener)
        runner = thread { events.put(End(session.run())) }
    }

    @After
    fun stop() {
        session.close()
        runner.join()
        server.close()
    }

    private fun accept(): Pair<Socket, BufferedReader> {
        val game = server.accept()
        return game to BufferedReader(InputStreamReader(game.getInputStream(), Charsets.UTF_8))
    }

    private fun Socket.write(text: String) {
        getOutputStream().write(text.toByteArray(Charsets.UTF_8))
        getOutputStream().flush()
    }

    @Test
    fun sendsHelloFirst() {
        val (game, reader) = accept()
        game.use {
            assertEquals("HELLO 1 $token", reader.readLine())
        }
        assertEquals(End(SessionEnd.Disconnected), events.take())
    }

    @Test
    fun reportsHelloStatesLogsAndDropsInOrder() {
        val (game, reader) = accept()
        game.use {
            reader.readLine()
            game.write(
                "HELLO 1 4242 Win32 Test\n" +
                    "STATE Pause\n" +
                    "LOG [2026.11.25-16.57.28:673][ 85]LogNet: Inform: RiderLink step sim_frames=1\n" +
                    "DROPPED 7\n" +
                    "STATE Pause\n" +
                    "LOG TEST - critical system destroyd:    rep_cap2_bridge    pointValue:    18\n" +
                    "PING 1\n" +
                    "STATE Play\n",
            )
        }
        assertEquals(Message.Hello(1, 4242, "Win32", "Test"), events.take())
        assertEquals(PlayState.Pause, events.take())
        assertEquals(Message.Log("[2026.11.25-16.57.28:673][ 85]LogNet: Inform: RiderLink step sim_frames=1"), events.take())
        assertEquals(Message.Dropped(7), events.take())
        assertEquals(PlayState.Pause, events.take())
        assertEquals(Message.Log("TEST - critical system destroyd:    rep_cap2_bridge    pointValue:    18"), events.take())
        assertEquals(PlayState.Play, events.take())
        assertEquals(End(SessionEnd.Disconnected), events.take())
    }

    @Test
    fun aRefusedCommandComesBackAsALogLineOnly() {
        val (game, reader) = accept()
        game.use {
            reader.readLine()
            game.write("HELLO 1 4242 x64 Debug\nSTATE Idle\n")
            assertEquals(Message.Hello(1, 4242, "x64", "Debug"), events.take())
            assertEquals(PlayState.Idle, events.take())
            session.send(Protocol.PAUSE)
            assertEquals("PAUSE", reader.readLine())
            game.write("LOG [2026.11.25-16.57.28:664][  2]LogNet: Warning: RiderLink refused PAUSE: it is not valid in this state\n")
        }
        assertEquals(
            Message.Log("[2026.11.25-16.57.28:664][  2]LogNet: Warning: RiderLink refused PAUSE: it is not valid in this state"),
            events.take(),
        )
        assertEquals(End(SessionEnd.Disconnected), events.take())
    }

    @Test
    fun anotherHelloVersionEndsTheSession() {
        val (game, reader) = accept()
        game.use {
            reader.readLine()
            game.write("HELLO 2 4242 x64 Test\nSTATE Play\n")
            assertEquals(End(SessionEnd.WrongVersion(2)), events.take())
        }
        assertEquals(0, events.size)
    }

    @Test
    fun invalidUtf8IsReplacedAndTheSessionGoesOn() {
        val (game, reader) = accept()
        game.use {
            reader.readLine()
            game.getOutputStream().write(byteArrayOf(0x4C, 0x4F, 0x47, 0x20, 0xFF.toByte(), 0xC3.toByte(), 0x61, 0x0A))
            game.write("STATE Play\n")
        }
        assertEquals(Message.Log("��a"), events.take())
        assertEquals(PlayState.Play, events.take())
        assertEquals(End(SessionEnd.Disconnected), events.take())
    }

    @Test
    fun closeFromAnotherThreadEndsABlockedRead() {
        val (game, reader) = accept()
        game.use {
            reader.readLine()
            game.write("HELLO 1 4242 Win32 Test\nSTATE Play\n")
            assertEquals(Message.Hello(1, 4242, "Win32", "Test"), events.take())
            assertEquals(PlayState.Play, events.take())
            session.close()
            assertEquals(End(SessionEnd.Closed), events.take())
        }
    }
}
