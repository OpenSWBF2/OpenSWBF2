package org.openswbf2.riderlink

import org.junit.Assert.assertEquals
import org.junit.Test

class PlayButtonsTest {
    private val shown = ButtonPresentation(true, true)

    private fun all(p: ButtonPresentation, state: PlayState) = listOf(
        PlayButtons.play(p, state),
        PlayButtons.resume(p, state),
        PlayButtons.pause(p, state),
        PlayButtons.singleStep(p, state),
        PlayButtons.stop(p, state),
    )

    @Test
    fun copiesUnrealLinkVisibleAndEnabledRulesPerState() {
        assertEquals(
            listOf(
                ButtonPresentation(true, true),
                ButtonPresentation(false, false),
                ButtonPresentation(true, false),
                ButtonPresentation(false, false),
                ButtonPresentation(true, false),
            ),
            all(shown, PlayState.Idle),
        )
        assertEquals(
            listOf(
                ButtonPresentation(false, false),
                ButtonPresentation(true, false),
                ButtonPresentation(true, true),
                ButtonPresentation(false, false),
                ButtonPresentation(true, true),
            ),
            all(shown, PlayState.Play),
        )
        assertEquals(
            listOf(
                ButtonPresentation(false, false),
                ButtonPresentation(true, true),
                ButtonPresentation(false, false),
                ButtonPresentation(true, true),
                ButtonPresentation(true, true),
            ),
            all(shown, PlayState.Pause),
        )
    }

    @Test
    fun baseNeedsTheSolutionAndTheSettingToShowAndAConnectedInitialisedGameToEnable() {
        assertEquals(ButtonPresentation(true, true), PlayButtons.base(true, true, true, true))
        assertEquals(ButtonPresentation(false, true), PlayButtons.base(false, true, true, true))
        assertEquals(ButtonPresentation(false, true), PlayButtons.base(true, false, true, true))
        assertEquals(ButtonPresentation(true, false), PlayButtons.base(true, true, false, true))
        assertEquals(ButtonPresentation(true, false), PlayButtons.base(true, true, true, false))
    }

    @Test
    fun disconnectedShowsPlayPauseStopDisabled() {
        val base = PlayButtons.base(true, true, false, false)
        assertEquals(
            listOf(
                ButtonPresentation(true, false),
                ButtonPresentation(false, false),
                ButtonPresentation(true, false),
                ButtonPresentation(false, false),
                ButtonPresentation(true, false),
            ),
            all(base, PlayState.Idle),
        )
    }
}
