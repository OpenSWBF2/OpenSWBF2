package org.openswbf2.riderlink

import com.intellij.openapi.util.IconLoader

class OpenSWBF2Icons {
    class ConnectionStatus {
        companion object {
            val PandemicLogo = IconLoader.getIcon("/icons/toolWindows/toolWindowPandemicLog.svg", OpenSWBF2Icons::class.java)
            val GameConnected = IconLoader.getIcon("/icons/run/pandemicConnected.svg", OpenSWBF2Icons::class.java)
            val GameDisconnected = IconLoader.getIcon("/icons/run/pandemicDisconnected.svg", OpenSWBF2Icons::class.java)
        }
    }

    class PIEControl {
        companion object {
            val Pause = IconLoader.getIcon("/icons/run/unrealStatusPause.svg", OpenSWBF2Icons::class.java)
            val Play = IconLoader.getIcon("/icons/run/unrealStatusPlay.svg", OpenSWBF2Icons::class.java)
            val Stop = IconLoader.getIcon("/icons/run/unrealStatusStop.svg", OpenSWBF2Icons::class.java)
            val FrameSkip = IconLoader.getIcon("/icons/run/unrealFrameSkip.svg", OpenSWBF2Icons::class.java)

        }
    }

    class ToolWindow {
        companion object {
            val OpenSWBF2ToolWindow = IconLoader.getIcon("/icons/toolWindows/toolWindowPandemicLog.svg", OpenSWBF2Icons::class.java)
        }
    }
}
