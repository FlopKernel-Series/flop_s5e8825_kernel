package me.weishu.kernelsu.ui.viewmodel

import android.util.Log
import android.widget.Toast
import androidx.compose.runtime.Immutable
import androidx.compose.runtime.State
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.text.input.TextFieldValue
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import me.weishu.kernelsu.R
import me.weishu.kernelsu.ksuApp
import me.weishu.kernelsu.ui.util.HanziToPinyin
import me.weishu.kernelsu.ui.util.isNetworkAvailable
import okhttp3.Request
import org.json.JSONArray
import org.json.JSONObject

class ModuleRepoViewModel : ViewModel() {

    companion object {
        private const val TAG = "ModuleRepoViewModel"
        private const val OSS_URL = "https://raw.githubusercontent.com/KernelSU-Next/KernelSU-Next-Modules-Repo/main/modules.json"
        private const val META_URL = "https://raw.githubusercontent.com/KernelSU-Next/KernelSU-Next-Modules-Repo/main/meta_modules.json"
        private const val NON_FREE_URL = "https://raw.githubusercontent.com/KernelSU-Next/KernelSU-Next-Modules-Repo/main/non_free_modules.json"
    }

    @Immutable
    data class Author(
        val name: String,
        val link: String,
    )

    @Immutable
    data class ReleaseAsset(
        val name: String,
        val downloadUrl: String,
        val size: Long
    )

    @Immutable
    data class RepoModule(
        val moduleId: String,
        val moduleName: String,
        val authors: String,
        val authorList: List<Author>,
        val summary: String,
        val repoUrl: String?,
        val license: String?,
        val bannerUrl: String?,
        val repoType: String,
        val metamodule: Boolean = false,
        val stargazerCount: Int = 0,
        val updatedAt: String = "",
        val createdAt: String = "",
        val latestRelease: String = "",
        val latestReleaseTime: String = "",
        val latestVersionCode: Int = 0,
        val latestAsset: ReleaseAsset? = null,
    )

    private var _modules = mutableStateOf<List<RepoModule>>(emptyList())
    val modules: State<List<RepoModule>> = _modules

    var search by mutableStateOf(TextFieldValue(""))

    val filteredModules by derivedStateOf {
        val searchText = search.text
        modules.value.filter { module ->
            module.moduleId.contains(searchText, true) || module.moduleName.contains(searchText, true) ||
                    HanziToPinyin.getInstance().toPinyinString(module.moduleName).contains(searchText, true) ||
                    module.summary.contains(searchText, true) || module.authors.contains(searchText, true)
        }
    }

    var isRefreshing by mutableStateOf(false)
        private set

    fun refresh() {
        viewModelScope.launch {
            val netAvailable = isNetworkAvailable(ksuApp)
            withContext(Dispatchers.Main) { isRefreshing = true }

            val parsed = withContext(Dispatchers.IO) {
                if (!netAvailable) null else fetchAllModules()
            }

            withContext(Dispatchers.Main) {
                if (parsed != null) {
                    _modules.value = parsed
                } else {
                    Toast.makeText(
                        ksuApp,
                        ksuApp.getString(R.string.network_offline), Toast.LENGTH_SHORT
                    ).show()
                }
                isRefreshing = false
            }
        }
    }

    private suspend fun fetchAllModules(): List<RepoModule> = withContext(Dispatchers.IO) {
        val ossTask = async { fetchAndParseList(OSS_URL, "OSS") }
        val metaTask = async { fetchAndParseList(META_URL, "META") }
        val nonFreeTask = async { fetchAndParseList(NON_FREE_URL, "NON-FREE") }
        val allResults = awaitAll(ossTask, metaTask, nonFreeTask)
        return@withContext allResults.flatten()
    }

    private fun fetchAndParseList(url: String, repoType: String): List<RepoModule> {
        return runCatching {
            val request = Request.Builder().url(url).build()
            ksuApp.okhttpClient.newCall(request).execute().use { resp ->
                if (!resp.isSuccessful) return emptyList()
                val body = resp.body?.string() ?: return emptyList()
                val json = JSONArray(body)
                (0 until json.length()).mapNotNull { idx ->
                    val item = json.optJSONObject(idx) ?: return@mapNotNull null
                    parseRepoModule(item, repoType)
                }
            }
        }.getOrElse {
            Log.e(TAG, "Fetch modules failed for $url", it)
            emptyList()
        }
    }

    private fun parseRepoModule(item: JSONObject, repoType: String): RepoModule? {
        val moduleName = item.optString("name", item.optString("moduleName", ""))
        val repoUrl = item.optString("repoUrl", "")
        val moduleId = item.optString("id", item.optString("moduleId", "")).ifEmpty {
            repoUrl.trimEnd('/').substringAfterLast('/')
        }.ifEmpty {
            moduleName.replace("\\s+".toRegex(), "_")
        }

        if (moduleId.isEmpty() || moduleName.isEmpty()) return null

        val summary = item.optString("description", item.optString("summary", ""))
        val authors = item.optString("author", item.optString("authors", ""))
        val license = item.optString("license", "")
        val bannerUrl = item.optString("bannerUrl", "")

        val authorList = mutableListOf<Author>()
        if (authors.isNotEmpty()) {
            authorList.add(Author(name = authors, link = ""))
        }

        val metamodule = repoType == "META" || item.optBoolean("metamodule", false)
        val stargazerCount = item.optInt("stargazerCount", 0)
        val updatedAt = item.optString("updatedAt", "")
        val createdAt = item.optString("createdAt", "")

        var latestRelease = ""
        var latestReleaseTime = ""
        var latestVersionCode = 0
        var latestAsset: ReleaseAsset? = null

        val lr = item.optJSONObject("latestRelease")
        if (lr != null) {
            val lrName = lr.optString("name", lr.optString("version", ""))
            val lrTime = lr.optString("time", "")
            var lrUrl = lr.optString("downloadUrl", "")
            lrUrl = lrUrl.trim().let {
                var s = it
                if (s.startsWith("`") && s.endsWith("`") && s.length >= 2) {
                    s = s.substring(1, s.length - 1)
                }
                s
            }
            val vcAny = lr.opt("versionCode")
            latestVersionCode = when (vcAny) {
                is Number -> vcAny.toInt()
                is String -> vcAny.toIntOrNull() ?: 0
                else -> 0
            }
            latestRelease = lrName
            latestReleaseTime = lrTime
            if (lrUrl.isNotEmpty()) {
                val fileName = lrUrl.substringAfterLast('/')
                latestAsset = ReleaseAsset(name = fileName, downloadUrl = lrUrl, size = 0L)
            }
        }

        return RepoModule(
            moduleId = moduleId,
            moduleName = moduleName,
            authors = authors,
            authorList = authorList,
            summary = summary,
            repoUrl = repoUrl,
            license = license,
            bannerUrl = bannerUrl,
            repoType = repoType,
            metamodule = metamodule,
            stargazerCount = stargazerCount,
            updatedAt = updatedAt,
            createdAt = createdAt,
            latestRelease = latestRelease,
            latestReleaseTime = latestReleaseTime,
            latestVersionCode = latestVersionCode,
            latestAsset = latestAsset,
        )
    }
}
