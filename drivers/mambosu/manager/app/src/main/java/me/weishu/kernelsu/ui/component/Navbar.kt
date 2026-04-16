package me.weishu.kernelsu.ui.component

import android.content.Context
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.animateColorAsState
import androidx.compose.animation.core.FastOutSlowInEasing
import androidx.compose.animation.core.tween
import androidx.compose.animation.expandHorizontally
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.shrinkHorizontally
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.displayCutout
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBars
import androidx.compose.foundation.layout.only
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.systemBars
import androidx.compose.foundation.layout.union
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarDefaults
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.surfaceColorAtElevation
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.navigation.NavHostController
import androidx.navigation.compose.currentBackStackEntryAsState
import com.ramcosta.composedestinations.generated.NavGraphs
import com.ramcosta.composedestinations.utils.isRouteOnBackStackAsState
import com.ramcosta.composedestinations.utils.rememberDestinationsNavigator
import me.weishu.kernelsu.Natives
import me.weishu.kernelsu.ui.screen.BottomBarDestination
import me.weishu.kernelsu.ui.util.rootAvailable

@Composable
fun BottomBar(navController: NavHostController) {
    val context = LocalContext.current
    val prefs = remember { context.getSharedPreferences("settings", Context.MODE_PRIVATE) }

    val isFloating by remember { mutableStateOf(prefs.getBoolean("enable_floating_navbar", false)) }

    val navigator = navController.rememberDestinationsNavigator()
    val isManager = Natives.isManager
    val fullFeatured = isManager && !Natives.requireNewKernel() && rootAvailable()
    val bottomBarRoutes = remember {
        BottomBarDestination.entries.map { it.direction.route }.toSet()
    }
    val currentBackStackEntry by navController.currentBackStackEntryAsState()
    val currentRoute = currentBackStackEntry?.destination?.route

    val visibleTabs = remember(fullFeatured) {
        BottomBarDestination.entries.filter { fullFeatured || !it.rootRequired }
    }

    val insets = if (isFloating) {
        WindowInsets(0, 0, 0, 0)
    } else {
        WindowInsets.systemBars.union(WindowInsets.displayCutout).only(
            WindowInsetsSides.Horizontal + WindowInsetsSides.Bottom
        )
    }

    Box(
        modifier = Modifier
            .fillMaxWidth()
            .then(if (isFloating) Modifier.windowInsetsPadding(WindowInsets.navigationBars) else Modifier),
        contentAlignment = Alignment.BottomCenter
    ) {
        if (isFloating) {
            Surface(
                modifier = Modifier.padding(bottom = 16.dp),
                shape = RoundedCornerShape(50),
                color = MaterialTheme.colorScheme.surfaceColorAtElevation(3.dp),
                tonalElevation = 0.dp
            ) {
                Row(
                    modifier = Modifier
                        .padding(horizontal = 8.dp, vertical = 6.dp)
                        .height(48.dp),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    visibleTabs.forEach { destination ->
                        val isCurrentDestOnBackStack by navController.isRouteOnBackStackAsState(destination.direction)

                        val animationSpec = tween<Color>(durationMillis = 300, easing = FastOutSlowInEasing)

                        val bgColor by animateColorAsState(
                            targetValue = if (isCurrentDestOnBackStack) MaterialTheme.colorScheme.primaryContainer else Color.Transparent,
                            animationSpec = animationSpec,
                            label = "bgColor"
                        )
                        val contentColor by animateColorAsState(
                            targetValue = if (isCurrentDestOnBackStack) MaterialTheme.colorScheme.onPrimaryContainer else MaterialTheme.colorScheme.onSurfaceVariant,
                            animationSpec = animationSpec,
                            label = "contentColor"
                        )

                        Row(
                            modifier = Modifier
                                .fillMaxHeight()
                                .clip(RoundedCornerShape(50))
                                .background(bgColor)
                                .clickable {
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
                                }
                                .padding(horizontal = 16.dp),
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.Center
                        ) {
                            Icon(
                                imageVector = if (isCurrentDestOnBackStack) destination.iconSelected else destination.iconNotSelected,
                                contentDescription = stringResource(destination.label),
                                tint = contentColor
                            )

                            AnimatedVisibility(
                                visible = isCurrentDestOnBackStack,
                                enter = expandHorizontally(
                                    animationSpec = tween(300, easing = FastOutSlowInEasing),
                                    expandFrom = Alignment.Start
                                ) + fadeIn(animationSpec = tween(300)),
                                exit = shrinkHorizontally(
                                    animationSpec = tween(300, easing = FastOutSlowInEasing),
                                    shrinkTowards = Alignment.Start
                                ) + fadeOut(animationSpec = tween(300))
                            ) {
                                Text(
                                    text = stringResource(destination.label),
                                    color = contentColor,
                                    style = MaterialTheme.typography.labelMedium,
                                    fontWeight = FontWeight.Bold,
                                    maxLines = 1,
                                    softWrap = false,
                                    modifier = Modifier.padding(start = 8.dp)
                                )
                            }
                        }
                    }
                }
            }
        } else {
            NavigationBar(
                windowInsets = insets,
                containerColor = NavigationBarDefaults.containerColor,
                tonalElevation = NavigationBarDefaults.Elevation
            ) {
                visibleTabs.forEach { destination ->
                    val isCurrentDestOnBackStack by navController.isRouteOnBackStackAsState(destination.direction)
                    NavigationBarItem(
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
                        alwaysShowLabel = false
                    )
                }
            }
        }
    }
}