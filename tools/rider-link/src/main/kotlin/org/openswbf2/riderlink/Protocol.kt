package org.openswbf2.riderlink

enum class PlayState { Idle, Play, Pause }

sealed class Message {
    data class Hello(val version: Int, val pid: Long, val platform: String, val config: String) : Message()
    data class State(val state: PlayState) : Message()
    data class Log(val text: String) : Message()
    data class Dropped(val count: Long) : Message()
    data class Unknown(val line: String) : Message()
}

object Protocol {
    const val VERSION = 1

    private val MISSION = Regex("[A-Za-z0-9_]{1,63}")
    private val NUMBER = Regex("[0-9]{1,10}")
    private val PLATFORMS = setOf("Win32", "x64")
    private val CONFIGS = setOf("Debug", "Test", "Asan")

    fun parse(line: String): Message {
        val space = line.indexOf(' ')
        val type = if (space < 0) line else line.substring(0, space)
        val rest = if (space < 0) "" else line.substring(space + 1)
        return when (type) {
            "LOG" -> Message.Log(rest)
            "STATE" -> PlayState.entries.firstOrNull { it.name == rest }?.let { Message.State(it) }
            "DROPPED" -> if (NUMBER.matches(rest)) Message.Dropped(rest.toLong()) else null
            "HELLO" -> parseHello(rest.split(' '))
            else -> null
        } ?: Message.Unknown(line)
    }

    private fun parseHello(words: List<String>): Message.Hello? {
        if (!NUMBER.matches(words[0])) {
            return null
        }
        val version = words[0].toLong()
        if (version != VERSION.toLong()) {
            return if (version <= Int.MAX_VALUE) Message.Hello(version.toInt(), 0, "", "") else null
        }
        if (words.size != 4 || !NUMBER.matches(words[1]) || words[2] !in PLATFORMS || words[3] !in CONFIGS) {
            return null
        }
        return Message.Hello(VERSION, words[1].toLong(), words[2], words[3])
    }

    fun isMissionName(name: String): Boolean = MISSION.matches(name)

    fun hello(token: String): String = "HELLO $VERSION $token"

    fun play(mission: String): String? = if (isMissionName(mission)) "PLAY $mission" else null

    const val PAUSE = "PAUSE"
    const val RESUME = "RESUME"
    const val STEP = "STEP"
    const val STOP = "STOP"
}
