package org.openswbf2.riderlink

import com.intellij.icons.AllIcons
import com.intellij.openapi.actionSystem.DefaultActionGroup
import com.intellij.openapi.project.DumbAware


class OpenSWBF2LogSettingsActionGroup(private val settings: OpenSWBF2LogPanelSettings) : DefaultActionGroup(), DumbAware {
    init {
        isPopup = true
        templatePresentation.icon = AllIcons.General.GearPlain

        add(FilterCheckboxAction(OpenSWBF2LinkBundle.message("toolWindow.OpenSWBF2Log.settings.clearLogOnStart.label"), settings::clearOnStart))
        add(FilterCheckboxAction(OpenSWBF2LinkBundle.message("toolWindow.OpenSWBF2Log.settings.focusLogOnStart.label"), settings::focusOnStart))
        addSeparator()
        add(FilterCheckboxAction(OpenSWBF2LinkBundle.message("toolWindow.OpenSWBF2Log.settings.showTimestampsCheckbox.label"), settings::showTimestamps))
        add(FilterCheckboxAction(OpenSWBF2LinkBundle.message("toolWindow.OpenSWBF2Log.settings.showVerbosityCheckbox.label"), settings::showVerbosity))
        add(FilterCheckboxAction(OpenSWBF2LinkBundle.message("toolWindow.OpenSWBF2Log.settings.alignMessagesCheckbox.label"), settings::alignMessages))
    }
}
