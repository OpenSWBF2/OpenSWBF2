package org.openswbf2.riderlink

import com.intellij.DynamicBundle
import org.jetbrains.annotations.Nls
import org.jetbrains.annotations.NonNls
import org.jetbrains.annotations.PropertyKey
import java.util.function.Supplier

@NonNls
const val BUNDLE = "messages.OpenSWBF2LinkBundle"

object OpenSWBF2LinkBundle {
  private val instance = DynamicBundle(OpenSWBF2LinkBundle::class.java, BUNDLE)
    @JvmStatic
    @Nls
    fun message(@PropertyKey(resourceBundle = BUNDLE) key: String, vararg params: Any): String {
        return instance.getMessage(key, *params)
    }

    @JvmStatic
    fun messagePointer(@PropertyKey(resourceBundle = BUNDLE) key: String, vararg params: Any): Supplier<String> {
        return instance.getLazyMessage(key, *params)
    }
}
