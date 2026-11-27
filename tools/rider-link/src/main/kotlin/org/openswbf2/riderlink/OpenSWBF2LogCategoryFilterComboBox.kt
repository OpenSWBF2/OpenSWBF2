package org.openswbf2.riderlink

import com.intellij.openapi.actionSystem.ActionUpdateThread
import com.intellij.openapi.actionSystem.DefaultActionGroup
import com.intellij.openapi.actionSystem.ex.ComboBoxAction
import com.intellij.openapi.project.DumbAware
import javax.swing.JComponent

class OpenSWBF2LogCategoryFilterComboBox(private val settings: OpenSWBF2LogPanelSettings,
                                         private val logFilter: OpenSWBF2LogFilter) : ComboBoxAction(), DumbAware {
    override fun getActionUpdateThread() = ActionUpdateThread.BGT

    init {
        val presentation = this.templatePresentation
        presentation.text = OpenSWBF2LinkBundle.message("toolWindow.OpenSWBF2Log.settings.categoriesSelection.label")

        logFilter.addOnCategoryAddedListener(this::onCategoryAdded)
    }

    private var showAllCategories: Boolean
        get() = settings.showAllCategories
        set(value) {
            if (settings.showAllCategories != value) {
                logFilter.toggleAllCategories(value)
                settings.showAllCategories = value
            }
        }


    private val allSelectedCheckbox: FilterCheckboxAction =
            FilterCheckboxAction(OpenSWBF2LinkBundle.message("toolWindow.OpenSWBF2Log.settings.categories.ShowAll.text"), ::showAllCategories)

    private val categoriesCheckboxes: ArrayList<FilterCheckboxAction> = arrayListOf()

    override fun createPopupActionGroup(parent: JComponent?): DefaultActionGroup {
        val group = DefaultActionGroup()
        group.add(allSelectedCheckbox)
        group.addSeparator()
        group.addAll(categoriesCheckboxes)
        return group
    }

    private fun isCategorySelected(category: String): Boolean = logFilter.isCategorySelected(category)

    private fun selectCategory(category: String, state: Boolean) = logFilter.toggleCategory(category, state)

    private fun onCategoryAdded(category: String) {
        var idx = categoriesCheckboxes.binarySearchBy(category) { it.templatePresentation.text }
        if (idx >= 0) return

        val checkbox = FilterCheckboxAction(category,
                { isCategorySelected(category) },
                { selectCategory(category, it) }
        )

        idx = -idx - 1
        categoriesCheckboxes.add(idx, checkbox)
    }
}
