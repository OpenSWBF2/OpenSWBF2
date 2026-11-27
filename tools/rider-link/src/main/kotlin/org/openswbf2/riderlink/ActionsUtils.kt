package org.openswbf2.riderlink

import com.intellij.ide.ActivityTracker
import com.intellij.openapi.actionSystem.AnActionEvent

fun AnActionEvent.getLinkService(): LinkService? {
    val project = project ?: return null
    return LinkService.of(project)
}

fun forceTriggerUIUpdate() {
    ActivityTracker.getInstance().inc()
}
