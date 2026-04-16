package me.weishu.kernelsu.ui.screen

import android.content.Context
import android.os.Build
import android.os.Build.VERSION.SDK_INT
import android.system.Os
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.animateContentSize
import androidx.compose.animation.expandVertically
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.shrinkVertically
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.only
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawing
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Archive
import androidx.compose.material.icons.outlined.Block
import androidx.compose.material.icons.outlined.CheckCircle
import androidx.compose.material.icons.outlined.Warning
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.material3.TopAppBarScrollBehavior
import androidx.compose.material3.rememberTopAppBarState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.produceState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalUriHandler
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.surfaceColorAtElevation
import androidx.compose.runtime.remember
import androidx.compose.ui.graphics.Shape
import androidx.core.content.pm.PackageInfoCompat
import com.ramcosta.composedestinations.annotation.Destination
import com.ramcosta.composedestinations.annotation.RootGraph
import com.ramcosta.composedestinations.generated.NavGraphs
import com.ramcosta.composedestinations.generated.destinations.InstallScreenDestination
import com.ramcosta.composedestinations.generated.destinations.ModuleScreenDestination
import com.ramcosta.composedestinations.generated.destinations.SuperUserScreenDestination
import com.ramcosta.composedestinations.navigation.DestinationsNavigator
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.offset
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.draw.clip
import androidx.compose.ui.res.painterResource
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.size
import androidx.compose.ui.draw.blur
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.animation.core.spring
import androidx.compose.animation.core.Spring
import androidx.compose.ui.graphics.painter.Painter
import androidx.compose.material.icons.filled.Android
import androidx.compose.material.icons.filled.AutoAwesomeMotion
import androidx.compose.material.icons.filled.Extension
import androidx.compose.material.icons.filled.Fingerprint
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material.icons.filled.KeyboardCommandKey
import androidx.compose.material.icons.filled.Memory
import androidx.compose.material.icons.filled.Security
import androidx.compose.ui.draw.rotate
import androidx.compose.animation.core.tween
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.unit.IntSize
import androidx.compose.animation.core.VisibilityThreshold
import androidx.compose.foundation.layout.IntrinsicSize
import androidx.compose.foundation.layout.asPaddingValues
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.navigationBars
import me.weishu.kernelsu.ui.util.getHeaderImage
import coil.compose.AsyncImage
import coil.request.ImageRequest
import coil.ImageLoader
import coil.decode.GifDecoder
import coil.decode.ImageDecoderDecoder
import me.weishu.kernelsu.KernelVersion
import me.weishu.kernelsu.Natives
import me.weishu.kernelsu.R
import me.weishu.kernelsu.getKernelVersion
import me.weishu.kernelsu.ui.component.RebootListPopup
import me.weishu.kernelsu.ui.component.rememberConfirmDialog
import me.weishu.kernelsu.ui.util.checkNewVersion
import me.weishu.kernelsu.ui.util.getLayoutStyle
import me.weishu.kernelsu.ui.util.getModuleCount
import me.weishu.kernelsu.ui.util.getSELinuxStatus
import me.weishu.kernelsu.ui.util.getSuperuserCount
import me.weishu.kernelsu.ui.util.module.LatestVersionInfo
import me.weishu.kernelsu.ui.util.rootAvailable

@OptIn(ExperimentalMaterial3Api::class)
@Destination<RootGraph>(start = true)
@Composable
fun HomeScreen(navigator: DestinationsNavigator) {
    val context = LocalContext.current
    val kernelVersion = getKernelVersion()
    val scrollBehavior = TopAppBarDefaults.pinnedScrollBehavior(rememberTopAppBarState())
    val isFloating = LocalContext.current.getSharedPreferences("settings", Context.MODE_PRIVATE).getBoolean("enable_floating_navbar", false)
    val sysNavBarPadding = WindowInsets.navigationBars.asPaddingValues().calculateBottomPadding()
    val floatingPadding = if (isFloating) sysNavBarPadding + 96.dp else 0.dp

    Scaffold(
        topBar = { TopBar(scrollBehavior = scrollBehavior) },
        contentWindowInsets = WindowInsets.safeDrawing.only(WindowInsetsSides.Top + WindowInsetsSides.Horizontal)
    ) { innerPadding ->
        Column(
            modifier = Modifier
                .padding(innerPadding)
                .nestedScroll(scrollBehavior.nestedScrollConnection)
                .verticalScroll(rememberScrollState())
                .padding(start = 16.dp, end = 16.dp, top = 16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            val isManager = Natives.isManager
            val ksuVersion = if (isManager) Natives.version else null
            val lkmMode = ksuVersion?.let {
                if (kernelVersion.isGKI()) Natives.isLkmMode else null
            }
            val fullFeatured = isManager && !Natives.requireNewKernel() && rootAvailable()

            StatusCard(
                kernelVersion,
                ksuVersion,
                lkmMode,
                fullFeatured,
                useClassicLayout = context.getLayoutStyle(),
                onClickInstall = { navigator.navigate(InstallScreenDestination) },
                onClickSuperuser = {
                    navigator.navigate(SuperUserScreenDestination) {
                        popUpTo(NavGraphs.root) {
                            saveState = true
                        }
                        launchSingleTop = true
                        restoreState = true
                    }
                },
                onclickModule = {
                    navigator.navigate(ModuleScreenDestination) {
                        popUpTo(NavGraphs.root) {
                            saveState = true
                        }
                        launchSingleTop = true
                        restoreState = true
                    }
                }
            )
//            if (ksuVersion != null && !Natives.isLkmMode) {
//                WarningCard(stringResource(id = R.string.home_gki_warning))
//            }
            if (isManager && Natives.requireNewKernel()) {
                WarningCard(
                    stringResource(id = R.string.require_kernel_version).format(
                        ksuVersion, Natives.MINIMAL_SUPPORTED_KERNEL
                    )
                )
            }
            if (ksuVersion != null && !rootAvailable()) {
                WarningCard(
                    stringResource(id = R.string.grant_root_failed)
                )
            }
//            val checkUpdate =
//                LocalContext.current.getSharedPreferences("settings", Context.MODE_PRIVATE)
//                    .getBoolean("check_update", true)
//            if (checkUpdate) {
//                UpdateCard()
//            }
            InfoCard()
            DonateCard()
            LearnMoreCard()
            Spacer(modifier = Modifier.height(floatingPadding))
        }
    }
}

@Composable
fun UpdateCard() {
    val context = LocalContext.current
    val latestVersionInfo = LatestVersionInfo()
    val newVersion by produceState(initialValue = latestVersionInfo) {
        value = withContext(Dispatchers.IO) {
            checkNewVersion()
        }
    }

    val currentVersionCode = getManagerVersion(context).second
    val newVersionCode = newVersion.versionCode
    val newVersionUrl = newVersion.downloadUrl
    val changelog = newVersion.changelog

    val uriHandler = LocalUriHandler.current
    val title = stringResource(id = R.string.module_changelog)
    val updateText = stringResource(id = R.string.module_update)

    AnimatedVisibility(
        visible = newVersionCode > currentVersionCode,
        enter = fadeIn() + expandVertically(),
        exit = shrinkVertically() + fadeOut()
    ) {
        val updateDialog = rememberConfirmDialog(onConfirm = { uriHandler.openUri(newVersionUrl) })
        WarningCard(
            message = stringResource(id = R.string.new_version_available).format(newVersionCode),
            MaterialTheme.colorScheme.outlineVariant
        ) {
            if (changelog.isEmpty()) {
                uriHandler.openUri(newVersionUrl)
            } else {
                updateDialog.showConfirm(
                    title = title,
                    content = changelog,
                    markdown = true,
                    confirm = updateText
                )
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun TopBar(
    scrollBehavior: TopAppBarScrollBehavior? = null
) {
    val context = LocalContext.current
    val prefs = context.getSharedPreferences("settings", Context.MODE_PRIVATE)
    val isOfficialEnabled = prefs.getBoolean("enable_official_launcher", false)
    val appNameId = if (isOfficialEnabled) R.string.app_name else R.string.app_name_mambo
    val managerVersion = getManagerVersion(context)

    TopAppBar(
        title = {
            Column(verticalArrangement = Arrangement.Center) {
                Text(
                    text = stringResource(appNameId),
                    fontWeight = FontWeight.Bold,
                    style = MaterialTheme.typography.titleLarge
                )
                Text(
                    text = "${managerVersion.first} (${managerVersion.second})",
                    style = MaterialTheme.typography.labelMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        },
        actions = { RebootListPopup() },
        windowInsets = WindowInsets.safeDrawing.only(WindowInsetsSides.Top + WindowInsetsSides.Horizontal),
        scrollBehavior = scrollBehavior
    )
}

@Composable
private fun StatusCard(
    kernelVersion: KernelVersion,
    ksuVersion: Int?,
    lkmMode: Boolean?,
    fullFeatured: Boolean?,
    useClassicLayout: Boolean,
    onClickInstall: () -> Unit = {},
    onClickSuperuser: () -> Unit = {},
    onclickModule: () -> Unit = {},
) {
    val context = LocalContext.current
    val cs = MaterialTheme.colorScheme
    
    val workingMode = when (lkmMode) {
        null -> "Legacy"
        true -> "LKM"
        else -> "GKI"
    }

    val statusText = if (ksuVersion != null)
        stringResource(R.string.home_working)
    else
        stringResource(R.string.home_not_installed)

    val titleStyle = if (useClassicLayout) MaterialTheme.typography.titleLarge else MaterialTheme.typography.titleMedium
    val bodyStyle = if (useClassicLayout) MaterialTheme.typography.bodyMedium else MaterialTheme.typography.bodySmall

    val headerImageUri = context.getHeaderImage()
    val imageLoader = remember(context) {
        ImageLoader.Builder(context)
            .components {
                if (SDK_INT >= 28) {
                    add(ImageDecoderDecoder.Factory())
                } else {
                    add(GifDecoder.Factory())
                }
            }
            .build()
    }

    val headerCardContent = @Composable { modifier: Modifier ->
        TonalCard(
            containerColor = cs.secondaryContainer,
            modifier = modifier.clip(RoundedCornerShape(24.dp))
        ) {
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .clickable { onClickInstall() }
            ) {
                if (headerImageUri != null) {
                    AsyncImage(
                        model = ImageRequest.Builder(context)
                            .data(headerImageUri)
                            .crossfade(true)
                            .build(),
                        imageLoader = imageLoader,
                        contentDescription = null,
                        contentScale = ContentScale.Crop,
                        modifier = Modifier.matchParentSize()
                    )

                    Box(
                        modifier = Modifier
                            .matchParentSize()
                            .background(
                                Brush.verticalGradient(
                                    listOf(Color.Transparent, cs.surface.copy(alpha = 0.8f))
                                )
                            )
                    )
                }

                Icon(
                    imageVector = Icons.Filled.KeyboardCommandKey,
                    contentDescription = null,
                    tint = cs.onSecondaryContainer.copy(alpha = 0.15f),
                    modifier = Modifier
                        .align(Alignment.BottomEnd)
                        .size(140.dp)
                        .offset(x = 16.dp, y = 16.dp)
                )

                if (headerImageUri != null) {
                    Box(
                        modifier = Modifier
                            .matchParentSize()
                            .background(
                                Brush.verticalGradient(
                                    listOf(Color.Transparent, cs.surface.copy(alpha = 0.8f))
                                )
                            )
                    )
                }

                Column(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(16.dp),
                    verticalArrangement = if (useClassicLayout) Arrangement.Bottom else Arrangement.Top,
                    horizontalAlignment = Alignment.Start
                ) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Text(
                            text = statusText,
                            style = titleStyle,
                            fontWeight = FontWeight.Bold,
                            color = cs.onSecondaryContainer
                        )
                        Spacer(Modifier.width(8.dp))

                        Box(
                            modifier = Modifier
                                .background(
                                    color = cs.onSecondaryContainer.copy(alpha = 0.15f),
                                    shape = RoundedCornerShape(6.dp)
                                )
                                .padding(horizontal = 6.dp, vertical = 2.dp)
                        ) {
                            Text(
                                text = workingMode,
                                style = MaterialTheme.typography.labelSmall,
                                color = cs.onSecondaryContainer
                            )
                        }
                    }
                    Spacer(modifier = Modifier.height(4.dp))
                    Text(
                        text = if (ksuVersion != null) "Version: $ksuVersion" else stringResource(R.string.home_click_to_install),
                        style = bodyStyle,
                        color = cs.onSecondaryContainer.copy(alpha = 0.8f)
                    )
                }
            }
        }
    }

    val statsCardsContent = @Composable { modifier: Modifier, isVertical: Boolean ->
        if (fullFeatured == true) {
            @Composable
            fun StatInfoCard(title: String, count: String, icon: ImageVector, onClick: () -> Unit, itemModifier: Modifier) {
                TonalCard(
                    modifier = itemModifier,
                    containerColor = cs.surfaceColorAtElevation(2.dp)
                ) {
                    Box(
                        modifier = Modifier
                            .fillMaxWidth()
                            .clickable { onClick() }
                            .padding(16.dp)
                    ) {
                        Icon(
                            imageVector = icon,
                            contentDescription = null,
                            tint = cs.onSurface.copy(alpha = 0.05f),
                            modifier = Modifier
                                .align(Alignment.BottomEnd)
                                .size(64.dp)
                                .offset(x = 8.dp, y = 8.dp)
                        )
                        Column {
                            Text(
                                text = title,
                                style = MaterialTheme.typography.titleSmall,
                                color = cs.onSurfaceVariant
                            )
                            Spacer(Modifier.height(8.dp))
                            Text(
                                text = count,
                                style = MaterialTheme.typography.headlineMedium,
                                fontWeight = FontWeight.Bold,
                                color = cs.onSurface
                            )
                        }
                    }
                }
            }

            if (isVertical) {
                Column(
                    modifier = modifier,
                    verticalArrangement = Arrangement.spacedBy(16.dp)
                ) {
                    StatInfoCard(
                        title = stringResource(R.string.module),
                        count = getModuleCount().toString(),
                        icon = Icons.Filled.Extension,
                        onClick = onclickModule,
                        itemModifier = Modifier.weight(1f)
                    )
                    StatInfoCard(
                        title = stringResource(R.string.superuser),
                        count = getSuperuserCount().toString(),
                        icon = Icons.Filled.Security,
                        onClick = onClickSuperuser,
                        itemModifier = Modifier.weight(1f)
                    )
                }
            } else {
                Row(
                    modifier = modifier,
                    horizontalArrangement = Arrangement.spacedBy(16.dp)
                ) {
                    StatInfoCard(
                        title = stringResource(R.string.module),
                        count = getModuleCount().toString(),
                        icon = Icons.Filled.Extension,
                        onClick = onclickModule,
                        itemModifier = Modifier.weight(1f)
                    )
                    StatInfoCard(
                        title = stringResource(R.string.superuser),
                        count = getSuperuserCount().toString(),
                        icon = Icons.Filled.Security,
                        onClick = onClickSuperuser,
                        itemModifier = Modifier.weight(1f)
                    )
                }
            }
        }
    }

    if (useClassicLayout) {
        Column(verticalArrangement = Arrangement.spacedBy(16.dp)) {
            headerCardContent(Modifier.fillMaxWidth().height(170.dp))
            statsCardsContent(Modifier.fillMaxWidth(), false)
        }
    } else {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .height(IntrinsicSize.Min),
            horizontalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            headerCardContent(Modifier.weight(0.6f).fillMaxHeight())
            statsCardsContent(Modifier.weight(0.4f).fillMaxHeight(), true)
        }
    }
}

@Composable
fun WarningCard(
    message: String, color: Color = MaterialTheme.colorScheme.error, onClick: (() -> Unit)? = null
) {
    TonalCard(containerColor = color) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .then(onClick?.let { Modifier.clickable { it() } } ?: Modifier)
                .padding(24.dp)
        ) {
            Text(
                text = message, style = MaterialTheme.typography.bodyMedium
            )
        }
    }
}

@Composable
fun TonalCard(
    modifier: Modifier = Modifier,
    containerColor: Color = MaterialTheme.colorScheme.surfaceColorAtElevation(1.dp),
    shape: Shape = RoundedCornerShape(20.dp),
    content: @Composable () -> Unit
) {
    Card(
        modifier = modifier,
        colors = CardDefaults.cardColors(containerColor = containerColor),
        shape = shape
    ) {
        content()
    }
}

@Composable
fun LearnMoreCard() {
    val uriHandler = LocalUriHandler.current
    val url = stringResource(R.string.home_learn_kernelsu_url)

    TonalCard {
        Row(modifier = Modifier
            .fillMaxWidth()
            .clickable {
                uriHandler.openUri(url)
            }
            .padding(24.dp), verticalAlignment = Alignment.CenterVertically) {
            Column {
                Text(
                    text = stringResource(R.string.home_learn_kernelsu),
                    style = MaterialTheme.typography.titleSmall
                )
                Spacer(Modifier.height(4.dp))
                Text(
                    text = stringResource(R.string.home_click_to_learn_kernelsu),
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.outline
                )
            }
        }
    }
}

@Composable
fun DonateCard() {
    val uriHandler = LocalUriHandler.current

    TonalCard {
        Row(modifier = Modifier
            .fillMaxWidth()
            .clickable {
                uriHandler.openUri("https://patreon.com/weishu")
            }
            .padding(24.dp), verticalAlignment = Alignment.CenterVertically) {
            Column {
                Text(
                    text = stringResource(R.string.home_support_title),
                    style = MaterialTheme.typography.titleSmall
                )
                Spacer(Modifier.height(4.dp))
                Text(
                    text = stringResource(R.string.home_support_content),
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.outline
                )
            }
        }
    }
}

@Composable
private fun InfoCard() {
    val context = LocalContext.current
    val prefs = context.getSharedPreferences("settings", Context.MODE_PRIVATE)

    val isManager = Natives.isManager
    val ksuVersion = if (isManager) Natives.version else null

    // State expand
    var expanded by rememberSaveable { mutableStateOf(false) }
    val developerOptionsEnabled = prefs.getBoolean("enable_developer_options", false)

    val arrowRotation by animateFloatAsState(
        targetValue = if (expanded) 180f else 0f,
        animationSpec = spring(stiffness = Spring.StiffnessLow), // Putaran smooth pelan
        label = "arrowRotation"
    )

    TonalCard(
        modifier = Modifier
            .fillMaxWidth()
            .clip(CardDefaults.shape)
            .clickable { expanded = !expanded }
            .animateContentSize(
                animationSpec = spring(
                    dampingRatio = Spring.DampingRatioNoBouncy,
                    stiffness = Spring.StiffnessMediumLow
                )
            )
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(24.dp)
        ) {
            @Composable
            fun InfoCardItem(label: String, content: String, icon: Any? = null) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    if (icon != null) {
                        val modifier = Modifier.padding(end = 20.dp)
                        val tint = MaterialTheme.colorScheme.primary
                        when (icon) {
                            is ImageVector -> Icon(icon, null, modifier, tint = tint)
                            is Painter -> Icon(icon, null, modifier, tint = tint)
                        }
                    }
                    Column {
                        Text(
                            text = label,
                            style = MaterialTheme.typography.titleMedium,
                            fontWeight = FontWeight.SemiBold
                        )
                        Text(
                            text = content,
                            style = MaterialTheme.typography.bodySmall,
                            modifier = Modifier.padding(top = 4.dp),
                            color = MaterialTheme.colorScheme.outline
                        )
                    }
                }
            }

            Column {
                val managerVersion = getManagerVersion(context)
                val uidText = if (developerOptionsEnabled) " | UID: ${android.os.Process.myUid()}" else ""

                InfoCardItem(
                    label = stringResource(R.string.home_manager_version),
                    content = "${managerVersion.first} (${managerVersion.second})$uidText",
                    icon = Icons.Filled.AutoAwesomeMotion,
                )

                Spacer(Modifier.height(16.dp))

                val uname = Os.uname()
                InfoCardItem(
                    label = stringResource(R.string.home_kernel),
                    content = "${uname.release} (${uname.machine})",
                    icon = Icons.Filled.Memory,
                )
            }

            AnimatedVisibility(
                visible = expanded,
                enter = fadeIn(animationSpec = tween(400)) + expandVertically(
                    animationSpec = spring(
                        dampingRatio = Spring.DampingRatioNoBouncy,
                        stiffness = Spring.StiffnessMediumLow
                    )
                ),
                exit = fadeOut(animationSpec = tween(200)) + shrinkVertically(
                    animationSpec = spring(
                        dampingRatio = Spring.DampingRatioNoBouncy,
                        stiffness = Spring.StiffnessMediumLow
                    )
                )
            ) {
                Column {
                    Spacer(Modifier.height(16.dp))

                    InfoCardItem(
                        label = stringResource(R.string.home_android),
                        content = "${Build.VERSION.RELEASE} (SDK ${Build.VERSION.SDK_INT})",
                        icon = Icons.Filled.Android,
                    )

                    Spacer(Modifier.height(16.dp))

                    InfoCardItem(
                        label = stringResource(R.string.home_selinux_status),
                        content = getSELinuxStatus(),
                        icon = Icons.Filled.Security,
                    )

                    Spacer(Modifier.height(16.dp))

                    InfoCardItem(
                        label = stringResource(R.string.home_fingerprint),
                        content = Build.FINGERPRINT,
                        icon = Icons.Filled.Fingerprint,
                    )
                }
            }

            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(top = 16.dp),
                contentAlignment = Alignment.Center
            ) {
                IconButton(
                    onClick = { expanded = !expanded },
                    modifier = Modifier.size(36.dp)
                ) {
                    Icon(
                        imageVector = Icons.Filled.KeyboardArrowDown,
                        contentDescription = "Show more",
                        modifier = Modifier.rotate(arrowRotation) // Rotasi Smooth
                    )
                }
            }
        }
    }
}

fun getManagerVersion(context: Context): Pair<String, Long> {
    val packageInfo = context.packageManager.getPackageInfo(context.packageName, 0)!!
    val versionCode = PackageInfoCompat.getLongVersionCode(packageInfo)
    return Pair(packageInfo.versionName!!, versionCode)
}

@Preview
@Composable
private fun StatusCardPreview() {
    Column(verticalArrangement = Arrangement.spacedBy(16.dp)) {
        // Preview 1: Classic Layout (Working)
        StatusCard(
            kernelVersion = KernelVersion(5, 10, 101),
            ksuVersion = 1,
            lkmMode = null,
            fullFeatured = false,
            useClassicLayout = true // Classic
        )

        // Preview 2: Modern Layout (Working + Full Featured)
        StatusCard(
            kernelVersion = KernelVersion(5, 10, 101),
            ksuVersion = 20000,
            lkmMode = true,
            fullFeatured = true,
            useClassicLayout = false // Modern
        )

        // Preview 3: Classic Layout (Not Installed)
        StatusCard(
            kernelVersion = KernelVersion(5, 10, 101),
            ksuVersion = null,
            lkmMode = true,
            fullFeatured = true,
            useClassicLayout = true // Classic
        )

        // Preview 4: Modern Layout (Unsupported)
        StatusCard(
            kernelVersion = KernelVersion(4, 10, 101),
            ksuVersion = null,
            lkmMode = false,
            fullFeatured = false,
            useClassicLayout = false // Modern
        )
    }
}

@Preview
@Composable
private fun WarningCardPreview() {
    Column {
        WarningCard(message = "Warning message")
        WarningCard(
            message = "Warning message ",
            MaterialTheme.colorScheme.outlineVariant,
            onClick = {})
    }
}