package org.openswbf2.riderlink

data class ButtonPresentation(val visible: Boolean, val enabled: Boolean)

object PlayButtons {
    fun base(isOpenSWBF2Solution: Boolean, showPlayButtons: Boolean, connected: Boolean, gameControlInitialized: Boolean) =
        ButtonPresentation(isOpenSWBF2Solution && showPlayButtons, connected && gameControlInitialized)

    fun play(p: ButtonPresentation, state: PlayState) =
        ButtonPresentation(p.visible && state == PlayState.Idle, p.enabled && state == PlayState.Idle)

    fun resume(p: ButtonPresentation, state: PlayState) =
        ButtonPresentation(p.visible && state != PlayState.Idle, p.enabled && state == PlayState.Pause)

    fun stop(p: ButtonPresentation, state: PlayState) =
        ButtonPresentation(p.visible, p.enabled && state != PlayState.Idle)

    fun pause(p: ButtonPresentation, state: PlayState) =
        ButtonPresentation(p.visible && state != PlayState.Pause, p.enabled && state == PlayState.Play)

    fun singleStep(p: ButtonPresentation, state: PlayState) =
        ButtonPresentation(p.visible && state == PlayState.Pause, p.enabled && state == PlayState.Pause)
}
