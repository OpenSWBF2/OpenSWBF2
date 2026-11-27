package org.openswbf2.riderlink

import org.junit.Assert.assertEquals
import org.junit.Test

class LogLineTextTest {
    private val line = LogLine.split("[2026.11.25-20.27.11:326][679]LogRender: Inform: Display msaa list entries=4 windowed=1")
    private val lua = LogLine.split("ifs_saveop_DoOps    LoadFileList")
    private val defaults = LogLineLayout(showTimestamps = false, showVerbosity = true, alignMessages = true, categoryWidth = 25)

    @Test
    fun defaultLayoutIsVerbosityPaddedToThirteenThenCategoryPaddedToTwentySixLikeUnrealLink() {
        assertEquals(listOf("Inform", " ".repeat(7), "LogRender", " ".repeat(16), " "), LogLineText.info(line, defaults))
        assertEquals("Inform       LogRender                 Display msaa list entries=4 windowed=1",
            LogLineText.info(line, defaults).joinToString("") + line.message)
    }

    @Test
    fun timestampComesFirstPaddedToThirty() {
        val text = LogLineText.info(line, defaults.copy(showTimestamps = true))
        assertEquals(listOf("2026.11.25-20.27.11:326", " ".repeat(7), "Inform", " ".repeat(7), "LogRender", " ".repeat(16), " "), text)
    }

    @Test
    fun hidingVerbosityLeavesCategoryAndMessage() {
        assertEquals(listOf("LogRender", " ".repeat(16), " "), LogLineText.info(line, defaults.copy(showVerbosity = false)))
    }

    @Test
    fun unalignedUsesColonSeparators() {
        val unaligned = defaults.copy(alignMessages = false)
        assertEquals(listOf("Inform", ": ", "LogRender", ": "), LogLineText.info(line, unaligned))
        assertEquals(listOf("LogRender", ": "), LogLineText.info(line, unaligned.copy(showVerbosity = false)))
    }

    @Test
    fun aCategoryLongerThanTheWidthGetsOneSpace() {
        val long = LogLine.split("LogAVeryLongCategoryNameIndeed: Warning: x")
        assertEquals(listOf("Warning", " ".repeat(6), "LogAVeryLongCategoryNameIndeed", " "), LogLineText.info(long, defaults))
    }

    @Test
    fun aLineWithNoCategoryKeepsTheColumnsWhenAlignedAndHasNoPrefixOtherwise() {
        assertEquals(listOf("", " ".repeat(13), "", " ".repeat(25), " "), LogLineText.info(lua, defaults))
        assertEquals(emptyList<String>(), LogLineText.info(lua, defaults.copy(alignMessages = false)))
    }

    @Test
    fun errorGroupHoldsErrorAndBugOnly() {
        assertEquals(listOf(Verbosity.Error, Verbosity.Bug), Verbosity.entries.filter { it.isError() })
        assertEquals(false, (null as Verbosity?).isError())
    }
}
