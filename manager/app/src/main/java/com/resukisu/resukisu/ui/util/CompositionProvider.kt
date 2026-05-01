package com.resukisu.resukisu.ui.util

import androidx.compose.foundation.pager.PagerState
import androidx.compose.material3.SnackbarHostState
import androidx.compose.runtime.compositionLocalOf
import com.resukisu.resukisu.ui.activity.PermissionRequestInterface
import dev.chrisbanes.haze.HazeState

val LocalSnackbarHost = compositionLocalOf<SnackbarHostState> {
    error("CompositionLocal LocalSnackbarController not present")
}

val LocalHazeState = compositionLocalOf<HazeState?> {
    error("CompositionLocal HazeState not present")
}

val LocalPagerState = compositionLocalOf<PagerState> { error("No pager state") }
val LocalHandlePageChange = compositionLocalOf<(Int) -> Unit> { error("No handle page change") }
val LocalSelectedPage = compositionLocalOf<Int> { error("No selected page") }

val LocalPermissionRequestInterface = compositionLocalOf<PermissionRequestInterface> {
    error("CompositionLocal LocalPermissionRequestInterface not present")
}