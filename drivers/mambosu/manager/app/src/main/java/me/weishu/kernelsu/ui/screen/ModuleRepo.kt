package me.weishu.kernelsu.ui.screen

import android.annotation.SuppressLint
import android.content.Context
import android.net.Uri
import android.os.Parcelable
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.Spring
import androidx.compose.animation.core.spring
import androidx.compose.animation.expandVertically
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.shrinkVertically
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.add
import androidx.compose.foundation.layout.asPaddingValues
import androidx.compose.foundation.layout.calculateEndPadding
import androidx.compose.foundation.layout.calculateStartPadding
import androidx.compose.foundation.layout.defaultMinSize
import androidx.compose.foundation.layout.displayCutout
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.navigationBars
import androidx.compose.foundation.layout.only
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawing
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.systemBars
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.pager.HorizontalPager
import androidx.compose.foundation.pager.rememberPagerState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.automirrored.outlined.ChromeReaderMode
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.outlined.Download
import androidx.compose.material.icons.outlined.Extension
import androidx.compose.material.icons.outlined.Link
import androidx.compose.material.icons.rounded.Star
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Checkbox
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.CircularWavyProgressIndicator
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExperimentalMaterial3ExpressiveApi
import androidx.compose.material3.FilledTonalButton
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LoadingIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.PrimaryTabRow
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Tab
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.material3.TopAppBarScrollBehavior
import androidx.compose.material3.rememberTopAppBarState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalLayoutDirection
import androidx.compose.ui.platform.LocalUriHandler
import androidx.compose.ui.platform.UriHandler
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.TextFieldValue
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.zIndex
import androidx.core.content.edit
import androidx.lifecycle.viewmodel.compose.viewModel
import coil.compose.AsyncImage
import coil.request.ImageRequest
import com.ramcosta.composedestinations.annotation.Destination
import com.ramcosta.composedestinations.annotation.RootGraph
import com.ramcosta.composedestinations.generated.destinations.FlashScreenDestination
import com.ramcosta.composedestinations.generated.destinations.ModuleRepoDetailScreenDestination
import com.ramcosta.composedestinations.navigation.DestinationsNavigator
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlinx.parcelize.Parcelize
import me.weishu.kernelsu.R
import me.weishu.kernelsu.ui.component.ConfirmDialogHandle
import me.weishu.kernelsu.ui.component.ExpressiveList
import me.weishu.kernelsu.ui.component.ExpressiveListItem
import me.weishu.kernelsu.ui.component.GithubMarkdown
import me.weishu.kernelsu.ui.component.SearchAppBar
import me.weishu.kernelsu.ui.component.rememberConfirmDialog
import me.weishu.kernelsu.ui.util.DownloadListener
import me.weishu.kernelsu.ui.util.download
import me.weishu.kernelsu.ui.util.isNetworkAvailable
import me.weishu.kernelsu.ui.util.module.fetchModuleDetail
import me.weishu.kernelsu.ui.viewmodel.ModuleRepoViewModel
import me.weishu.kernelsu.ui.viewmodel.ModuleViewModel
import org.json.JSONArray
import java.io.BufferedReader
import java.io.InputStreamReader
import java.net.URL
import java.text.Collator
import java.util.Locale

@Parcelize
data class ReleaseAssetArg(
    val name: String,
    val downloadUrl: String,
    val size: Long,
    val downloadCount: Int
) : Parcelable

@Parcelize
data class ReleaseArg(
    val tagName: String,
    val name: String,
    val publishedAt: String,
    val assets: List<ReleaseAssetArg>,
    val descriptionHTML: String
) : Parcelable

@Parcelize
data class AuthorArg(
    val name: String,
    val link: String,
) : Parcelable

@Parcelize
data class RepoModuleArg(
    val moduleId: String,
    val moduleName: String,
    val authors: String,
    val description: String,
    val repoUrl: String,
    val license: String,
    val bannerUrl: String?,
    val repoType: String,
    val authorsList: List<AuthorArg> = emptyList(),
    val latestRelease: String = "",
    val latestReleaseTime: String = "",
    val releases: List<ReleaseArg> = emptyList()
) : Parcelable

@SuppressLint("LocalContextGetResourceValueCall")
@OptIn(ExperimentalMaterial3Api::class, ExperimentalMaterial3ExpressiveApi::class)
@Composable
@Destination<RootGraph>
fun ModuleRepoScreen(
    navigator: DestinationsNavigator
) {
    val viewModel = viewModel<ModuleRepoViewModel>()
    val installedVm = viewModel<ModuleViewModel>()
    val context = LocalContext.current
    val prefs = context.getSharedPreferences("settings", Context.MODE_PRIVATE)
    val repoSortByNameState = remember { mutableStateOf(prefs.getBoolean("module_repo_sort_name", false)) }
    val listState = rememberLazyListState()

    val offline = !isNetworkAvailable(context)
    val sysNavBarPadding = WindowInsets.navigationBars.asPaddingValues().calculateBottomPadding()

    LaunchedEffect(Unit) {
        if (viewModel.modules.value.isEmpty()) {
            viewModel.refresh()
        }
        if (installedVm.moduleList.isEmpty()) {
            installedVm.fetchModuleList()
        }
    }

    val scrollBehavior = TopAppBarDefaults.pinnedScrollBehavior(rememberTopAppBarState())
    val cs = MaterialTheme.colorScheme
    val isDark = isSystemInDarkTheme()

    val scrim = remember(cs, isDark) {
        Brush.verticalGradient(
            0f to cs.surface.copy(alpha = if (isDark) 0.08f else 0.10f),
            1f to cs.surface.copy(alpha = if (isDark) 0.40f else 0.32f)
        )
    }
    val bannerAlpha = if (isDark) 0.12f else 0.16f

    Scaffold(
        topBar = {
            SearchAppBar(
                title = {
                    Text(
                        text = stringResource(R.string.module_repos),
                        fontWeight = FontWeight.Bold
                    )
                },
                onBackClick = { navigator.popBackStack() },
                searchText = viewModel.search,
                onSearchTextChange = { viewModel.search = it },
                onClearClick = { viewModel.search = TextFieldValue("") },
                scrollBehavior = scrollBehavior,
                dropdownContent = {
                    var showDropdown by remember { mutableStateOf(false) }

                    IconButton(
                        onClick = { showDropdown = true }
                    ) {
                        Icon(
                            imageVector = Icons.Filled.MoreVert,
                            contentDescription = stringResource(id = R.string.settings)
                        )

                        DropdownMenu(expanded = showDropdown, onDismissRequest = {
                            showDropdown = false
                        }) {
                            DropdownMenuItem(
                                text = { Text(stringResource(R.string.module_repos_sort_name)) },
                                trailingIcon = { Checkbox(repoSortByNameState.value, null) },
                                onClick = {
                                    repoSortByNameState.value = !repoSortByNameState.value
                                    prefs.edit {
                                        putBoolean("module_repo_sort_name", repoSortByNameState.value)
                                    }
                                }
                            )
                        }
                    }
                }
            )
        },
        contentWindowInsets = WindowInsets.safeDrawing.only(WindowInsetsSides.Top + WindowInsetsSides.Horizontal)
    ) { innerPadding ->
        val isLoading = viewModel.modules.value.isEmpty()

        if (isLoading) {
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(innerPadding),
                contentAlignment = Alignment.Center
            ) {
                if (offline) {
                    Column(horizontalAlignment = Alignment.CenterHorizontally) {
                        Text(text = stringResource(R.string.network_offline), color = MaterialTheme.colorScheme.outline)
                        Spacer(Modifier.height(12.dp))
                        Button(
                            onClick = { viewModel.refresh() },
                        ) {
                            Text(stringResource(R.string.network_retry))
                        }
                    }
                } else {
                    LoadingIndicator()
                }
            }
        } else {
            val displayModules = run {
                val base = viewModel.filteredModules
                val sortByName = repoSortByNameState.value
                val collator = Collator.getInstance(Locale.getDefault())
                if (!sortByName) base else base.sortedWith(compareBy(collator) { it.moduleName })
            }

            LazyColumn(
                modifier = Modifier
                    .fillMaxSize()
                    .nestedScroll(scrollBehavior.nestedScrollConnection)
                    .padding(innerPadding),
                state = listState,
                verticalArrangement = Arrangement.spacedBy(16.dp),
                contentPadding = PaddingValues(
                    start = 16.dp, top = 16.dp, end = 16.dp, bottom = 16.dp + sysNavBarPadding
                ),
            ) {
                items(displayModules, key = { it.moduleId }) { module ->
                    val latestReleaseTime = remember(module.latestReleaseTime) { module.latestReleaseTime }
                    val moduleAuthor = stringResource(id = R.string.module_author)

                    TonalCard(
                        modifier = Modifier
                            .fillMaxWidth()
                            .clip(RoundedCornerShape(14.dp))
                    ) {
                        Box(
                            modifier = Modifier
                                .fillMaxWidth()
                                .heightIn(min = 170.dp)
                                .clickable {
                                    val args = RepoModuleArg(
                                        moduleId = module.moduleId,
                                        moduleName = module.moduleName,
                                        authors = module.authors,
                                        description = module.summary,
                                        repoUrl = module.repoUrl ?: "",
                                        license = module.license ?: "",
                                        bannerUrl = module.bannerUrl,
                                        repoType = module.repoType,
                                        authorsList = module.authorList.map { AuthorArg(it.name, it.link) },
                                        latestRelease = module.latestRelease,
                                        latestReleaseTime = module.latestReleaseTime,
                                        releases = emptyList()
                                    )
                                    navigator.navigate(ModuleRepoDetailScreenDestination(args)) {
                                        launchSingleTop = true
                                    }
                                }
                        ) {
                            val useBanner = prefs.getBoolean("use_banner", true)

                            if (useBanner && !module.bannerUrl.isNullOrEmpty()) {
                                val req = remember(module.bannerUrl) {
                                    ImageRequest.Builder(context)
                                        .data(module.bannerUrl)
                                        .crossfade(true)
                                        .build()
                                }
                                AsyncImage(
                                    model = req,
                                    contentDescription = null,
                                    modifier = Modifier.matchParentSize(),
                                    contentScale = ContentScale.Crop,
                                    alpha = bannerAlpha
                                )
                            } else {
                                Box(
                                    modifier = Modifier
                                        .matchParentSize()
                                        .background(cs.surfaceContainerLow)
                                )
                            }

                            Box(
                                modifier = Modifier
                                    .matchParentSize()
                                    .background(scrim)
                            )

                            Column(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .padding(start = 22.dp, top = 22.dp, end = 22.dp, bottom = 12.dp)
                            ) {
                                Row(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .padding(bottom = 12.dp)
                                        .zIndex(3f),
                                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    val typeStr = module.repoType
                                    val typeColor = when (typeStr.uppercase()) {
                                        "META" -> MaterialTheme.colorScheme.tertiaryContainer
                                        "NON-FREE" -> MaterialTheme.colorScheme.errorContainer
                                        else -> MaterialTheme.colorScheme.primaryContainer
                                    }
                                    val typeTextColor = when (typeStr.uppercase()) {
                                        "META" -> MaterialTheme.colorScheme.onTertiaryContainer
                                        "NON-FREE" -> MaterialTheme.colorScheme.onErrorContainer
                                        else -> MaterialTheme.colorScheme.onPrimaryContainer
                                    }

                                    BadgeChipCustom(
                                        text = typeStr,
                                        containerColor = typeColor,
                                        contentColor = typeTextColor
                                    )

                                    if (!module.license.isNullOrEmpty()) {
                                        BadgeChipCustom(
                                            text = module.license,
                                            containerColor = MaterialTheme.colorScheme.surfaceContainerHigh.copy(alpha = 0.8f),
                                            contentColor = MaterialTheme.colorScheme.onSurface
                                        )
                                    }
                                }

                                if (module.moduleName.isNotEmpty()) {
                                    Text(
                                        text = module.moduleName,
                                        color = cs.onSurface,
                                        fontSize = MaterialTheme.typography.titleMedium.fontSize,
                                        fontWeight = FontWeight.SemiBold,
                                        lineHeight = MaterialTheme.typography.bodySmall.lineHeight,
                                        fontFamily = MaterialTheme.typography.titleMedium.fontFamily,
                                    )
                                }
                                if (module.moduleId.isNotEmpty()) {
                                    Text(
                                        text = "ID: ${module.moduleId}",
                                        color = cs.onSurface.copy(alpha = 0.78f),
                                        fontSize = MaterialTheme.typography.bodySmall.fontSize,
                                        lineHeight = MaterialTheme.typography.bodySmall.lineHeight,
                                        fontFamily = MaterialTheme.typography.bodySmall.fontFamily,
                                        fontWeight = FontWeight.Medium,
                                    )
                                }
                                Text(
                                    text = "$moduleAuthor: ${module.authors}",
                                    color = cs.onSurface.copy(alpha = 0.78f),
                                    fontSize = MaterialTheme.typography.bodySmall.fontSize,
                                    lineHeight = MaterialTheme.typography.bodySmall.lineHeight,
                                    fontFamily = MaterialTheme.typography.bodySmall.fontFamily,
                                    fontWeight = FontWeight.Medium,
                                )
                                if (module.summary.isNotEmpty()) {
                                    Spacer(modifier = Modifier.height(12.dp))
                                    Text(
                                        text = module.summary,
                                        color = cs.onSurface.copy(alpha = 0.80f),
                                        fontSize = MaterialTheme.typography.bodySmall.fontSize,
                                        fontFamily = MaterialTheme.typography.bodySmall.fontFamily,
                                        lineHeight = MaterialTheme.typography.bodySmall.lineHeight,
                                        fontWeight = FontWeight.Medium,
                                        overflow = TextOverflow.Ellipsis,
                                        maxLines = 4,
                                    )
                                }

                                Spacer(modifier = Modifier.height(16.dp))

                                Row(
                                    horizontalArrangement = Arrangement.SpaceBetween,
                                    verticalAlignment = Alignment.CenterVertically,
                                    modifier = Modifier.fillMaxWidth()
                                ) {
                                    if (module.stargazerCount > 0) {
                                        Row(verticalAlignment = Alignment.CenterVertically) {
                                            Icon(
                                                imageVector = Icons.Rounded.Star,
                                                contentDescription = "stars",
                                                tint = cs.onSurface.copy(alpha = 0.78f),
                                                modifier = Modifier.size(16.dp)
                                            )
                                            Text(
                                                text = module.stargazerCount.toString(),
                                                color = cs.onSurface.copy(alpha = 0.78f),
                                                style = MaterialTheme.typography.bodySmall,
                                                fontWeight = FontWeight.Medium,
                                                modifier = Modifier.padding(start = 4.dp)
                                            )
                                        }
                                    } else {
                                        Spacer(modifier = Modifier.width(1.dp))
                                    }
                                    Spacer(Modifier.weight(1f))
                                    if (latestReleaseTime.isNotEmpty()) {
                                        Text(
                                            text = latestReleaseTime,
                                            color = cs.onSurface.copy(alpha = 0.78f),
                                            fontSize = MaterialTheme.typography.bodySmall.fontSize,
                                            fontFamily = MaterialTheme.typography.bodySmall.fontFamily,
                                            lineHeight = MaterialTheme.typography.bodySmall.lineHeight,
                                            fontWeight = FontWeight.Medium,
                                        )
                                    }
                                }
                            }
                        }
                    }
                }
                item { Spacer(Modifier.height(12.dp)) }
            }
        }
    }
}

@SuppressLint("StringFormatInvalid", "DefaultLocale")
@OptIn(ExperimentalMaterial3Api::class, ExperimentalMaterial3ExpressiveApi::class)
@Composable
@Destination<RootGraph>
fun ModuleRepoDetailScreen(
    navigator: DestinationsNavigator,
    module: RepoModuleArg
) {
    val context = LocalContext.current
    val uriHandler = LocalUriHandler.current
    val scope = rememberCoroutineScope()
    val confirmTitle = stringResource(R.string.module_install)
    var pendingDownload by remember { mutableStateOf<(() -> Unit)?>(null) }
    val confirmDialog = rememberConfirmDialog(onConfirm = { pendingDownload?.invoke() })
    val onInstallModule: (Uri) -> Unit = { uri ->
        navigator.navigate(FlashScreenDestination(FlashIt.FlashModules(listOf(uri)))) {
            launchSingleTop = true
        }
    }

    var readmeHtml by remember(module.moduleId) { mutableStateOf<String?>(null) }
    var readmeLoaded by remember(module.moduleId) { mutableStateOf(false) }
    var detailReleases by remember(module.moduleId) { mutableStateOf<List<ReleaseArg>>(emptyList()) }
    var webUrl by remember(module.moduleId) { mutableStateOf(module.repoUrl) }
    var sourceUrl by remember(module.moduleId) { mutableStateOf(module.repoUrl) }

    val scrollBehavior = TopAppBarDefaults.pinnedScrollBehavior(rememberTopAppBarState())

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text(text = module.moduleName) },
                navigationIcon = {
                    IconButton(onClick = { navigator.popBackStack() }) {
                        Icon(
                            imageVector = Icons.AutoMirrored.Filled.ArrowBack,
                            contentDescription = null,
                        )
                    }
                },
                actions = {
                    if (webUrl.isNotEmpty()) {
                        IconButton(onClick = { uriHandler.openUri(webUrl) }) {
                            Icon(
                                imageVector = Icons.AutoMirrored.Outlined.ChromeReaderMode,
                                contentDescription = null,
                            )
                        }
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor =  MaterialTheme.colorScheme.surfaceContainer,
                )
            )
        },
        contentWindowInsets = WindowInsets.systemBars.add(WindowInsets.displayCutout).only(WindowInsetsSides.Horizontal),
    ) { innerPadding ->
        LaunchedEffect(module.moduleId) {
            if (module.moduleId.isNotEmpty()) {
                withContext(Dispatchers.IO) {
                    runCatching {
                        val detail = fetchModuleDetail(module.moduleId)
                        if (detail != null && detail.readmeHtml.isNotEmpty()) {
                            readmeHtml = detail.readmeHtml
                            if (detail.sourceUrl.isNotEmpty()) sourceUrl = detail.sourceUrl
                            detailReleases = detail.releases.map { r ->
                                ReleaseArg(
                                    tagName = r.tagName,
                                    name = r.name,
                                    publishedAt = r.publishedAt,
                                    assets = r.assets.map { a -> ReleaseAssetArg(a.name, a.downloadUrl, a.size, a.downloadCount) },
                                    descriptionHTML = r.descriptionHTML
                                )
                            }
                        } else {
                            val (fetchedReadmeHtml, fetchedReleases) = fetchGitHubDetails(module.repoUrl)
                            readmeHtml = fetchedReadmeHtml
                            detailReleases = fetchedReleases
                        }
                    }.onSuccess {
                        readmeLoaded = true
                    }.onFailure {
                        readmeLoaded = true
                        detailReleases = emptyList()
                    }
                }
            } else {
                readmeLoaded = true
            }
        }
        val tabs = listOf(
            stringResource(R.string.tab_readme),
            stringResource(R.string.tab_releases),
            stringResource(R.string.tab_info)
        )
        val pagerState = rememberPagerState(initialPage = 0, pageCount = { tabs.size })
        val layoutDirection = LocalLayoutDirection.current
        Box(modifier = Modifier.fillMaxSize()) {
            HorizontalPager(
                state = pagerState,
                modifier = Modifier.fillMaxSize()
            ) { page ->
                val paddedInnerPadding = PaddingValues(
                    top = innerPadding.calculateTopPadding() + 56.dp + 8.dp,
                    start = innerPadding.calculateStartPadding(layoutDirection),
                    end = innerPadding.calculateEndPadding(layoutDirection),
                    bottom = innerPadding.calculateBottomPadding() + WindowInsets.navigationBars.asPaddingValues().calculateBottomPadding() + 16.dp
                )
                when (page) {
                    0 -> ReadmePage(
                        readmeHtml = readmeHtml,
                        readmeLoaded = readmeLoaded,
                        innerPadding = paddedInnerPadding,
                        scrollBehavior = scrollBehavior
                    )
                    1 -> ReleasesPage(
                        detailReleases = detailReleases,
                        innerPadding = paddedInnerPadding,
                        scrollBehavior = scrollBehavior,
                        confirmTitle = confirmTitle,
                        confirmDialog = confirmDialog,
                        scope = scope,
                        onInstallModule = onInstallModule,
                        setPendingDownload = { pendingDownload = it }
                    )
                    2 -> InfoPage(
                        module = module,
                        innerPadding = paddedInnerPadding,
                        scrollBehavior = scrollBehavior,
                        uriHandler = uriHandler,
                        sourceUrl = sourceUrl
                    )
                }
            }
            PrimaryTabRow(
                selectedTabIndex = pagerState.currentPage,
                containerColor = MaterialTheme.colorScheme.surfaceContainer,
                modifier = Modifier.padding(top = innerPadding.calculateTopPadding()),
            ) {
                tabs.forEachIndexed { index, tab ->
                    Tab(
                        selected = pagerState.currentPage == index,
                        onClick = { scope.launch { pagerState.animateScrollToPage(index) } },
                        text = { Text(tab) },
                        unselectedContentColor = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }
        }
        DownloadListener(context) { uri -> onInstallModule(uri) }
    }
}

@OptIn(ExperimentalMaterial3Api::class, ExperimentalMaterial3ExpressiveApi::class)
@Composable
private fun ReadmePage(
    readmeHtml: String?,
    readmeLoaded: Boolean,
    innerPadding: PaddingValues,
    scrollBehavior: TopAppBarScrollBehavior
) {
    Box(modifier = Modifier.fillMaxSize()) {
        if (readmeLoaded) {
            val layoutDirection = LocalLayoutDirection.current
            LazyColumn(
                modifier = Modifier
                    .fillMaxSize()
                    .nestedScroll(scrollBehavior.nestedScrollConnection),
                contentPadding = PaddingValues(
                    top = innerPadding.calculateTopPadding(),
                    start = innerPadding.calculateStartPadding(layoutDirection),
                    end = innerPadding.calculateEndPadding(layoutDirection),
                    bottom = innerPadding.calculateBottomPadding(),
                ),
                verticalArrangement = Arrangement.spacedBy(16.dp)
            ) {
                item {
                    Column(modifier = Modifier.padding(vertical = 8.dp, horizontal = 16.dp)) {
                        GithubMarkdown(
                            content = readmeHtml ?: "<p>Readme is not available for this module.</p>",
                            containerColor = MaterialTheme.colorScheme.background
                        )
                    }
                }
            }
        } else {
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(innerPadding),
                contentAlignment = Alignment.Center
            ) {
                CircularProgressIndicator()
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class, ExperimentalMaterial3ExpressiveApi::class)
@SuppressLint("DefaultLocale")
@Composable
fun ReleasesPage(
    detailReleases: List<ReleaseArg>,
    innerPadding: PaddingValues,
    scrollBehavior: TopAppBarScrollBehavior,
    confirmTitle: String,
    confirmDialog: ConfirmDialogHandle,
    scope: CoroutineScope,
    onInstallModule: (Uri) -> Unit,
    setPendingDownload: ((() -> Unit)) -> Unit,
) {
    val layoutDirection = LocalLayoutDirection.current
    LazyColumn(
        modifier = Modifier
            .fillMaxSize()
            .nestedScroll(scrollBehavior.nestedScrollConnection),
        contentPadding = PaddingValues(
            top = innerPadding.calculateTopPadding(),
            start = innerPadding.calculateStartPadding(layoutDirection) + 16.dp,
            end = innerPadding.calculateEndPadding(layoutDirection) + 16.dp,
            bottom = innerPadding.calculateBottomPadding(),
        ),
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        if (detailReleases.isNotEmpty()) {
            items(
                items = detailReleases,
                key = { it.tagName },
            ) { rel ->
                val title = remember(rel.name, rel.tagName) { rel.name.ifBlank { rel.tagName } }

                TonalCard(
                    modifier = Modifier.fillMaxWidth().clip(RoundedCornerShape(14.dp))
                ) {
                    Column(
                        modifier = Modifier.padding(vertical = 18.dp, horizontal = 22.dp)
                    ) {
                        Row(
                            verticalAlignment = Alignment.CenterVertically,
                            modifier = Modifier.fillMaxWidth()
                        ) {
                            Column(modifier = Modifier.weight(1f)) {
                                Text(
                                    text = title,
                                    style = MaterialTheme.typography.titleMedium,
                                    fontWeight = FontWeight.SemiBold,
                                    color = MaterialTheme.colorScheme.onSurface
                                )
                                Text(
                                    text = rel.tagName,
                                    style = MaterialTheme.typography.bodySmall,
                                    color = MaterialTheme.colorScheme.outline,
                                    modifier = Modifier.padding(top = 2.dp)
                                )
                            }
                            Text(
                                text = rel.publishedAt,
                                style = MaterialTheme.typography.bodySmall,
                                color = MaterialTheme.colorScheme.outline,
                                modifier = Modifier.align(Alignment.Top)
                            )
                        }

                        AnimatedVisibility(
                            visible = rel.descriptionHTML.isNotEmpty(),
                            enter = fadeIn() + expandVertically(animationSpec = spring(stiffness = Spring.StiffnessLow)),
                            exit = fadeOut() + shrinkVertically(animationSpec = spring(stiffness = Spring.StiffnessLow))
                        ) {
                            Column {
                                HorizontalDivider(
                                    modifier = Modifier.padding(vertical = 8.dp),
                                    thickness = Dp.Hairline,
                                    color = MaterialTheme.colorScheme.outline.copy(alpha = 0.3f)
                                )
                                GithubMarkdown(content = rel.descriptionHTML, containerColor = Color.Transparent)
                            }
                        }

                        AnimatedVisibility(
                            visible = rel.assets.isNotEmpty(),
                            enter = fadeIn() + expandVertically(animationSpec = spring(stiffness = Spring.StiffnessLow)),
                            exit = fadeOut() + shrinkVertically(animationSpec = spring(stiffness = Spring.StiffnessLow))
                        ) {
                            Column {
                                HorizontalDivider(
                                    modifier = Modifier.padding(vertical = 8.dp),
                                    thickness = Dp.Hairline,
                                    color = MaterialTheme.colorScheme.outline.copy(alpha = 0.3f)
                                )

                                rel.assets.forEachIndexed { index, asset ->
                                    val fileName = asset.name
                                    val sizeText = remember(asset.size) {
                                        val s = asset.size
                                        when {
                                            s >= 1024L * 1024L * 1024L -> String.format("%.1f GB", s / (1024f * 1024f * 1024f))
                                            s >= 1024L * 1024L -> String.format("%.1f MB", s / (1024f * 1024f))
                                            s >= 1024L -> String.format("%.0f KB", s / 1024f)
                                            else -> "$s B"
                                        }
                                    }
                                    val sizeAndDownloads =
                                        remember(sizeText, asset.downloadCount) { "$sizeText • ${asset.downloadCount} Downloads" }
                                    var isDownloading by remember(fileName, asset.downloadUrl) { mutableStateOf(false) }

                                    val onClickDownload = remember(fileName, asset.downloadUrl) {
                                        {
                                            setPendingDownload {
                                                isDownloading = true
                                                scope.launch(Dispatchers.IO) {
                                                    download(
                                                        asset.downloadUrl,
                                                        fileName,
                                                        { uri ->
                                                            isDownloading = false
                                                            onInstallModule(uri)
                                                        },
                                                        { isDownloading = true },
                                                        { progress -> }
                                                    )
                                                }
                                            }
                                            confirmDialog.showConfirm(title = confirmTitle, content = "Install $fileName?")
                                        }
                                    }
                                    Row(
                                        modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp),
                                        verticalAlignment = Alignment.CenterVertically,
                                        horizontalArrangement = Arrangement.spacedBy(8.dp)
                                    ) {
                                        Column(modifier = Modifier.weight(1f)) {
                                            Text(
                                                text = fileName,
                                                style = MaterialTheme.typography.bodyMedium,
                                                color = MaterialTheme.colorScheme.onSurface
                                            )
                                            Text(
                                                text = sizeAndDownloads,
                                                style = MaterialTheme.typography.bodySmall,
                                                color = MaterialTheme.colorScheme.outline,
                                                modifier = Modifier.padding(top = 2.dp)
                                            )
                                        }
                                        FilledTonalButton(
                                            onClick = onClickDownload,
                                            contentPadding = ButtonDefaults.TextButtonContentPadding
                                        ) {
                                            if (isDownloading) {
                                                CircularWavyProgressIndicator(modifier = Modifier.size(20.dp))
                                            } else {
                                                Icon(
                                                    modifier = Modifier.size(20.dp),
                                                    imageVector = Icons.Outlined.Download,
                                                    contentDescription = "Install"
                                                )
                                                Text(
                                                    modifier = Modifier.padding(start = 7.dp),
                                                    text = "Install",
                                                    style = MaterialTheme.typography.labelMedium,
                                                )
                                            }
                                        }
                                    }
                                    if (index < rel.assets.lastIndex) {
                                        HorizontalDivider(
                                            modifier = Modifier.padding(vertical = 4.dp),
                                            thickness = Dp.Hairline,
                                            color = MaterialTheme.colorScheme.outline.copy(alpha = 0.2f)
                                        )
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun InfoPage(
    module: RepoModuleArg,
    innerPadding: PaddingValues,
    scrollBehavior: TopAppBarScrollBehavior,
    uriHandler: UriHandler,
    sourceUrl: String,
) {
    val layoutDirection = LocalLayoutDirection.current
    LazyColumn(
        modifier = Modifier
            .fillMaxSize()
            .nestedScroll(scrollBehavior.nestedScrollConnection),
        contentPadding = PaddingValues(
            top = innerPadding.calculateTopPadding(),
            start = innerPadding.calculateStartPadding(layoutDirection),
            end = innerPadding.calculateEndPadding(layoutDirection),
            bottom = innerPadding.calculateBottomPadding(),
        ),
    ) {
        if (module.authorsList.isNotEmpty()) {
            item {
                val authorsItems: List<@Composable () -> Unit> = module.authorsList.map { author ->
                    @Composable {
                        ExpressiveListItem(
                            headlineContent = {
                                Text(
                                    text = author.name,
                                    fontSize = MaterialTheme.typography.bodySmall.fontSize,
                                    lineHeight = MaterialTheme.typography.bodySmall.lineHeight,
                                    fontFamily = MaterialTheme.typography.bodySmall.fontFamily
                                )
                            },
                            trailingContent = {
                                FilledTonalButton(
                                    modifier = Modifier.defaultMinSize(52.dp, 32.dp),
                                    onClick = { uriHandler.openUri(author.link) },
                                    contentPadding = ButtonDefaults.TextButtonContentPadding
                                ) {
                                    Icon(
                                        modifier = Modifier.size(20.dp),
                                        imageVector = Icons.Outlined.Link,
                                        contentDescription = null
                                    )
                                }
                            }
                        )
                    }
                }
                ExpressiveList(
                    modifier = Modifier.padding(horizontal = 16.dp).padding(bottom = 8.dp),
                    title = stringResource(R.string.module_author),
                    content = authorsItems
                )
            }
        }
        if (sourceUrl.isNotEmpty()) {
            item {
                val sourceCodeItems: List<@Composable () -> Unit> = listOf(
                    @Composable {
                        ExpressiveListItem(
                            headlineContent = {
                                Text(
                                    text = sourceUrl,
                                    fontSize = MaterialTheme.typography.bodySmall.fontSize,
                                    lineHeight = MaterialTheme.typography.bodySmall.lineHeight,
                                    fontFamily = MaterialTheme.typography.bodySmall.fontFamily
                                )
                            },
                            trailingContent = {
                                FilledTonalButton(
                                    modifier = Modifier.defaultMinSize(52.dp, 32.dp),
                                    onClick = { uriHandler.openUri(sourceUrl) },
                                    contentPadding = ButtonDefaults.TextButtonContentPadding
                                ) {
                                    Icon(
                                        modifier = Modifier.size(20.dp),
                                        imageVector = Icons.Outlined.Link,
                                        contentDescription = null
                                    )
                                }
                            }
                        )
                    }
                )
                ExpressiveList(
                    modifier = Modifier.padding(horizontal = 16.dp, vertical = 8.dp),
                    title = "Source Code",
                    content = sourceCodeItems
                )
            }
        }
    }
}

private fun fetchGitHubDetails(repoUrl: String): Pair<String?, List<ReleaseArg>> {
    if (repoUrl.isBlank() || !repoUrl.contains("github.com")) return Pair(null, emptyList())
    val repoPath = repoUrl.trimEnd('/').substringAfter("github.com/")
    if (repoPath.isBlank() || !repoPath.contains("/")) return Pair(null, emptyList())

    var readmeHtml: String? = null
    try {
        val readmeApiUrl = "https://api.github.com/repos/$repoPath/readme"
        val conn = URL(readmeApiUrl).openConnection() as java.net.HttpURLConnection
        conn.setRequestProperty("Accept", "application/vnd.github.v3.html")
        conn.setRequestProperty("User-Agent", "KernelSU-Manager")
        if (conn.responseCode == 200) {
            readmeHtml = BufferedReader(InputStreamReader(conn.inputStream)).use { it.readText() }
        }
    } catch(e: Exception) {
        e.printStackTrace()
    }

    val releasesList = mutableListOf<ReleaseArg>()
    val releasesApiUrl = "https://api.github.com/repos/$repoPath/releases"
    try {
        val conn = URL(releasesApiUrl).openConnection() as java.net.HttpURLConnection
        conn.setRequestProperty("Accept", "application/vnd.github.v3.html+json")
        conn.setRequestProperty("User-Agent", "KernelSU-Manager")
        if (conn.responseCode == 200) {
            val response = BufferedReader(InputStreamReader(conn.inputStream)).use { it.readText() }
            val jsonArray = JSONArray(response)
            for (i in 0 until jsonArray.length()) {
                val relObj = jsonArray.getJSONObject(i)
                val tagName = relObj.optString("tag_name", "")
                val name = relObj.optString("name", tagName)
                val publishedAt = relObj.optString("published_at", "").substringBefore("T")

                val bodyHtml = relObj.optString("body_html", relObj.optString("body", ""))

                val assetsArray = relObj.optJSONArray("assets")
                val assetsList = mutableListOf<ReleaseAssetArg>()
                if (assetsArray != null) {
                    for (j in 0 until assetsArray.length()) {
                        val assetObj = assetsArray.getJSONObject(j)
                        val assetName = assetObj.optString("name", "")
                        val downloadUrl = assetObj.optString("browser_download_url", "")
                        val size = assetObj.optLong("size", 0L)
                        val downloadCount = assetObj.optInt("download_count", 0)
                        if (assetName.endsWith(".zip")) {
                            assetsList.add(ReleaseAssetArg(assetName, downloadUrl, size, downloadCount))
                        }
                    }
                }
                releasesList.add(ReleaseArg(tagName, name, publishedAt, assetsList, bodyHtml))
            }
        }
    } catch(e: Exception) {
        e.printStackTrace()
    }

    return Pair(readmeHtml, releasesList)
}

@Composable
fun BadgeChipCustom(
    text: String,
    containerColor: Color,
    contentColor: Color
) {
    Surface(
        color = containerColor,
        shape = RoundedCornerShape(6.dp),
    ) {
        Text(
            text = text.uppercase(),
            style = MaterialTheme.typography.labelSmall,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.padding(horizontal = 8.dp, vertical = 4.dp),
            color = contentColor
        )
    }
}