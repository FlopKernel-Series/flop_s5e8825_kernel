package com.rifsxd.ksunext.ui.screen

import android.content.Context
import android.content.SharedPreferences
import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.CloudDownload
import androidx.compose.material.icons.filled.SettingsSuggest
import androidx.compose.material.icons.filled.FileDownload
import androidx.compose.material.icons.filled.Sync
import androidx.compose.material.icons.filled.Edit
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import com.rifsxd.ksunext.ui.LocalScrollState
import com.rifsxd.ksunext.ui.rememberScrollConnection
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.dropUnlessResumed
import com.ramcosta.composedestinations.annotation.Destination
import com.ramcosta.composedestinations.annotation.RootGraph
import com.ramcosta.composedestinations.generated.destinations.FlashScreenDestination
import com.ramcosta.composedestinations.generated.destinations.MetaModuleScreenDestination
import com.ramcosta.composedestinations.navigation.DestinationsNavigator
import com.ramcosta.composedestinations.navigation.EmptyDestinationsNavigator
import com.rifsxd.ksunext.BuildConfig
import com.rifsxd.ksunext.Natives
import com.rifsxd.ksunext.R
import com.rifsxd.ksunext.ui.util.LocalSnackbarHost
import com.rifsxd.ksunext.ui.screen.FlashIt
import com.rifsxd.ksunext.ui.util.download
import com.rifsxd.ksunext.ui.util.DownloadListener
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import androidx.lifecycle.viewmodel.compose.viewModel
import com.dergoogler.mmrl.ui.component.LabelItem
import com.dergoogler.mmrl.ui.component.LabelItemDefaults
import com.rifsxd.ksunext.ui.viewmodel.ModuleViewModel
import com.rifsxd.ksunext.ui.component.SearchAppBar
import org.json.JSONArray
import java.io.BufferedReader
import java.io.InputStreamReader
import java.net.URL
import android.content.Intent

data class ModuleRepo(
    val id: String,
    val name: String,
    val description: String,
    val author: String,
    val repoUrl: String,
    val visibility: Int = 1,
    val latestVersion: String = "",
    val downloadUrl: String = "",
    val isLoading: Boolean = true
)

sealed class ModuleRepoState {
    object Loading : ModuleRepoState()
    data class Success(val modules: List<ModuleRepo>) : ModuleRepoState()
    data class Error(val message: String) : ModuleRepoState()
}

// SharedPreferences helper functions
private const val PREFS_NAME = "module_repo_prefs"
private const val KEY_JSON_URL = "json_url"
private const val DEFAULT_JSON_URL = "https://raw.githubusercontent.com/KernelSU-Next/KernelSU-Next-Modules-Repo/refs/heads/main/modules.json"

private fun getModuleRepoPrefs(context: Context): SharedPreferences {
    return context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
}

private fun saveJsonUrl(context: Context, url: String) {
    getModuleRepoPrefs(context).edit().putString(KEY_JSON_URL, url).apply()
}

private fun loadJsonUrl(context: Context): String {
    return getModuleRepoPrefs(context).getString(KEY_JSON_URL, DEFAULT_JSON_URL) ?: DEFAULT_JSON_URL
}

/**
 * @author rifsxd
 * @date 2026/1/29.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Destination<RootGraph>
@Composable
fun ModuleRepoScreen(navigator: DestinationsNavigator) {
    val scrollBehavior = TopAppBarDefaults.pinnedScrollBehavior(rememberTopAppBarState())
    // Bottom bar scroll tracking
    val bottomBarScrollState = LocalScrollState.current
    val bottomBarScrollConnection = if (bottomBarScrollState != null) {
        rememberScrollConnection(
            isScrollingDown = bottomBarScrollState.isScrollingDown,
            scrollOffset = bottomBarScrollState.scrollOffset,
            previousScrollOffset = bottomBarScrollState.previousScrollOffset,
            threshold = 30f
        )
    } else null
    val snackBarHost = LocalSnackbarHost.current
    val context = LocalContext.current
    val scope = rememberCoroutineScope()

    val isManager = Natives.isManager
    val ksuVersion = if (isManager) Natives.version else null
    val navBarPadding = WindowInsets.navigationBars.asPaddingValues().calculateBottomPadding() + 112.dp
    
    var modulesJsonUrl by remember { 
        mutableStateOf(loadJsonUrl(context)) 
    }
    var showEditDialog by remember { mutableStateOf(false) }
    var editedUrl by remember { mutableStateOf(modulesJsonUrl) }
    var searchText by remember { mutableStateOf("") }

    var moduleState by remember { mutableStateOf<ModuleRepoState>(ModuleRepoState.Loading) }
    var selectedModule by remember { mutableStateOf<ModuleRepo?>(null) }
    var downloadingModuleId by remember { mutableStateOf<String?>(null) }
    var downloadedUri by remember { mutableStateOf<Uri?>(null) }

    DownloadListener(context) { uri ->
        downloadedUri = uri
        downloadingModuleId = null
        navigator.navigate(
            FlashScreenDestination(FlashIt.FlashModules(listOf(uri)))
        )
    }

    val moduleViewModel = viewModel<ModuleViewModel>()

    suspend fun fetchModuleReposFromJson(jsonUrl: String): List<ModuleRepo>? {
        return withContext(Dispatchers.IO) {
            try {
                val conn = URL(jsonUrl).openConnection() as java.net.HttpURLConnection
                conn.setRequestProperty("User-Agent", "KernelSU/${BuildConfig.VERSION_CODE}")
                val text = BufferedReader(InputStreamReader(conn.inputStream)).use { it.readText() }
                conn.disconnect()

                val arr = JSONArray(text)
                val out = mutableListOf<ModuleRepo>()
                for (i in 0 until arr.length()) {
                    val obj = arr.getJSONObject(i)
                    out += ModuleRepo(
                        id = obj.optString("id"),
                        name = obj.optString("name"),
                        description = obj.optString("description"),
                        author = obj.optString("author"),
                        repoUrl = obj.optString("repoUrl"),
                        visibility = obj.optInt("visibility", 1)
                    )
                }
                out
            } catch (e: Exception) {
                e.printStackTrace()
                null
            }
        }
    }

    suspend fun loadModules() {
        try {
            moduleState = ModuleRepoState.Loading

            val baseList = withContext(Dispatchers.IO) {
                fetchModuleReposFromJson(modulesJsonUrl)
            }

            if (baseList == null) {
                moduleState = ModuleRepoState.Error("Failed to load module list")
                return
            }

            // Filter out modules with visibility = 0
            val visibleModules = baseList.filter { it.visibility == 1 }

            moduleState = ModuleRepoState.Success(
                visibleModules.map { it.copy(latestVersion = "", downloadUrl = "", isLoading = true) }
            )

            kotlinx.coroutines.coroutineScope {
                visibleModules.forEach { baseModule ->
                    launch {
                        try {
                            val releaseInfo = withContext(Dispatchers.IO) { fetchLatestReleaseInfo(baseModule.repoUrl) }
                            val updated = baseModule.copy(
                                latestVersion = releaseInfo?.version ?: "null",
                                downloadUrl = releaseInfo?.zipUrl ?: "",
                                isLoading = false
                            )

                            val current = moduleState
                            if (current is ModuleRepoState.Success) {
                                val mutable = current.modules.toMutableList()
                                val idx = mutable.indexOfFirst { it.id == baseModule.id }
                                if (idx >= 0) mutable[idx] = updated else mutable.add(updated)
                                moduleState = ModuleRepoState.Success(mutable)
                            }
                        } catch (e: Exception) {
                            val updated = baseModule.copy(latestVersion = "null", downloadUrl = "", isLoading = false)
                            val current = moduleState
                            if (current is ModuleRepoState.Success) {
                                val mutable = current.modules.toMutableList()
                                val idx = mutable.indexOfFirst { it.id == baseModule.id }
                                if (idx >= 0) mutable[idx] = updated else mutable.add(updated)
                                moduleState = ModuleRepoState.Success(mutable)
                            }
                        }
                    }
                }
            }
        } catch (e: Exception) {
            moduleState = ModuleRepoState.Error(e.message ?: "Unknown error")
        }
    }

    LaunchedEffect(modulesJsonUrl) {
        scope.launch {
            loadModules()
        }
    }

    // Edit URL Dialog
    if (showEditDialog) {
        AlertDialog(
            onDismissRequest = { 
                showEditDialog = false
                editedUrl = modulesJsonUrl
            },
            title = { Text("Edit JSON URL") },
            text = {
                Column {
                    Text(
                        text = "Enter the module repository JSON URL:",
                        style = MaterialTheme.typography.bodyMedium,
                        modifier = Modifier.padding(bottom = 8.dp)
                    )
                    OutlinedTextField(
                        value = editedUrl,
                        onValueChange = { editedUrl = it },
                        modifier = Modifier.fillMaxWidth(),
                        singleLine = false,
                        maxLines = 3,
                        placeholder = { Text("https://...") }
                    )
                    
                    // Reset to default button
                    TextButton(
                        onClick = { 
                            editedUrl = DEFAULT_JSON_URL
                        },
                        modifier = Modifier.padding(top = 8.dp)
                    ) {
                        Text("Reset to Default")
                    }
                }
            },
            confirmButton = {
                TextButton(
                    onClick = {
                        if (editedUrl.isNotBlank()) {
                            modulesJsonUrl = editedUrl
                            saveJsonUrl(context, editedUrl)
                            showEditDialog = false
                        }
                    }
                ) {
                    Text("Apply")
                }
            },
            dismissButton = {
                TextButton(
                    onClick = { 
                        showEditDialog = false
                        editedUrl = modulesJsonUrl
                    }
                ) {
                    Text("Cancel")
                }
            }
        )
    }

    Scaffold(
        topBar = {
            SearchAppBar(
                title = {
                    Text(
                        text = stringResource(R.string.module_repo_screen),
                        style = MaterialTheme.typography.titleLarge,
                        fontWeight = FontWeight.Black,
                    )
                },
                searchText = searchText,
                onSearchTextChange = { searchText = it },
                onClearClick = { searchText = "" },
                onBackClick = dropUnlessResumed { navigator.popBackStack() },
                actionsContent = {
                    IconButton(
                        onClick = {
                            editedUrl = modulesJsonUrl
                            showEditDialog = true
                        }
                    ) {
                        Icon(
                            imageVector = Icons.Filled.Edit,
                            contentDescription = "Edit JSON URL"
                        )
                    }
                    IconButton(
                        onClick = {
                            navigator.navigate(MetaModuleScreenDestination)
                        }
                    ) {
                        Icon(
                            imageVector = Icons.Filled.SettingsSuggest,
                            contentDescription = stringResource(id = R.string.module_repo_screen)
                        )
                    }
                },
                dropdownContent = {
                    IconButton(onClick = {
                        scope.launch {
                            moduleState = ModuleRepoState.Loading
                            loadModules()
                        }
                    }) {
                        Icon(Icons.Default.Sync, contentDescription = "Refresh")
                    }
                },
                scrollBehavior = scrollBehavior
            )
        },
        snackbarHost = { SnackbarHost(snackBarHost, modifier = Modifier.padding(bottom = navBarPadding)) },
        contentWindowInsets = WindowInsets.safeDrawing.only(WindowInsetsSides.Top + WindowInsetsSides.Horizontal)
    ) { paddingValues ->
        when (val state = moduleState) {
            is ModuleRepoState.Loading -> {
                Box(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(paddingValues),
                    contentAlignment = Alignment.Center
                ) {
                    CircularProgressIndicator()
                }
            }
            is ModuleRepoState.Success -> {
                val installedIds = moduleViewModel.moduleList.map { it.id }
                
                // Filter modules based on search text
                val filteredModules = if (searchText.isBlank()) {
                    state.modules
                } else {
                    state.modules.filter { module ->
                        module.name.contains(searchText, ignoreCase = true) ||
                        module.description.contains(searchText, ignoreCase = true) ||
                        module.author.contains(searchText, ignoreCase = true) ||
                        module.id.contains(searchText, ignoreCase = true)
                    }
                }
                
                // Sort modules alphabetically by name (A to Z)
                val sortedModules = filteredModules.sortedBy { it.name.lowercase() }
                
                LazyColumn(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(paddingValues)
                        .let { modifier ->
                            if (bottomBarScrollConnection != null) {
                                modifier
                                    .nestedScroll(bottomBarScrollConnection)
                                    .nestedScroll(scrollBehavior.nestedScrollConnection)
                            } else {
                                modifier.nestedScroll(scrollBehavior.nestedScrollConnection)
                            }
                        },
                    contentPadding = PaddingValues(start = 16.dp, end = 16.dp, top = 8.dp, bottom = 8.dp + 16.dp + navBarPadding),
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    items(sortedModules) { module ->
                        val isInstalled = installedIds.contains(module.id)
                        val isThisModuleDownloading = downloadingModuleId == module.id
                        
                        val isInstallEnabled = when {
                            downloadingModuleId != null && !isThisModuleDownloading -> false
                            else -> true
                        }

                        ModuleRepoCard(
                            module = module,
                            isInstalled = isInstalled,
                            isInstallEnabled = isInstallEnabled,
                            isDownloading = isThisModuleDownloading,
                            onCardClick = {
                                val intent = Intent(Intent.ACTION_VIEW, Uri.parse(module.repoUrl))
                                context.startActivity(intent)
                            },
                            onDownload = { selectedModule ->
                                downloadingModuleId = selectedModule.id
                                scope.launch {
                                    try {
                                        val fileName = "${selectedModule.name.replace(" ", "_")}_${selectedModule.latestVersion.replace("/", "_")}.zip"
                                        download(
                                            context,
                                            selectedModule.downloadUrl,
                                            fileName,
                                            "Downloading ${selectedModule.name}"
                                        )
                                    } catch (e: Exception) {
                                        snackBarHost.showSnackbar("Error downloading module: ${e.message}")
                                        downloadingModuleId = null
                                    }
                                }
                            }
                        )
                    }
                }
            }
            is ModuleRepoState.Error -> {
                Box(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(paddingValues),
                    contentAlignment = Alignment.Center
                ) {
                    Column(
                        horizontalAlignment = Alignment.CenterHorizontally,
                        modifier = Modifier.padding(16.dp)
                    ) {
                        Text(
                            text = stringResource(R.string.error_loading_modules),
                            style = MaterialTheme.typography.titleMedium
                        )
                        Button(
                            onClick = {
                                scope.launch {
                                    moduleState = ModuleRepoState.Loading
                                    loadModules()
                                }
                            },
                            modifier = Modifier.padding(top = 16.dp)
                        ) {
                            Text(stringResource(R.string.retry))
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun ModuleRepoCard(
    module: ModuleRepo,
    isInstalled: Boolean = false,
    isInstallEnabled: Boolean = true,
    isDownloading: Boolean = false,
    onCardClick: () -> Unit,
    onDownload: (ModuleRepo) -> Unit,
    modifier: Modifier = Modifier
) {
    Card(
        modifier = modifier
            .fillMaxWidth()
            .clickable { onCardClick() }
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp)
        ) {
            Row(modifier = Modifier.fillMaxWidth()) {
                if (isInstalled) {
                    LabelItem(
                        text = stringResource(R.string.installed),
                        style = LabelItemDefaults.style.copy(
                            containerColor = MaterialTheme.colorScheme.primaryContainer,
                            contentColor = MaterialTheme.colorScheme.onPrimaryContainer
                        )
                    )
                    Spacer(modifier = Modifier.width(8.dp))
                }
            }
            Row(
                modifier = Modifier
                    .fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column(
                    modifier = Modifier.weight(1f)
                ) {
                    Text(
                        text = module.name,
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.Bold
                    )
                    Text(
                        text = "by ${module.author}",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }

            Text(
                text = module.description,
                style = MaterialTheme.typography.bodySmall,
                modifier = Modifier.padding(top = 8.dp)
            )

            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(top = 12.dp),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column {
                    Text(
                        text = stringResource(R.string.latest_version),
                        style = MaterialTheme.typography.labelSmall
                    )
                    Text(
                        text = if (module.latestVersion.isNotEmpty()) module.latestVersion else stringResource(R.string.loading),
                        style = MaterialTheme.typography.bodyMedium,
                        fontWeight = FontWeight.SemiBold
                    )
                }

                Button(
                    onClick = { onDownload(module) },
                    modifier = Modifier.height(40.dp),
                    enabled = module.downloadUrl.isNotEmpty() && isInstallEnabled,
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 0.dp)
                ) {
                    if (isDownloading) {
                        CircularProgressIndicator(
                            modifier = Modifier.size(18.dp),
                            strokeWidth = 2.dp,
                            color = MaterialTheme.colorScheme.onPrimary
                        )
                    } else {
                        Icon(
                            imageVector = Icons.Default.CloudDownload,
                            contentDescription = "Download",
                            modifier = Modifier.size(18.dp)
                        )
                    }
                    Spacer(modifier = Modifier.width(4.dp))
                    Text(if (isInstalled) stringResource(R.string.reinstall) else stringResource(R.string.install))
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun TopBar(
    onBack: () -> Unit = {},
    onRefresh: () -> Unit = {},
    onMetaModuleClick: () -> Unit = {},
    onEditUrl: () -> Unit = {},
    scrollBehavior: TopAppBarScrollBehavior? = null
) {
    TopAppBar(
        title = {
            Text(
                text = stringResource(R.string.module_repo_screen),
                style = MaterialTheme.typography.titleLarge,
                fontWeight = FontWeight.Black,
            )
        },
        navigationIcon = {
            IconButton(onClick = onBack) {
                Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = null)
            }
        },
        actions = {
            IconButton(
                onClick = onEditUrl
            ) {
                Icon(
                    imageVector = Icons.Filled.Edit,
                    contentDescription = "Edit JSON URL"
                )
            }
            IconButton(
                onClick = onMetaModuleClick
            ) {
                Icon(
                    imageVector = Icons.Filled.SettingsSuggest,
                    contentDescription = stringResource(id = R.string.module_repo_screen)
                )
            }
            IconButton(onClick = onRefresh) {
                Icon(Icons.Default.Sync, contentDescription = "Refresh")
            }
        },
        windowInsets = WindowInsets.safeDrawing.only(WindowInsetsSides.Top + WindowInsetsSides.Horizontal),
        scrollBehavior = scrollBehavior
    )
}

private suspend fun fetchLatestReleaseInfo(repoUrl: String): ReleaseInfoModuleRepo? {
    return withContext(Dispatchers.IO) {
        try {
            val latestUrl = "$repoUrl/releases/latest"
            val connection = URL(latestUrl).openConnection() as java.net.HttpURLConnection
            connection.instanceFollowRedirects = false
            connection.setRequestProperty("User-Agent", "KernelSU/${BuildConfig.VERSION_CODE}")
            
            val redirectUrl = connection.getHeaderField("Location")
            connection.disconnect()
            
            if (redirectUrl == null) return@withContext null
            
            val tagMatch = """/tag/([^/?\s]+)""".toRegex()
                .find(redirectUrl)?.groupValues?.get(1)
            
            if (tagMatch == null) return@withContext null
            
            val releasePageUrl = "$repoUrl/releases/expanded_assets/$tagMatch"
            val pageConnection = URL(releasePageUrl).openConnection()
            pageConnection.setRequestProperty("User-Agent", "KernelSU/${BuildConfig.VERSION_CODE}")
            
            val pageHtml = BufferedReader(InputStreamReader(pageConnection.getInputStream())).use {
                it.readText()
            }
        
            val zipUrlMatch = """href="(/[^"]+/releases/download/[^"]+\.zip)"""".toRegex()
                .find(pageHtml)?.groupValues?.get(1)
            
            if (zipUrlMatch != null) {
                return@withContext ReleaseInfoModuleRepo(
                    version = tagMatch,
                    zipUrl = "https://github.com$zipUrlMatch"
                )
            }
            
            null
        } catch (e: Exception) {
            e.printStackTrace()
            null
        }
    }
}

data class ReleaseInfoModuleRepo(
    val version: String,
    val zipUrl: String
)

@Preview
@Composable
private fun ModuleRepoScreenPreview() {
    ModuleRepoScreen(EmptyDestinationsNavigator)
}