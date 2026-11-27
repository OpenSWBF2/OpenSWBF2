package org.openswbf2.riderlink

import com.intellij.execution.filters.TextConsoleBuilderFactory
import com.intellij.execution.impl.ConsoleViewImpl
import com.intellij.execution.ui.ConsoleViewContentType
import com.intellij.openapi.actionSystem.ActionManager
import com.intellij.openapi.actionSystem.ActionPlaces
import com.intellij.openapi.actionSystem.DefaultActionGroup
import com.intellij.openapi.application.invokeLater
import com.intellij.openapi.project.Project
import com.intellij.openapi.ui.SimpleToolWindowPanel
import com.intellij.ui.components.panels.HorizontalLayout
import com.jetbrains.rd.util.eol
import com.jetbrains.rd.util.lifetime.Lifetime
import java.awt.BorderLayout
import java.util.ArrayDeque
import javax.swing.JPanel

internal class OpenSWBF2LogPanel(val tabModel: String, lifetime: Lifetime, val project: Project) : SimpleToolWindowPanel(false), LinkView {
    companion object {
        private const val MAX_STORED_LOG_DATA_ITEMS = 32 * 1024

        private fun createConsole(project: Project, lifetime: Lifetime): ConsoleViewImpl {
            val consoleView = TextConsoleBuilderFactory.getInstance()
                    .createBuilder(project)
                    .console as ConsoleViewImpl

            lifetime.bracketIfAlive({
                consoleView.component
            }, {
                consoleView.dispose()
            })

            return consoleView

        }
    }

    private val settings: OpenSWBF2LogPanelSettings = OpenSWBF2LogPanelSettings.getInstance(project)

    private val logData: ArrayDeque<LogLine> = ArrayDeque()
    private val consoleView: ConsoleViewImpl = createConsole(project, lifetime)

    val console: ConsoleViewImpl get() = consoleView

    private val logFilter: OpenSWBF2LogFilter = OpenSWBF2LogFilter(lifetime, settings)
    private val verbosityFilterActionGroup: OpenSWBF2LogVerbosityFilterComboBox = OpenSWBF2LogVerbosityFilterComboBox(settings)
    private val categoryFilterActionGroup: OpenSWBF2LogCategoryFilterComboBox = OpenSWBF2LogCategoryFilterComboBox(settings, logFilter)
    private val settingsActionGroup = OpenSWBF2LogSettingsActionGroup(settings)

    init {
        setContent(consoleView)
        val actionGroup = DefaultActionGroup().apply {
            addAll(consoleView.createConsoleActions().toList())
        }

        val toolbar = ActionManager.getInstance().createActionToolbar(ActionPlaces.TOOLBAR, actionGroup, myVertical)

        val topGroup = DefaultActionGroup().apply {
            add(verbosityFilterActionGroup)
            add(categoryFilterActionGroup)
            add(settingsActionGroup)
        }
        val topToolbar = ActionManager.getInstance().createActionToolbar(ActionPlaces.TOOLBAR, topGroup, true)

        val topPanel = JPanel(HorizontalLayout(0))
        topToolbar.targetComponent = topPanel
        topPanel.add(topToolbar.component)

        consoleView.scrollTo(0)

        consoleView.add(topPanel, BorderLayout.NORTH)
        toolbar.targetComponent = this
        setToolbar(toolbar.component)

        logFilter.addFilterChangedListener { filter(); }

        val service = LinkService.of(project)
        lifetime.bracketIfAlive({ service.addView(this) }, { service.removeView(this) })
        if (service.connected) {
            connected()
        }
    }

    override fun connected() {
        if (settings.clearOnStart) {
            clear()
        }
    }

    override fun lineAdded(line: LogLine) {
        print(line)
    }

    fun clear() {
        logData.clear()
        consoleView.clear()
    }

    private fun print(logLine: LogLine) {
        addLogDataItem(logLine)
        printImpl(logLine)
    }

    private fun filter() {
        val currentScroll = consoleView.editor!!.scrollingModel.verticalScrollOffset
        consoleView.clear()
        invokeLater {
            for (logEvent in logData) {
                printImpl(logEvent)
            }
            consoleView.editor!!.scrollingModel.scrollVertically(currentScroll)
        }
    }

    private fun printInfo(s: LogLine, style: ConsoleViewContentType) {
        val layout = LogLineLayout(settings.showTimestamps, settings.showVerbosity, settings.alignMessages, settings.categoryWidth)
        for (segment in LogLineText.info(s, layout)) {
            consoleView.print(segment, style)
        }
    }

    private fun getMessageStyle(type: Verbosity?): ConsoleViewContentType {
        return when (type) {
            Verbosity.Fatal -> OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_FATAL
            Verbosity.Bug -> OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_BUG
            Verbosity.Error -> OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_ERROR
            Verbosity.Warning -> OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_WARNING
            Verbosity.Inform -> OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_INFORM
            Verbosity.Verbose -> OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_VERBOSE
            else -> OpenSWBF2LogConsoleViewContentType.OPENSWBF2_LOG_INFORM
        }
    }

    private fun printImpl(logLine: LogLine): Boolean {
        logFilter.addCategory(logLine.filterCategory)
        if (!logFilter.isMessageAllowed(logLine)) {
            return false
        }

        val style = getMessageStyle(logLine.verbosity)

        printInfo(logLine, style)

        consoleView.print(logLine.message, style)
        consoleView.print(eol, style)
        return true
    }

    private fun addLogDataItem(item: LogLine) {
        while (logData.size >= MAX_STORED_LOG_DATA_ITEMS)
            logData.removeFirst()
        logData.add(item)
    }
}
