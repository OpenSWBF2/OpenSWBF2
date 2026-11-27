package org.openswbf2.riderlink

enum class Verbosity {
    Verbose,
    Inform,
    Warning,
    Error,
    Bug,
    Fatal,
}

fun Verbosity?.isError(): Boolean = this == Verbosity.Error || this == Verbosity.Bug

data class LogLine(
    val timestamp: String,
    val frame: String,
    val category: String?,
    val verbosity: Verbosity?,
    val message: String,
) {
    val filterCategory: String
        get() = category ?: NO_CATEGORY

    companion object {
        const val CAP = 1024
        const val NO_CATEGORY = "(no category)"

        private val LINE = Regex(
            """^(?:\[(\d{4}\.\d{2}\.\d{2}-\d{2}\.\d{2}\.\d{2}:\d{3})])?(?:\[([0-9 ]{3})])?([A-Za-z0-9_]+): (Verbose|Inform|Warning|Error|Bug|Fatal): (.*)$""",
            RegexOption.DOT_MATCHES_ALL,
        )

        fun split(line: String): LogLine {
            val text = line.take(CAP)
            val match = LINE.matchEntire(text) ?: return LogLine("", "", null, null, text)
            val (timestamp, frame, category, verbosity, message) = match.destructured
            return LogLine(timestamp, frame, category, Verbosity.valueOf(verbosity), message)
        }
    }
}
