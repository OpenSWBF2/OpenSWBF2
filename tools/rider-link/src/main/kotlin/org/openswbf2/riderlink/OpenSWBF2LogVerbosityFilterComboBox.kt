package org.openswbf2.riderlink

import com.intellij.openapi.actionSystem.ActionUpdateThread
import com.intellij.openapi.actionSystem.DefaultActionGroup
import com.intellij.openapi.actionSystem.ex.ComboBoxAction
import com.intellij.openapi.project.DumbAware
import javax.swing.JComponent

class OpenSWBF2LogVerbosityFilterComboBox(settings: OpenSWBF2LogPanelSettings) : ComboBoxAction(), DumbAware {
    override fun getActionUpdateThread() = ActionUpdateThread.BGT
    init {
        val presentation = this.templatePresentation
        presentation.text = OpenSWBF2LinkBundle.message("toolWindow.OpenSWBF2Log.settings.verbositySelection.label")
    }

    private val messagesCheckBox: FilterCheckboxAction =
            FilterCheckboxAction(OpenSWBF2LinkBundle.message("toolWindow.OpenSWBF2Log.settings.verbosity.Messages.text"), settings::showMessages)
    private val warningsCheckBox: FilterCheckboxAction =
            FilterCheckboxAction(OpenSWBF2LinkBundle.message("toolWindow.OpenSWBF2Log.settings.verbosity.Warnings.text"), settings::showWarnings)
    private val errorsCheckBox: FilterCheckboxAction =
            FilterCheckboxAction(OpenSWBF2LinkBundle.message("toolWindow.OpenSWBF2Log.settings.verbosity.Errors.text"), settings::showErrors)
    private val popupGroup: DefaultActionGroup = VerbosityActionGroup()

    override fun createPopupActionGroup(button: JComponent?): DefaultActionGroup {
        return popupGroup
    }

    inner class VerbosityActionGroup : DefaultActionGroup() {
        init {
            add(messagesCheckBox)
            add(warningsCheckBox)
            add(errorsCheckBox)
        }
    }

}
