package org.openswbf2.riderlink

import com.intellij.ide.impl.ContentManagerWatcher
import com.intellij.openapi.components.Service
import com.intellij.openapi.components.service
import com.intellij.openapi.project.Project
import com.intellij.openapi.rd.util.lifetime
import com.intellij.openapi.util.Disposer
import com.intellij.openapi.wm.RegisterToolWindowTask
import com.intellij.openapi.wm.ToolWindow
import com.intellij.openapi.wm.ToolWindowAnchor
import com.intellij.openapi.wm.ToolWindowManager
import com.intellij.ui.content.ContentManagerEvent
import com.intellij.ui.content.ContentManagerListener
import com.jetbrains.rider.ui.toolWindow.RiderOnDemandToolWindowFactory

@Suppress("UnstableApiUsage")
@Service(Service.Level.PROJECT)
class OpenSWBF2ToolWindowFactory(val project: Project)
    : RiderOnDemandToolWindowFactory<String>(project, TOOLWINDOW_ID, { it }, ::OpenSWBF2LogPanel, { it }) {

    companion object {
        const val TOOLWINDOW_ID = "OpenSWBF2 Link"
        val TITLE_ID =  OpenSWBF2LinkBundle.message("OpenSWBF2Link.OpenSWBF2GameLog.text.title")

        fun getInstance(project: Project): OpenSWBF2ToolWindowFactory = project.service()
    }

    override fun registerToolWindow(toolWindowManager: ToolWindowManager, project: Project): ToolWindow {
        val toolWindow = toolWindowManager.registerToolWindow(
                RegisterToolWindowTask(
                    id = TOOLWINDOW_ID, anchor = ToolWindowAnchor.BOTTOM,
                    icon = OpenSWBF2Icons.ToolWindow.OpenSWBF2ToolWindow,
                    canCloseContent = false, canWorkInDumbMode = true, sideTool = false
                )
        )
        toolWindow.title = OpenSWBF2LinkBundle.message("toolWindow.OpenSWBF2Log.title")

        val contentManager = toolWindow.contentManager
        contentManager.addContentManagerListener(object : ContentManagerListener {
            override fun contentAdded(event: ContentManagerEvent) {
                val content = event.content.component as OpenSWBF2LogPanel
                Disposer.register(toolWindow.disposable, content.console)
            }
        })

        ContentManagerWatcher.watchContentManager(toolWindow, contentManager)

        return toolWindow
    }

    fun showTabForNewSession() {
        val settings = project.service<OpenSWBF2LogPanelSettings>()
        if (settings.focusOnStart) {
            showTab(TITLE_ID, project.lifetime)
        } else {
            getOrCreateTab(TITLE_ID, project.lifetime)
        }
    }
}
