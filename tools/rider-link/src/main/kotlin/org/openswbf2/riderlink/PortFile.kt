package org.openswbf2.riderlink

import java.io.IOException
import java.nio.file.Files
import java.nio.file.Path

data class PortEntry(val pid: Long, val port: Int, val token: String)

sealed class Discovery {
    object Waiting : Discovery()
    data class One(val entry: PortEntry) : Discovery()
    data class Several(val entries: List<PortEntry>) : Discovery()
}

object PortFile {
    private val NAME = Regex("([0-9]{1,10})\\.port")
    private val CONTENT = Regex("([0-9]{1,5}) ([0-9a-f]{32})\r?\n?")

    fun parse(fileName: String, content: String): PortEntry? {
        val name = NAME.matchEntire(fileName) ?: return null
        val body = CONTENT.matchEntire(content) ?: return null
        val port = body.groupValues[1].toInt()
        if (port !in 1..65535) {
            return null
        }
        return PortEntry(name.groupValues[1].toLong(), port, body.groupValues[2])
    }

    fun read(file: Path): PortEntry? {
        val bytes = try {
            Files.readAllBytes(file)
        } catch (e: IOException) {
            return null
        }
        if (bytes.size > 64) {
            return null
        }
        return parse(file.fileName.toString(), String(bytes, Charsets.US_ASCII))
    }

    fun list(folder: Path): List<PortEntry> {
        if (!Files.isDirectory(folder)) {
            return emptyList()
        }
        val files = try {
            Files.newDirectoryStream(folder, "*.port").use { it.toList() }
        } catch (e: IOException) {
            return emptyList()
        }
        return files.mapNotNull { read(it) }
    }

    fun live(entries: List<PortEntry>, isAlive: (Long) -> Boolean): Discovery {
        val alive = entries.filter { isAlive(it.pid) }.sortedBy { it.pid }
        return when (alive.size) {
            0 -> Discovery.Waiting
            1 -> Discovery.One(alive[0])
            else -> Discovery.Several(alive)
        }
    }
}
