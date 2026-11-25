package org.openswbf2.riderlink

import com.intellij.openapi.rd.createNestedDisposable
import com.jetbrains.rd.util.lifetime.Lifetime

class OpenSWBF2LogFilter(lifetime: Lifetime, private val settings: OpenSWBF2LogPanelSettings) {
    private val categories: HashSet<String> = hashSetOf()
    private val selectedCategories: HashSet<String> = hashSetOf()

    private val filterChangedListeners: ArrayList<() -> Unit> = arrayListOf()
    private val onCategoryAddedListeners: ArrayList<(String) -> Unit> = arrayListOf()

    init {
        val disposable = lifetime.createNestedDisposable()
        settings.addSettingsChangedListener({ onFilterChanged() }, disposable)
    }

    fun addFilterChangedListener(listener: () -> Unit) {
        filterChangedListeners.add(listener)
    }

    private fun onFilterChanged() {
        filterChangedListeners.forEach { it.invoke() }
    }

    fun isMessageAllowed(message: LogLine): Boolean {
        val verbosity = message.verbosity
        if (verbosity.isError() && !settings.showErrors) {
            return false
        }

        if (verbosity == Verbosity.Warning && !settings.showWarnings) {
            return false
        }

        if (!verbosity.isError() && verbosity != Verbosity.Warning && !settings.showMessages) {
            return false
        }

        val category = message.filterCategory
        if (category !in selectedCategories) {
            return false
        }

        return true
    }

    fun addCategory(category: String) {
        if (!categories.add(category)) {
            return
        }

        val isSelected = settings.showAllCategories
        if (isSelected) {
            selectedCategories.add(category)
        }

        fireOnCategoryAdded(category)
    }

    fun addOnCategoryAddedListener(listener: (String) -> Unit) {
        onCategoryAddedListeners.add(listener)
    }

    private fun fireOnCategoryAdded(category: String) {
        onCategoryAddedListeners.forEach { it.invoke(category) }
    }

    fun isCategorySelected(category: String): Boolean {
        return category in selectedCategories
    }

    fun toggleCategory(category: String, state: Boolean) {
        if (state) {
            selectedCategories.add(category)
        } else {
            selectedCategories.remove(category)
        }

        onFilterChanged()
    }

    fun toggleAllCategories(state: Boolean) {
        selectedCategories.clear()
        if (state) {
            selectedCategories.addAll(categories)
        }
    }
}
