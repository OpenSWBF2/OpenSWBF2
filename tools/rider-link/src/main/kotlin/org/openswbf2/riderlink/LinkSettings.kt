package org.openswbf2.riderlink

import com.intellij.openapi.components.BaseState
import com.intellij.openapi.components.Service
import com.intellij.openapi.components.SimplePersistentStateComponent
import com.intellij.openapi.components.State
import com.intellij.openapi.components.Storage
import com.intellij.openapi.components.StoragePathMacros
import com.intellij.openapi.components.service
import com.intellij.openapi.project.Project

class LinkSettingsState : BaseState() {
    var mission by string("")
}

@Service(Service.Level.PROJECT)
@State(name = "OpenSWBF2Link", storages = [Storage(StoragePathMacros.WORKSPACE_FILE)])
class LinkSettings : SimplePersistentStateComponent<LinkSettingsState>(LinkSettingsState()) {
    var mission: String
        get() = state.mission.orEmpty()
        set(value) {
            state.mission = value.trim()
        }

    companion object {
        fun of(project: Project): LinkSettings = project.service()
    }
}
