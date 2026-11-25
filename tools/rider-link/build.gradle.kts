import groovy.json.JsonSlurper

plugins {
    id("org.jetbrains.kotlin.jvm") version "2.3.20"
    id("org.jetbrains.intellij.platform") version "2.20.1"
}

group = "org.openswbf2"
version = "0.1.0"

kotlin {
    jvmToolchain(21)
}

repositories {
    mavenCentral()
    intellijPlatform {
        defaultRepositories()
    }
}

fun findRider(): String {
    val given = providers.gradleProperty("riderHome")
    if (given.isPresent) {
        return given.get()
    }
    val programFiles = System.getenv("ProgramFiles") ?: "C:\\Program Files"
    val found = File(programFiles, "JetBrains").listFiles().orEmpty().filter { dir ->
        val info = File(dir, "product-info.json")
        if (!info.isFile) {
            false
        } else {
            val json = JsonSlurper().parse(info) as Map<*, *>
            json["productCode"] == "RD" && (json["buildNumber"] as? String).orEmpty().startsWith("261.")
        }
    }
    if (found.size != 1) {
        throw GradleException(
            "Expected one Rider 2026.1 (product RD, build 261.*) under $programFiles\\JetBrains, found ${found.size}. " +
                "Pass -PriderHome=<Rider install folder>."
        )
    }
    return found[0].path
}

dependencies {
    testImplementation("junit:junit:4.13.2")
    intellijPlatform {
        local(findRider())
        bundledPlugin("com.jetbrains.rider-cpp")
    }
}

intellijPlatform {
    buildSearchableOptions = false
    instrumentCode = false
    pluginConfiguration {
        ideaVersion {
            sinceBuild = "261"
            untilBuild = provider { null }
        }
    }
}

tasks.jar {
    from(files("NOTICE", "LICENSE-UnrealLink.txt")) {
        into("META-INF")
    }
}
