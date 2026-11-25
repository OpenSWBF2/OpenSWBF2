package org.openswbf2.riderlink

import com.intellij.execution.impl.ConsoleViewUtil
import com.intellij.openapi.editor.colors.EditorColorsScheme
import com.intellij.openapi.editor.colors.TextAttributesKey
import com.intellij.openapi.fileTypes.PlainSyntaxHighlighter
import com.intellij.openapi.fileTypes.SyntaxHighlighter
import com.intellij.openapi.options.colors.AttributesDescriptor
import com.intellij.openapi.options.colors.ColorDescriptor
import com.intellij.openapi.options.colors.ColorSettingsPage
import com.intellij.openapi.util.NlsSafe
import javax.swing.Icon


@Suppress("UnstableApiUsage")
class OpenSWBF2LogColorSettingsPage : ColorSettingsPage {
    companion object {
        private const val FATAL: @NlsSafe String = "Fatal"
        private const val BUG: @NlsSafe String = "Bug"
        private const val ERROR: @NlsSafe String = "Error"
        private const val WARNING: @NlsSafe String = "Warning"
        private const val INFORM: @NlsSafe String = "Inform"
        private const val VERBOSE: @NlsSafe String = "Verbose"

        private val ATTR_DESCRIPTORS: Array<AttributesDescriptor> = arrayOf(
                AttributesDescriptor(FATAL, OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_FATAL_KEY),
                AttributesDescriptor(BUG, OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_BUG_KEY),
                AttributesDescriptor(ERROR, OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_ERROR_KEY),
                AttributesDescriptor(WARNING, OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_WARNING_KEY),
                AttributesDescriptor(INFORM, OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_INFORM_KEY),
                AttributesDescriptor(VERBOSE, OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_VERBOSE_KEY),
        )

        private val ADDITIONAL_HIGHLIGHT_DESCRIPTORS: Map<String, TextAttributesKey> = mapOf(
                "fatal" to OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_FATAL_KEY,
                "bug" to OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_BUG_KEY,
                "error" to OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_ERROR_KEY,
                "warning" to OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_WARNING_KEY,
                "inform" to OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_INFORM_KEY,
                "verbose" to OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_VERBOSE_KEY,
        )
    }

    override fun getDisplayName(): String {
        return OpenSWBF2LinkBundle.message("color.settings.openswbf2.console.name")
    }

    override fun getAttributeDescriptors(): Array<AttributesDescriptor> {
        return ATTR_DESCRIPTORS
    }

    override fun getColorDescriptors(): Array<ColorDescriptor> {
        return emptyArray()
    }

    override fun getHighlighter(): SyntaxHighlighter {
        return PlainSyntaxHighlighter()
    }

    override fun getAdditionalHighlightingTagToDescriptorMap(): Map<String, TextAttributesKey> {
        return ADDITIONAL_HIGHLIGHT_DESCRIPTORS
    }

    override fun getDemoText(): String {
        return """

            <fatal>LOG(LogCore, Fatal, "Message");</fatal>
            <bug>LOG(LogCore, Bug, "Message");</bug>
            <error>LOG(LogCore, Error, "Message");</error>
            <warning>LOG(LogCore, Warning, "Message");</warning>
            <inform>LOG(LogCore, Inform, "Message");</inform>
            <verbose>LOG(LogCore, Verbose, "Message");</verbose>
        """.trimIndent()
    }

    override fun getIcon(): Icon = OpenSWBF2Icons.ConnectionStatus.PandemicLogo

    override fun customizeColorScheme(scheme: EditorColorsScheme): EditorColorsScheme {
        return ConsoleViewUtil.updateConsoleColorScheme(scheme)
    }
}
