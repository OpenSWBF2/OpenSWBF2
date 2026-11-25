package org.openswbf2.riderlink

import com.intellij.openapi.application.ApplicationManager
import com.intellij.openapi.components.Service
import com.intellij.openapi.components.service
import com.intellij.openapi.project.Project
import com.intellij.openapi.startup.ProjectActivity
import com.jetbrains.rider.projectView.solutionFile
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import java.io.IOException
import java.nio.file.Path
import java.nio.file.Paths
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.CopyOnWriteArrayList
import java.util.concurrent.atomic.AtomicBoolean

sealed class LinkStatus {
    object Waiting : LinkStatus()
    object Choose : LinkStatus()
    data class Connecting(val entry: PortEntry) : LinkStatus()
    data class Connected(val hello: Message.Hello) : LinkStatus()
    data class Disconnected(val end: SessionEnd) : LinkStatus()
}

data class ConnectionInfo(val pid: Long, val platform: String, val config: String, val executableName: String)

interface LinkView {
    fun connected()
    fun lineAdded(line: LogLine)
}

class LinkStartup : ProjectActivity {
    override suspend fun execute(project: Project) {
        val service = LinkService.of(project)
        if (service.isOpenSWBF2Solution) {
            service.start()
        }
    }
}

@Service(Service.Level.PROJECT)
class LinkService(val project: Project, private val scope: CoroutineScope) : LinkListener {
    companion object {
        const val SOLUTION_FILE = "BattlefrontII.sln"

        fun of(project: Project): LinkService = project.service()

        fun portFolder(): Path {
            val base = System.getenv("LOCALAPPDATA") ?: System.getProperty("user.home")
            return Paths.get(base, "OpenSWBF2", "RiderLink")
        }
    }

    private val views = CopyOnWriteArrayList<LinkView>()
    private val ended = ConcurrentHashMap.newKeySet<PortEntry>()
    private val started = AtomicBoolean(false)

    @Volatile
    var status: LinkStatus = LinkStatus.Waiting
        private set

    @Volatile
    var state: PlayState? = null
        private set

    @Volatile
    var games: List<PortEntry> = emptyList()
        private set

    @Volatile
    var current: PortEntry? = null
        private set

    @Volatile
    private var chosen: Long? = null

    @Volatile
    private var session: LinkSession? = null

    @Volatile
    private var lastEnd: SessionEnd? = null

    @Volatile
    private var executableName: String = ""

    val isOpenSWBF2Solution: Boolean
        get() = project.solutionFile.name.equals(SOLUTION_FILE, ignoreCase = true)

    val connected: Boolean
        get() = status is LinkStatus.Connected

    val connectionInfo: ConnectionInfo?
        get() = (status as? LinkStatus.Connected)?.hello?.let { ConnectionInfo(it.pid, it.platform, it.config, executableName) }

    val playState: PlayState
        get() = state ?: PlayState.Idle

    val isGameControlInitialized: Boolean
        get() = state != null

    fun disconnectReason(): String = when (val now = status) {
        is LinkStatus.Disconnected -> when (val end = now.end) {
            is SessionEnd.WrongVersion -> ": the game speaks protocol ${end.version}, this plugin speaks ${Protocol.VERSION}"
            is SessionEnd.Failed -> ": ${end.reason}"
            else -> ""
        }
        else -> ""
    }

    fun addView(view: LinkView) {
        views.add(view)
    }

    fun removeView(view: LinkView) {
        views.remove(view)
    }

    fun start() {
        if (!started.compareAndSet(false, true)) {
            return
        }
        scope.launch(Dispatchers.IO) {
            try {
                while (isActive) {
                    poll()
                    delay(1000)
                }
            } finally {
                session?.close()
            }
        }
    }

    fun choose(pid: Long) {
        chosen = pid
        val open = session
        if (open != null && current?.pid != pid) {
            open.close()
        }
    }

    fun send(line: String) {
        val open = session ?: return
        scope.launch(Dispatchers.IO) {
            try {
                open.send(line)
            } catch (e: IOException) {
                open.close()
            }
        }
    }

    private fun poll() {
        val listed = PortFile.list(portFolder())
        ended.retainAll(listed.toSet())
        val discovery = PortFile.live(listed.filter { it !in ended }) { pid ->
            ProcessHandle.of(pid).map { it.isAlive }.orElse(false)
        }
        games = when (discovery) {
            is Discovery.Waiting -> emptyList()
            is Discovery.One -> listOf(discovery.entry)
            is Discovery.Several -> discovery.entries
        }
        if (session == null) {
            val target = when (discovery) {
                is Discovery.Waiting -> null
                is Discovery.One -> discovery.entry
                is Discovery.Several -> discovery.entries.firstOrNull { it.pid == chosen }
            }
            when {
                target != null -> open(target)
                discovery is Discovery.Several -> setStatus(LinkStatus.Choose)
                else -> setStatus(lastEnd?.let { LinkStatus.Disconnected(it) } ?: LinkStatus.Waiting)
            }
        }
    }

    private fun open(entry: PortEntry) {
        val link = LinkSession(entry, this)
        session = link
        current = entry
        state = null
        setStatus(LinkStatus.Connecting(entry))
        scope.launch(Dispatchers.IO) {
            val end = link.run()
            if (end !is SessionEnd.Closed) {
                ended.add(entry)
            }
            lastEnd = end
            session = null
            current = null
            state = null
            setStatus(LinkStatus.Disconnected(end))
        }
    }

    private fun setStatus(next: LinkStatus) {
        val was = status is LinkStatus.Connected
        status = next
        val now = next is LinkStatus.Connected
        ApplicationManager.getApplication().invokeLater({
            if (now && !was) {
                OpenSWBF2ToolWindowFactory.getInstance(project).showTabForNewSession()
                views.forEach { it.connected() }
            }
            forceTriggerUIUpdate()
        }, project.disposed)
    }

    private fun addLine(line: LogLine) {
        ApplicationManager.getApplication().invokeLater({
            views.forEach { it.lineAdded(line) }
        }, project.disposed)
    }

    override fun hello(hello: Message.Hello) {
        executableName = ProcessHandle.of(hello.pid).flatMap { it.info().command() }.map { Paths.get(it).fileName.toString() }.orElse("")
        setStatus(LinkStatus.Connected(hello))
    }

    override fun state(state: PlayState) {
        this.state = state
        ApplicationManager.getApplication().invokeLater({ forceTriggerUIUpdate() }, project.disposed)
    }

    override fun log(text: String) {
        addLine(LogLine.split(text))
    }

    override fun dropped(count: Long) {
        addLine(LogLine("", "", null, Verbosity.Warning, "OpenSWBF2 Link: the game dropped $count log lines because Rider read too slowly"))
    }
}
