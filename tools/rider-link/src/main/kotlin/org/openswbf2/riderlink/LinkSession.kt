package org.openswbf2.riderlink

import java.io.ByteArrayOutputStream
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream
import java.net.InetAddress
import java.net.InetSocketAddress
import java.net.Socket

interface LinkListener {
    fun hello(hello: Message.Hello)
    fun state(state: PlayState)
    fun log(text: String)
    fun dropped(count: Long)
}

sealed class SessionEnd {
    object Disconnected : SessionEnd()
    object Closed : SessionEnd()
    data class WrongVersion(val version: Int) : SessionEnd()
    data class Failed(val reason: String) : SessionEnd()
}

class LinkSession(private val entry: PortEntry, private val listener: LinkListener) {
    companion object {
        const val LINE_CAP = 4096
    }

    private val socket = Socket()

    @Volatile
    private var closed = false

    fun run(): SessionEnd {
        return try {
            socket.connect(InetSocketAddress(InetAddress.getLoopbackAddress(), entry.port))
            send(Protocol.hello(entry.token))
            read(socket.getInputStream())
        } catch (e: IOException) {
            if (closed) SessionEnd.Closed else SessionEnd.Failed(e.message ?: e.javaClass.simpleName)
        } finally {
            socket.close()
        }
    }

    fun send(line: String) {
        val out: OutputStream = socket.getOutputStream()
        synchronized(this) {
            out.write((line + "\n").toByteArray(Charsets.UTF_8))
            out.flush()
        }
    }

    fun close() {
        closed = true
        socket.close()
    }

    private fun read(input: InputStream): SessionEnd {
        val stream = input.buffered()
        val line = ByteArrayOutputStream()
        while (true) {
            val b = stream.read()
            if (b < 0) {
                return if (closed) SessionEnd.Closed else SessionEnd.Disconnected
            }
            if (b != '\n'.code) {
                if (line.size() < LINE_CAP) {
                    line.write(b)
                }
                continue
            }
            val text = String(line.toByteArray(), Charsets.UTF_8)
            line.reset()
            when (val message = Protocol.parse(text)) {
                is Message.Hello -> {
                    if (message.version != Protocol.VERSION) {
                        return SessionEnd.WrongVersion(message.version)
                    }
                    listener.hello(message)
                }
                is Message.State -> listener.state(message.state)
                is Message.Log -> listener.log(message.text)
                is Message.Dropped -> listener.dropped(message.count)
                is Message.Unknown -> Unit
            }
        }
    }
}
