package org.openswbf2.riderlink

import com.intellij.openapi.components.service
import com.intellij.openapi.options.DslConfigurableBase
import com.intellij.openapi.options.SearchableConfigurable
import com.intellij.openapi.project.Project
import com.intellij.openapi.ui.DialogPanel
import com.intellij.openapi.util.NlsSafe
import com.intellij.ui.dsl.builder.RightGap
import com.intellij.ui.dsl.builder.bindIntText
import com.intellij.ui.dsl.builder.bindSelected
import com.intellij.ui.dsl.builder.bindText
import com.intellij.ui.dsl.builder.columns
import com.intellij.ui.dsl.builder.panel
import com.intellij.ui.dsl.builder.selected

internal class OpenSWBF2LogSettingsConfigurable(private val project: Project) : DslConfigurableBase(), SearchableConfigurable {
    companion object {
        @NlsSafe
        const val OPENSWBF2_LINK = "OpenSWBF2 Link"
        const val ID = "OpenSWBF2LogSettings"
    }

    override fun getId(): String = ID

    override fun getDisplayName(): String = OPENSWBF2_LINK

    override fun createPanel(): DialogPanel = panel {
        val commonSettings = project.service<LinkSettings>()
        val logSettings = project.service<OpenSWBF2LogPanelSettings>()

        group(OpenSWBF2LinkBundle.message("configurable.OpenSWBF2Link.toolbar.settings.label")) {
            row(OpenSWBF2LinkBundle.message("configurable.OpenSWBF2Link.mission.label")) {
                textField()
                        .columns(24)
                        .bindText(commonSettings::mission)
            }
        }

        group(OpenSWBF2LinkBundle.message("configurable.OpenSWBF2Link.log.settings.label")) {
            row {
                checkBox(OpenSWBF2LinkBundle.message("configurable.OpenSWBF2Link.clearLogOnStart.label"))
                        .bindSelected(logSettings::clearOnStart)
            }
            row {
                checkBox(OpenSWBF2LinkBundle.message("configurable.OpenSWBF2Link.focusLogOnStart.label"))
                        .bindSelected(logSettings::focusOnStart)
            }
            separator()
            row {
                checkBox(OpenSWBF2LinkBundle.message("configurable.OpenSWBF2Link.showTimestampsCheckbox.label"))
                        .bindSelected(logSettings::showTimestamps)
            }
            row {
                checkBox(OpenSWBF2LinkBundle.message("configurable.OpenSWBF2Link.showVerbosityCheckbox.label"))
                        .bindSelected(logSettings::showVerbosity)
            }
            row {
                val alignMessages =
                        checkBox(OpenSWBF2LinkBundle.message("configurable.OpenSWBF2Link.alignMessagesCheckbox.label"))
                                .bindSelected(logSettings::alignMessages)
                                .gap(RightGap.SMALL)
                intTextField(1..100, 1)
                        .columns(2)
                        .bindIntText(logSettings::categoryWidth)
                        .enabledIf(alignMessages.selected)
                        .gap(RightGap.SMALL)
                label(OpenSWBF2LinkBundle.message("configurable.OpenSWBF2Link.alignMessagesCheckbox.label.ending"))
            }
        }
    }
}
