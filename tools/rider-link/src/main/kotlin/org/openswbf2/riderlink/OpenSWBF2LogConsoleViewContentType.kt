package org.openswbf2.riderlink

import com.intellij.execution.ui.ConsoleViewContentType
import com.intellij.openapi.editor.colors.TextAttributesKey
import org.jetbrains.annotations.NonNls

class OpenSWBF2LogConsoleViewContentType(@NonNls name: String, textAttributesKey: TextAttributesKey)
    : ConsoleViewContentType(name, textAttributesKey) {

    companion object {
        val OPENSWBF2_LOG_FATAL_KEY = TextAttributesKey.createTextAttributesKey("OPENSWBF2_LOG_FATAL", LOG_ERROR_OUTPUT_KEY)
        val OPENSWBF2_LOG_BUG_KEY = TextAttributesKey.createTextAttributesKey("OPENSWBF2_LOG_BUG", LOG_ERROR_OUTPUT_KEY)
        val OPENSWBF2_LOG_ERROR_KEY = TextAttributesKey.createTextAttributesKey("OPENSWBF2_LOG_ERROR", LOG_ERROR_OUTPUT_KEY)
        val OPENSWBF2_LOG_WARNING_KEY = TextAttributesKey.createTextAttributesKey("OPENSWBF2_LOG_WARNING", LOG_WARNING_OUTPUT_KEY)
        val OPENSWBF2_LOG_INFORM_KEY = TextAttributesKey.createTextAttributesKey("OPENSWBF2_LOG_INFORM", LOG_INFO_OUTPUT_KEY)
        val OPENSWBF2_LOG_VERBOSE_KEY = TextAttributesKey.createTextAttributesKey("OPENSWBF2_LOG_VERBOSE", LOG_DEBUG_OUTPUT_KEY)

        val OPENSWBF2_LOG_FATAL = OpenSWBF2LogConsoleViewContentType("OPENSWBF2_LOG_FATAL", OPENSWBF2_LOG_FATAL_KEY)
        val OPENSWBF2_LOG_BUG = OpenSWBF2LogConsoleViewContentType("OPENSWBF2_LOG_BUG", OPENSWBF2_LOG_BUG_KEY)
        val OPENSWBF2_LOG_ERROR = OpenSWBF2LogConsoleViewContentType("OPENSWBF2_LOG_ERROR", OPENSWBF2_LOG_ERROR_KEY)
        val OPENSWBF2_LOG_WARNING = OpenSWBF2LogConsoleViewContentType("OPENSWBF2_LOG_WARNING", OPENSWBF2_LOG_WARNING_KEY)
        val OPENSWBF2_LOG_INFORM = OpenSWBF2LogConsoleViewContentType("OPENSWBF2_LOG_INFORM", OPENSWBF2_LOG_INFORM_KEY)
        val OPENSWBF2_LOG_VERBOSE = OpenSWBF2LogConsoleViewContentType("OPENSWBF2_LOG_VERBOSE", OPENSWBF2_LOG_VERBOSE_KEY)
    }
}
