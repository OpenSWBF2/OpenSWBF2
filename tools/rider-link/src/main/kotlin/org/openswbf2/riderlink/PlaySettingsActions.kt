package org.openswbf2.riderlink

import com.intellij.execution.process.ProcessInfo
import com.intellij.ide.actions.ShowSettingsUtilImpl
import com.intellij.openapi.actionSystem.ActionGroup
import com.intellij.openapi.actionSystem.ActionUpdateThread
import com.intellij.openapi.actionSystem.AnAction
import com.intellij.openapi.actionSystem.AnActionEvent
import com.intellij.openapi.actionSystem.DefaultActionGroup
import com.intellij.openapi.project.DumbAware
import com.intellij.openapi.project.DumbAwareAction
import com.intellij.openapi.project.DumbAwareToggleAction
import com.intellij.openapi.project.Project
import com.intellij.openapi.util.NlsActions
import com.intellij.xdebugger.attach.LocalAttachHost
import com.jetbrains.rider.cpp.debugger.RiderCppLLDBDriverConfiguration
import com.jetbrains.rider.cpp.debugger.RiderCppLocalAttachDebugger

class PlaySettings : DefaultActionGroup(), DumbAware {
    private val connectedIcon = OpenSWBF2Icons.ConnectionStatus.GameConnected
    private val disconnectedIcon = OpenSWBF2Icons.ConnectionStatus.GameDisconnected

    override fun getActionUpdateThread(): ActionUpdateThread = ActionUpdateThread.BGT

    override fun update(e: AnActionEvent) {
        super.update(e)
        LinkServiceOperations.updatePresentationBasedOnGameAvailability(e, connectedIcon, disconnectedIcon)
    }
}

class ProtocolStatus : DumbAwareAction() {
    override fun getActionUpdateThread(): ActionUpdateThread = ActionUpdateThread.BGT

    override fun actionPerformed(e: AnActionEvent) = Unit

    override fun update(e: AnActionEvent) {
        super.update(e)

        e.presentation.isEnabled = false

        val service = e.getLinkService() ?: return
        e.presentation.isVisible = true
        e.presentation.text = getActionText(service)
    }

    @NlsActions.ActionText
    fun getActionText(service: LinkService): String {
        return if (service.connected) {
            val info = service.connectionInfo
            if (info != null) {
                OpenSWBF2LinkBundle.message("action.OpenSWBF2Link.ProtocolStatus.connected.ex.text", "${info.platform} ${info.config}",
                                            info.executableName, info.pid.toString())
            } else {
                OpenSWBF2LinkBundle.message("action.OpenSWBF2Link.ProtocolStatus.connected.text")
            }
        } else {
            OpenSWBF2LinkBundle.message("action.OpenSWBF2Link.ProtocolStatus.disconnected.text") + service.disconnectReason()
        }
    }
}

class AttachToConnectedGame : DumbAwareAction() {
    override fun getActionUpdateThread(): ActionUpdateThread = ActionUpdateThread.EDT

    override fun update(e: AnActionEvent) {
        super.update(e)

        val service = e.getLinkService()
        if (service == null) {
            e.presentation.isEnabledAndVisible = false
            return
        }

        e.presentation.isVisible = true

        if (!service.connected) {
            e.presentation.isEnabled = false
            return
        }

        val connectionInfo = service.connectionInfo
        if (connectionInfo == null) {
            e.presentation.isEnabled = false
            return
        }

        e.presentation.isEnabled = true
    }

    override fun actionPerformed(e: AnActionEvent) {
        val service = e.getLinkService() ?: return
        val connectionInfo = service.connectionInfo ?: return

        val processInfo = ProcessInfo(connectionInfo.pid.toInt(), "", connectionInfo.executableName, "")
        attachToGameProcess(service.project, processInfo)
    }

    private fun attachToGameProcess(project: Project, processInfo: ProcessInfo) {
        val attachDebugger = RiderCppLocalAttachDebugger(RiderCppLLDBDriverConfiguration())
        attachDebugger.attachDebugSession(project, LocalAttachHost.INSTANCE, processInfo)
    }

}

class OpenOpenSWBF2LinkSettings : DumbAwareAction() {
    override fun getActionUpdateThread(): ActionUpdateThread = ActionUpdateThread.BGT

    override fun update(e: AnActionEvent) {
        val service = e.getLinkService()
        e.presentation.isEnabledAndVisible = service != null
    }

    override fun actionPerformed(e: AnActionEvent) {
        val project = e.project ?: return
        ShowSettingsUtilImpl.showSettingsDialog(project, OpenSWBF2LogSettingsConfigurable.ID, null)
    }
}

class GamePicker : ActionGroup(), DumbAware {
    override fun getActionUpdateThread(): ActionUpdateThread = ActionUpdateThread.BGT

    override fun update(e: AnActionEvent) {
        val service = e.getLinkService()
        e.presentation.isEnabledAndVisible = service != null && service.games.size > 1
    }

    override fun getChildren(e: AnActionEvent?): Array<AnAction> {
        val service = e?.getLinkService() ?: return emptyArray()
        return service.games.map { game ->
            object : DumbAwareToggleAction(OpenSWBF2LinkBundle.message("action.OpenSWBF2Link.ChooseGame.text", game.pid.toString(), game.port.toString())) {
                override fun getActionUpdateThread(): ActionUpdateThread = ActionUpdateThread.BGT

                override fun isSelected(e: AnActionEvent): Boolean = service.current?.pid == game.pid

                override fun setSelected(e: AnActionEvent, state: Boolean) {
                    if (state) {
                        service.choose(game.pid)
                    }
                }
            }
        }.toTypedArray()
    }
}

class HidePlayButtonsAction : DumbAwareToggleAction() {
    override fun getActionUpdateThread(): ActionUpdateThread = ActionUpdateThread.BGT

    override fun update(e: AnActionEvent) {
        super.update(e)

        val service = e.getLinkService()
        e.presentation.isEnabledAndVisible = service != null
    }

    override fun isSelected(e: AnActionEvent): Boolean {
        val project = e.project ?: return false
        val settings = OpenSWBF2LogPanelSettings.getInstance(project)
        return settings.showPlayButtons
    }

    override fun setSelected(e: AnActionEvent, selected: Boolean) {
        val project = e.project ?: return
        val settings = OpenSWBF2LogPanelSettings.getInstance(project)
        settings.showPlayButtons = selected
    }
}

class LinkServiceOperations {
    companion object {
        fun updatePresentationBasedOnGameAvailability(e: AnActionEvent, connectedIcon: javax.swing.Icon, disconnectedIcon: javax.swing.Icon) {
            val service = e.getLinkService()
            if (service == null || !service.isOpenSWBF2Solution) {
                e.presentation.isEnabledAndVisible = false
                return
            }

            e.presentation.isEnabledAndVisible = true
            e.presentation.icon = if (service.connected) connectedIcon else disconnectedIcon
        }
    }
}
