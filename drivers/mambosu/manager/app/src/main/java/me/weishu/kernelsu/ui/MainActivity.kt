package me.weishu.kernelsu.ui

import android.content.SharedPreferences
import android.content.res.Configuration
import android.net.Uri
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.BackHandler
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.animation.AnimatedContentTransitionScope
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.EnterTransition
import androidx.compose.animation.ExitTransition
import androidx.compose.animation.core.FastOutSlowInEasing
import androidx.compose.animation.core.LinearOutSlowInEasing
import androidx.compose.animation.core.Spring
import androidx.compose.animation.core.spring
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.scaleIn
import androidx.compose.animation.scaleOut
import androidx.compose.animation.slideInHorizontally
import androidx.compose.animation.slideInVertically
import androidx.compose.animation.slideOutHorizontally
import androidx.compose.animation.slideOutVertically
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.displayCutout
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.only
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.systemBars
import androidx.compose.foundation.layout.union
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.material3.ExperimentalMaterial3ExpressiveApi
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationRail
import androidx.compose.material3.NavigationRailItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.Saver
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.input.nestedscroll.NestedScrollConnection
import androidx.compose.ui.input.nestedscroll.NestedScrollSource
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.navigation.NavBackStackEntry
import androidx.navigation.NavHostController
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import com.ramcosta.composedestinations.DestinationsNavHost
import com.ramcosta.composedestinations.animations.NavHostAnimatedDestinationStyle
import com.ramcosta.composedestinations.generated.NavGraphs
import com.ramcosta.composedestinations.generated.destinations.FlashScreenDestination
import com.ramcosta.composedestinations.utils.isRouteOnBackStackAsState
import com.ramcosta.composedestinations.utils.rememberDestinationsNavigator
import me.weishu.kernelsu.Natives
import me.weishu.kernelsu.ui.screen.BottomBarDestination
import me.weishu.kernelsu.ui.screen.FlashIt
import me.weishu.kernelsu.ui.theme.AppSettings
import me.weishu.kernelsu.ui.theme.ColorMode
import me.weishu.kernelsu.ui.theme.KernelSUTheme
import me.weishu.kernelsu.ui.theme.ThemeController
import me.weishu.kernelsu.ui.util.LocalSnackbarHost
import me.weishu.kernelsu.ui.util.install
import me.weishu.kernelsu.ui.util.rootAvailable
import me.weishu.kernelsu.ui.component.BottomBar

class MainActivity : ComponentActivity() {

    @OptIn(ExperimentalMaterial3ExpressiveApi::class)
    override fun onCreate(savedInstanceState: Bundle?) {

        // Enable edge to edge
        enableEdgeToEdge()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            window.isNavigationBarContrastEnforced = false
        }

        super.onCreate(savedInstanceState)

        val isManager = Natives.isManager
        if (isManager && !Natives.requireNewKernel()) install()

        val isAnyKernel = intent.component?.className?.endsWith("FlashAnyKernel") == true

        // Check if launched with a ZIP file
        val zipUri: ArrayList<Uri>? = if (intent.data != null) {
            arrayListOf(intent.data!!)
        } else {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                intent.getParcelableArrayListExtra("uris", Uri::class.java)
            } else {
                @Suppress("DEPRECATION")
                intent.getParcelableArrayListExtra("uris")
            }
        }

        val appSettingsSaver = Saver<AppSettings, IntArray>(
            save = { intArrayOf(it.colorMode.value, it.keyColor) },
            restore = { AppSettings(ColorMode.fromValue(it[0]), it[1]) }
        )

        setContent {
            val appSettingsState = rememberSaveable(stateSaver = appSettingsSaver) {
                mutableStateOf(ThemeController.getAppSettings(this@MainActivity))
            }

            val prefs = remember { getSharedPreferences("settings", MODE_PRIVATE) }

            var isFloatingState by remember {
                mutableStateOf(prefs.getBoolean("enable_floating_navbar", false))
            }

            val prefsListener = remember {
                SharedPreferences.OnSharedPreferenceChangeListener { _, key ->
                    if (key == "color_mode" || key == "key_color") {
                        appSettingsState.value = ThemeController.getAppSettings(this@MainActivity)
                    }
                    if (key == "enable_floating_navbar") {
                        isFloatingState = prefs.getBoolean("enable_floating_navbar", false)
                    }
                }
            }

            DisposableEffect(Unit) {
                prefs.registerOnSharedPreferenceChangeListener(prefsListener)
                onDispose {
                    prefs.unregisterOnSharedPreferenceChangeListener(prefsListener)
                }
            }

            KernelSUTheme(
                appSettings = appSettingsState.value
            ) {
                val navController = rememberNavController()
                val snackBarHostState = remember { SnackbarHostState() }

                val bottomBarRoutes = remember {
                    BottomBarDestination.entries.map { it.direction.route }.toSet()
                }

                val navigator = navController.rememberDestinationsNavigator()
                val currentBackStackEntry by navController.currentBackStackEntryAsState()
                val currentRoute = currentBackStackEntry?.destination?.route
                val showBottomBar = currentRoute in bottomBarRoutes

                var isScrollingDown by remember { mutableStateOf(false) }

                LaunchedEffect(currentRoute) {
                    isScrollingDown = false
                }

                val nestedScrollConnection = remember {
                    object : NestedScrollConnection {
                        var scrollAccumulator = 0f
                        override fun onPostScroll(
                            consumed: Offset,
                            available: Offset,
                            source: NestedScrollSource
                        ): Offset {
                            val delta = consumed.y
                            if (delta != 0f) {
                                scrollAccumulator += delta
                                scrollAccumulator = scrollAccumulator.coerceIn(-150f, 150f)

                                if (scrollAccumulator < -50f) {
                                    isScrollingDown = true
                                    scrollAccumulator = 0f
                                } else if (scrollAccumulator > 50f) {
                                    isScrollingDown = false
                                    scrollAccumulator = 0f
                                }
                            }
                            return Offset.Zero
                        }
                    }
                }

                val homeDestination = BottomBarDestination.entries.firstOrNull()
                val startRoute = homeDestination?.direction?.route

                if (homeDestination != null && startRoute != null) {
                    BackHandler(enabled = currentRoute != startRoute && currentRoute in bottomBarRoutes) {
                        navigator.navigate(homeDestination.direction) {
                            popUpTo(NavGraphs.root) {
                                saveState = true
                            }
                            launchSingleTop = true
                            restoreState = true
                        }
                    }
                }

                LaunchedEffect(zipUri) {
                    if (!zipUri.isNullOrEmpty()) {
                        val flashIt = if (isAnyKernel) {
                            FlashIt.FlashAnyKernel(zipUri.first())
                        } else {
                            FlashIt.FlashModules(zipUri)
                        }
                        navigator.navigate(
                            FlashScreenDestination(flashIt)
                        )
                    }
                }

                val configuration = LocalConfiguration.current
                val defaultTransitions = object : NavHostAnimatedDestinationStyle() {
                    override val enterTransition: AnimatedContentTransitionScope<NavBackStackEntry>.() -> EnterTransition =
                        {
                            if (targetState.destination.route !in bottomBarRoutes) {
                                slideInHorizontally(
                                    initialOffsetX = { fullWidth -> fullWidth },
                                    animationSpec = tween(300, easing = FastOutSlowInEasing)
                                ) + fadeIn(animationSpec = tween(300))
                            } else {
                                fadeIn(animationSpec = tween(220, easing = LinearOutSlowInEasing)) +
                                        scaleIn(
                                            initialScale = 0.96f,
                                            animationSpec = tween(220, easing = FastOutSlowInEasing)
                                        )
                            }
                        }

                    override val exitTransition: AnimatedContentTransitionScope<NavBackStackEntry>.() -> ExitTransition =
                        {
                            if (initialState.destination.route in bottomBarRoutes && targetState.destination.route !in bottomBarRoutes) {
                                slideOutHorizontally(
                                    targetOffsetX = { fullWidth -> -(fullWidth / 4) },
                                    animationSpec = tween(300, easing = FastOutSlowInEasing)
                                ) + fadeOut(animationSpec = tween(300))
                            } else {
                                fadeOut(animationSpec = tween(150))
                            }
                        }

                    override val popEnterTransition: AnimatedContentTransitionScope<NavBackStackEntry>.() -> EnterTransition =
                        {
                            if (targetState.destination.route in bottomBarRoutes) {
                                slideInHorizontally(
                                    initialOffsetX = { fullWidth -> -(fullWidth / 4) },
                                    animationSpec = tween(300, easing = FastOutSlowInEasing)
                                ) + fadeIn(animationSpec = tween(300))
                            } else {
                                fadeIn(animationSpec = tween(220))
                            }
                        }

                    override val popExitTransition: AnimatedContentTransitionScope<NavBackStackEntry>.() -> ExitTransition =
                        {
                            if (initialState.destination.route !in bottomBarRoutes) {
                                slideOutHorizontally(
                                    targetOffsetX = { fullWidth -> fullWidth },
                                    animationSpec = tween(300, easing = FastOutSlowInEasing)
                                ) + fadeOut(animationSpec = tween(300))
                            } else {
                                fadeOut(animationSpec = tween(150))
                            }
                        }
                }

                Scaffold(
                    bottomBar = {
                        if (showBottomBar && configuration.orientation == Configuration.ORIENTATION_PORTRAIT && !isFloatingState) {
                            BottomBar(navController)
                        }
                    },
                    contentWindowInsets = WindowInsets(0, 0, 0, 0)
                ) { innerPadding ->
                    CompositionLocalProvider(
                        LocalSnackbarHost provides snackBarHostState,
                    ) {
                        Box(modifier = Modifier.fillMaxSize().nestedScroll(nestedScrollConnection)) {

                            if (showBottomBar && configuration.orientation == Configuration.ORIENTATION_LANDSCAPE && !isFloatingState) {
                                Row(modifier = Modifier.fillMaxSize().windowInsetsPadding(WindowInsets.systemBars.only(WindowInsetsSides.Horizontal))) {
                                    SideBar(navController = navController, modifier = Modifier.windowInsetsPadding(WindowInsets.systemBars.only(WindowInsetsSides.Top)))
                                    DestinationsNavHost(
                                        modifier = Modifier.weight(1f).padding(innerPadding),
                                        navGraph = NavGraphs.root,
                                        navController = navController,
                                        defaultTransitions = defaultTransitions
                                    )
                                }
                            } else {
                                DestinationsNavHost(
                                    modifier = Modifier.padding(innerPadding),
                                    navGraph = NavGraphs.root,
                                    navController = navController,
                                    defaultTransitions = defaultTransitions
                                )
                            }

                            Box(modifier = Modifier.align(Alignment.BottomCenter)) {
                                AnimatedVisibility(
                                    visible = showBottomBar && isFloatingState && !isScrollingDown,
                                    enter = slideInVertically(
                                        initialOffsetY = { it * 2 },
                                        animationSpec = spring(
                                            dampingRatio = Spring.DampingRatioLowBouncy,
                                            stiffness = Spring.StiffnessLow
                                        )
                                    ) + fadeIn(tween(400)),
                                    exit = slideOutVertically(
                                        targetOffsetY = { it * 2 },
                                        animationSpec = spring(
                                            dampingRatio = Spring.DampingRatioNoBouncy,
                                            stiffness = Spring.StiffnessLow
                                        )
                                    ) + fadeOut(tween(400))
                                ) {
                                    BottomBar(navController)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun SideBar(navController: NavHostController, modifier: Modifier = Modifier) {
    val navigator = navController.rememberDestinationsNavigator()
    val isManager = Natives.isManager
    val fullFeatured = isManager && !Natives.requireNewKernel() && rootAvailable()
    val bottomBarRoutes = remember {
        BottomBarDestination.entries.map { it.direction.route }.toSet()
    }
    val currentRoute = navController.currentBackStackEntry?.destination?.route
    NavigationRail(
        modifier = modifier,
        containerColor = MaterialTheme.colorScheme.background,
    ) {
        Column(
            modifier = Modifier.fillMaxHeight(),
            verticalArrangement = Arrangement.spacedBy(8.dp, Alignment.CenterVertically)
        ) {
            BottomBarDestination.entries.forEach { destination ->
                if (!fullFeatured && destination.rootRequired) return@forEach
                val isCurrentDestOnBackStack by navController.isRouteOnBackStackAsState(destination.direction)
                NavigationRailItem(
                    selected = isCurrentDestOnBackStack,
                    onClick = {
                        if (isCurrentDestOnBackStack) {
                            navigator.popBackStack(destination.direction, false)
                        } else {
                            val isFromNonBottom = currentRoute !in bottomBarRoutes
                            navigator.navigate(destination.direction) {
                                if (isFromNonBottom) {
                                    popUpTo(NavGraphs.root) { inclusive = true }
                                } else {
                                    popUpTo(NavGraphs.root) { saveState = true }
                                }
                                launchSingleTop = true
                                restoreState = true
                            }
                        }
                    },
                    icon = {
                        Icon(
                            if (isCurrentDestOnBackStack) destination.iconSelected else destination.iconNotSelected,
                            stringResource(destination.label)
                        )
                    },
                    label = { Text(stringResource(destination.label)) },
                    alwaysShowLabel = false,
                    modifier = Modifier.padding(start = 8.dp)
                )
            }
        }
    }
}