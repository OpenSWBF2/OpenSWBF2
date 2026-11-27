package org.openswbf2.riderlink

data class LogLineLayout(
    val showTimestamps: Boolean,
    val showVerbosity: Boolean,
    val alignMessages: Boolean,
    val categoryWidth: Int,
)

object LogLineText {
    const val TIME_WIDTH = 29
    const val VERBOSITY_WIDTH = 12

    private fun spaces(out: MutableList<String>, n: Int = 1) {
        if (n <= 0) return
        out.add(" ".repeat(n))
    }

    fun info(s: LogLine, layout: LogLineLayout): List<String> {
        val out = arrayListOf<String>()
        if (layout.showTimestamps) {
            val timeString = s.timestamp
            out.add(timeString)
            spaces(out, TIME_WIDTH + 1 - timeString.length)
        }

        if (layout.alignMessages) {
            if (layout.showVerbosity) {
                val verbosityString = s.verbosity?.name.orEmpty()
                out.add(verbosityString)
                spaces(out, VERBOSITY_WIDTH - verbosityString.length + 1)
            }

            val category = s.category.orEmpty()
            out.add(category)
            spaces(out, layout.categoryWidth - category.length)
            spaces(out, 1)
        } else if (s.category != null) {
            if (layout.showVerbosity) {
                val verbosityString = s.verbosity?.name.orEmpty()
                out.add(verbosityString)
                out.add(": ")
            }

            out.add(s.category.orEmpty())
            out.add(": ")
        }
        return out
    }
}
