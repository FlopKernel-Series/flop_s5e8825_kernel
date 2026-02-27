package com.resukisu.resukisu.ui.viewmodel

import android.content.Context
import android.os.SystemClock
import android.util.Log
import androidx.compose.runtime.Stable
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.resukisu.resukisu.ksuApp
import com.resukisu.resukisu.ui.util.HanziToPinyin
import com.resukisu.resukisu.ui.util.getRootShell
import com.resukisu.resukisu.ui.util.listModules
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import org.json.JSONArray
import org.json.JSONObject
import java.text.Collator
import java.util.Locale

/**
 * @author ShirkNeko
 * @date 2025/5/31.
 */
class ModuleViewModel : ViewModel() {

    companion object {
        private const val TAG = "ModuleViewModel"
        private var modules by mutableStateOf<List<ModuleInfo>>(emptyList())
    }

    val moduleSize = MutableStateFlow<Map<String, String>>(emptyMap())

    fun loadSize(dirId: String) = viewModelScope.launch(Dispatchers.IO) {
        moduleSize.update {
            it + (dirId to
                    formatFileSize(
                        try {
                            val shell = getRootShell()
                            val command =
                                "/data/adb/ksu/bin/busybox du -sb /data/adb/modules/$dirId"
                            val result = shell.newJob().add(command).to(ArrayList(), null).exec()

                            if (result.isSuccess && result.out.isNotEmpty()) {
                                val sizeStr = result.out.firstOrNull()?.split("\t")?.firstOrNull()
                                sizeStr?.toLongOrNull() ?: 0L
                            } else {
                                0L
                            }
                        } catch (e: Exception) {
                            Log.e(TAG, "计算模块大小失败 $dirId: ${e.message}")
                            0L
                        }
                    ))
        }
    }

    data class ModuleUpdateInfo(
        val zipUrl: String,
        val version: String,
        val changelog: String
    )

    @Stable
    data class ModuleInfo(
        val id: String,
        val name: String,
        val author: String,
        val version: String,
        val versionCode: Int,
        val description: String,
        val enabled: Boolean,
        val update: Boolean,
        val remove: Boolean,
        val updateJson: String,
        val hasWebUi: Boolean,
        val hasActionScript: Boolean,
        val metamodule: Boolean,
        val actionIconPath: String?,
        val webUiIconPath: String?,
        val dirId: String, // real module id (dir name)
        val moduleUpdate: ModuleUpdateInfo?
    )

    var isRefreshing by mutableStateOf(false)
        private set
    var search by mutableStateOf("")

    var sortEnabledFirst by mutableStateOf(false)
    var sortActionFirst by mutableStateOf(false)
    val moduleList by derivedStateOf {
        val comparator =
            compareBy<ModuleInfo>(
                {
                    val executable = it.hasWebUi || it.hasActionScript
                    when {
                        it.metamodule && it.enabled -> 0
                        sortEnabledFirst && sortActionFirst -> when {
                            it.enabled && executable -> 1
                            it.enabled -> 2
                            executable -> 3
                            else -> 4
                        }
                        sortEnabledFirst && !sortActionFirst -> if (it.enabled) 1 else 2
                        !sortEnabledFirst && sortActionFirst -> if (executable) 1 else 2
                        else -> 1
                    }
                },
                { if (sortEnabledFirst) !it.enabled else 0 },
                { if (sortActionFirst) !(it.hasWebUi || it.hasActionScript) else 0 },
            ).thenBy(Collator.getInstance(Locale.getDefault()), ModuleInfo::id)
        modules.filter {
            it.id.contains(search, true) || it.name.contains(search, true) || HanziToPinyin.getInstance()
                .toPinyinString(it.name)?.contains(search, true) == true
        }.sortedWith(comparator).also {
            isRefreshing = false
        }
    }

    val haveWebuiModuleList by derivedStateOf {
        moduleList.filter { it.hasWebUi }
    }

    var isNeedRefresh by mutableStateOf(false)
        private set

    fun markNeedRefresh() {
        isNeedRefresh = true
    }

    fun fetchModuleList(
        manualRefresh: Boolean = false
    ) {
        viewModelScope.launch(Dispatchers.IO) {
            isRefreshing = true

            val oldModuleList = modules

            val start = SystemClock.elapsedRealtime()

            kotlin.runCatching {
                val result = listModules()

                Log.i(TAG, "result: $result")

                val moduleList = mutableListOf<String>()
                if (!manualRefresh) {
                    oldModuleList.forEach { module ->
                        moduleList.add(module.id + module.versionCode)
                    }
                }

                val array = JSONArray(result)
                modules = (0 until array.length())
                    .asSequence()
                    .map { array.getJSONObject(it) }
                    .map { obj ->
                        val moduleId = obj.getString("id")
                        val moduleVersionCode = obj.getIntCompat("versionCode", 0)
                        val enabled = obj.getBooleanCompat("enabled")
                        val update = obj.getBooleanCompat("update")
                        val remove = obj.getBooleanCompat("remove")
                        val updateJson = obj.optString("updateJson")

                        ModuleInfo(
                            id = moduleId,
                            name = obj.optString("name"),
                            author = obj.optString("author", "Unknown"),
                            version = obj.optString("version", "Unknown"),
                            versionCode = moduleVersionCode,
                            description = obj.optString("description"),
                            enabled = enabled,
                            update = update,
                            remove = remove,
                            updateJson = updateJson,
                            hasWebUi = obj.getBooleanCompat("web"),
                            hasActionScript = obj.getBooleanCompat("action"),
                            metamodule = obj.getBooleanCompat("metamodule"),
                            actionIconPath = obj.optString("actionIcon").takeIf { it.isNotBlank() },
                            webUiIconPath = obj.optString("webuiIcon").takeIf { it.isNotBlank() },
                            dirId = obj.optString("dir_id", obj.getString("id")),
                            moduleUpdate = null // we null moduleUpdate there, because checkUpdate may request network
                        )
                    }.toList()

                modules = modules.map { module ->
                    async(Dispatchers.IO) {
                        module.copy(
                            moduleUpdate = if (!moduleList.contains(module.id + module.versionCode) || module.updateJson.isEmpty() || module.remove || module.update || !module.enabled)
                                checkUpdate(module.updateJson, module.versionCode)
                            else null
                        )
                    }
                }.awaitAll()

                isNeedRefresh = false
            }.onFailure { e ->
                Log.e(TAG, "fetchModuleList: ", e)
                isRefreshing = false
            }

            // when both old and new is kotlin.collections.EmptyList
            // moduleList update will don't trigger
            if (oldModuleList === modules) {
                isRefreshing = false
            }

            Log.i(TAG, "load cost: ${SystemClock.elapsedRealtime() - start}, modules: $modules")
        }
    }

    private fun sanitizeVersionString(version: String): String {
        return version.replace(Regex("[^a-zA-Z0-9.\\-_]"), "_")
    }

    fun checkUpdate(updateUrl: String, versionCode: Int): ModuleUpdateInfo? {
        val isCheckUpdateEnabled = ksuApp.getSharedPreferences("settings", Context.MODE_PRIVATE)
            .getBoolean("check_update", true)
        if (!isCheckUpdateEnabled) {
            return null
        }
        // download updateJson
        val result = kotlin.runCatching {
            Log.i(TAG, "checkUpdate url: $updateUrl")

            val request = okhttp3.Request.Builder()
                .url(updateUrl)
                .build()

            val response = ksuApp.okhttpClient.newCall(request).execute()

            Log.d(TAG, "checkUpdate code: ${response.code}")
            if (response.isSuccessful) {
                response.body?.string() ?: ""
            } else {
                Log.d(TAG, "checkUpdate failed: ${response.message}")
                ""
            }
        }.getOrElse { e ->
            Log.e(TAG, "checkUpdate exception", e)
            ""
        }

        Log.i(TAG, "checkUpdate result: $result")

        if (result.isEmpty()) {
            return null
        }

        val updateJson = kotlin.runCatching {
            JSONObject(result)
        }.getOrNull() ?: return null

        var version = updateJson.optString("version", "")
        version = sanitizeVersionString(version)
        val onlineVersionCode = updateJson.optInt("versionCode", 0)
        val zipUrl = updateJson.optString("zipUrl", "")
        val changelog = updateJson.optString("changelog", "")
        if (onlineVersionCode <= versionCode || zipUrl.isEmpty()) {
            return null
        }

        return ModuleUpdateInfo(zipUrl, version, changelog)
    }
}

fun ModuleViewModel.ModuleInfo.copy(
    id: String = this.id,
    name: String = this.name,
    author: String = this.author,
    version: String = this.version,
    versionCode: Int = this.versionCode,
    description: String = this.description,
    enabled: Boolean = this.enabled,
    update: Boolean = this.update,
    remove: Boolean = this.remove,
    updateJson: String = this.updateJson,
    hasWebUi: Boolean = this.hasWebUi,
    hasActionScript: Boolean = this.hasActionScript,
    metamodule: Boolean = this.metamodule,
    actionIconPath: String? = this.actionIconPath,
    webUiIconPath: String? = this.webUiIconPath,
    dirId: String = this.dirId,
    moduleUpdate: ModuleViewModel.ModuleUpdateInfo? = this.moduleUpdate,
): ModuleViewModel.ModuleInfo {
    return ModuleViewModel.ModuleInfo(
        id, name, author, version, versionCode, description,
        enabled, update, remove, updateJson, hasWebUi, hasActionScript, metamodule,
        actionIconPath, webUiIconPath, dirId, moduleUpdate
    )
}

private fun JSONObject.getBooleanCompat(key: String, default: Boolean = false): Boolean {
    if (!has(key)) return default
    return when (val value = opt(key)) {
        is Boolean -> value
        is String -> value.equals("true", ignoreCase = true) || value == "1"
        is Number -> value.toInt() != 0
        else -> default
    }
}

private fun JSONObject.getIntCompat(key: String, default: Int = 0): Int {
    if (!has(key)) return default
    return when (val value = opt(key)) {
        is Int -> value
        is Number -> value.toInt()
        is String -> value.toIntOrNull() ?: default
        else -> default
    }
}

/**
 * 格式化文件大小
 */
fun formatFileSize(bytes: Long): String {
    val kb = 1024.0
    val mb = kb * 1024
    val gb = mb * 1024
    val tb = gb * 1024

    return when {
        bytes >= tb -> "%.2f TB".format(bytes / tb)
        bytes >= gb -> "%.2f GB".format(bytes / gb)
        bytes >= mb -> "%.2f MB".format(bytes / mb)
        bytes >= kb -> "%.2f KB".format(bytes / kb)
        else -> "$bytes B"
    }
}
