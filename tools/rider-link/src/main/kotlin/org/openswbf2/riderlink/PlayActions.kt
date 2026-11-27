package org.openswbf2.riderlink

import com.intellij.openapi.actionSystem.ActionUpdateThread
import com.intellij.openapi.actionSystem.AnActionEvent
import com.intellij.openapi.actionSystem.Presentation
import com.intellij.openapi.project.DumbAwareAction
import com.intellij.ui.icons.CachedImageIcon
import com.intellij.ui.icons.loadIconCustomVersionOrScale
import com.intellij.util.ui.JBUI
import javax.swing.Icon

private fun Presentation.current() = ButtonPresentation(isVisible, isEnabled)

private fun Presentation.apply(p: ButtonPresentation) {
    isVisible = p.visible
    isEnabled = p.enabled
}

abstract class PlayStateAction() : DumbAwareAction() {
    override fun getActionUpdateThread(): ActionUpdateThread = ActionUpdateThread.BGT

    override fun update(e: AnActionEvent) {
        val project = e.project
        if (project == null) {
            e.presentation.isEnabledAndVisible = false
            return
        }
        val service = LinkService.of(project)
        val settings = OpenSWBF2LogPanelSettings.getInstance(project)

        e.presentation.apply(PlayButtons.base(service.isOpenSWBF2Solution, settings.showPlayButtons,
            service.connected, service.isGameControlInitialized))
    }
}

class PlayInGameAction : PlayStateAction() {
    override fun update(e: AnActionEvent) {
        super.update(e)
        val service = e.getLinkService() ?: return
        e.presentation.icon = getToolbarIconHack(OpenSWBF2Icons.PIEControl.Play)
        e.presentation.apply(PlayButtons.play(e.presentation.current(), service.playState))
        val mission = LinkSettings.of(service.project).mission
        if (!Protocol.isMissionName(mission)) {
            e.presentation.isEnabled = false
            e.presentation.description = if (mission.isEmpty()) {
                "Set the mission name under Settings, Tools, OpenSWBF2 Link"
            } else {
                "The mission name under Settings, Tools, OpenSWBF2 Link must be 1 to 63 of A-Z a-z 0-9 _"
            }
        } else {
            e.presentation.description = templatePresentation.description
        }
    }

    override fun actionPerformed(e: AnActionEvent) {
        val service = e.getLinkService() ?: return
        val line = Protocol.play(LinkSettings.of(service.project).mission) ?: return
        service.send(line)
    }
}

class ResumeInGameAction : PlayStateAction() {
    override fun update(e: AnActionEvent) {
        super.update(e)
        val service = e.getLinkService() ?: return
        e.presentation.icon = getToolbarIconHack(OpenSWBF2Icons.PIEControl.Play)
        e.presentation.apply(PlayButtons.resume(e.presentation.current(), service.playState))
    }

    override fun actionPerformed(e: AnActionEvent) {
        val service = e.getLinkService() ?: return
        service.send(Protocol.RESUME)
    }
}

class StopInGameAction : PlayStateAction() {
    override fun update(e: AnActionEvent) {
        super.update(e)
        val service = e.getLinkService() ?: return
        e.presentation.icon = getToolbarIconHack(OpenSWBF2Icons.PIEControl.Stop)
        e.presentation.apply(PlayButtons.stop(e.presentation.current(), service.playState))
    }

    override fun actionPerformed(e: AnActionEvent) {
        val service = e.getLinkService() ?: return
        service.send(Protocol.STOP)
    }
}

class PauseInGameAction : PlayStateAction() {
    override fun update(e: AnActionEvent) {
        super.update(e)
        val service = e.getLinkService() ?: return
        e.presentation.icon = getToolbarIconHack(OpenSWBF2Icons.PIEControl.Pause)
        e.presentation.apply(PlayButtons.pause(e.presentation.current(), service.playState))
    }

    override fun actionPerformed(e: AnActionEvent) {
        val service = e.getLinkService() ?: return
        service.send(Protocol.PAUSE)
    }
}

class SingleStepInGameAction : PlayStateAction() {
    override fun update(e: AnActionEvent) {
        super.update(e)
        val service = e.getLinkService() ?: return
        e.presentation.icon = getToolbarIconHack(OpenSWBF2Icons.PIEControl.FrameSkip)
        e.presentation.apply(PlayButtons.singleStep(e.presentation.current(), service.playState))
    }

    override fun actionPerformed(e: AnActionEvent) {
        val service = e.getLinkService() ?: return
        service.send(Protocol.STEP)
    }
}

private fun getToolbarIconHack(icon: Icon): Icon {
  if (icon is CachedImageIcon) {
    return loadIconCustomVersionOrScale(icon, size = JBUI.CurrentTheme.Toolbar.experimentalToolbarButtonIconSize())
  }
  return icon
}
