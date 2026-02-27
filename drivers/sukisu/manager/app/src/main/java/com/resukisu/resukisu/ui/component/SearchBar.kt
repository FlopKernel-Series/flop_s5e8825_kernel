package com.resukisu.resukisu.ui.component

import androidx.activity.compose.BackHandler
import androidx.compose.animation.core.AnimationSpec
import androidx.compose.animation.core.DecayAnimationSpec
import androidx.compose.animation.rememberSplineBasedDecay
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.add
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.isImeVisible
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.text.input.rememberTextFieldState
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.twotone.ArrowBack
import androidx.compose.material.icons.twotone.Search
import androidx.compose.material3.AppBarWithSearch
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExperimentalMaterial3ExpressiveApi
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.MotionScheme
import androidx.compose.material3.SearchBarDefaults
import androidx.compose.material3.SearchBarScrollBehavior
import androidx.compose.material3.SearchBarValue
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.material3.rememberSearchBarState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.Saver
import androidx.compose.runtime.saveable.listSaver
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.NestedScrollConnection
import androidx.compose.ui.input.nestedscroll.NestedScrollSource
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.platform.LocalFocusManager
import androidx.compose.ui.platform.LocalSoftwareKeyboardController
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.resukisu.resukisu.R
import com.resukisu.resukisu.ui.component.settings.AppBackButton
import com.resukisu.resukisu.ui.theme.CardConfig
import com.resukisu.resukisu.ui.theme.ThemeConfig
import dev.chrisbanes.haze.HazeState
import dev.chrisbanes.haze.HazeStyle
import dev.chrisbanes.haze.HazeTint
import dev.chrisbanes.haze.hazeEffect
import kotlinx.coroutines.launch

@OptIn(ExperimentalMaterial3ExpressiveApi::class)
@ExperimentalMaterial3Api
@Composable
fun pinnedScrollBehavior(
    canScroll: () -> Boolean = { true },
    snapAnimationSpec: AnimationSpec<Float> = MotionScheme.expressive().defaultEffectsSpec(),
    flingAnimationSpec: DecayAnimationSpec<Float> = rememberSplineBasedDecay(),
    reverseLayout: Boolean = false,
): SearchBarScrollBehavior =
    rememberSaveable(
        snapAnimationSpec,
        flingAnimationSpec,
        canScroll,
        reverseLayout,
        saver =
            PinnedScrollBehavior.Saver(
                canScroll = canScroll,
            ),
    ) {
        PinnedScrollBehavior(
            canScroll = canScroll,
        )
    }

@OptIn(ExperimentalMaterial3Api::class)
private class PinnedScrollBehavior(
    val canScroll: () -> Boolean,
) : SearchBarScrollBehavior {
    // Offset remains 0 so the bar never moves vertically
    override var scrollOffset by mutableFloatStateOf(0f)
    override var scrollOffsetLimit by mutableFloatStateOf(0f)

    // Track contentOffset to allow for tonal elevation/color changes on scroll
    override var contentOffset by mutableFloatStateOf(0f)

    override fun Modifier.searchBarScrollBehavior(): Modifier {
        // We remove the .layout { ... } and .draggable blocks
        // that were responsible for hiding/moving the bar.
        return this.onSizeChanged { size ->
            scrollOffsetLimit = -size.height.toFloat()
        }
    }

    override val nestedScrollConnection: NestedScrollConnection =
        object : NestedScrollConnection {
            override fun onPreScroll(available: Offset, source: NestedScrollSource): Offset {
                if (!canScroll()) return Offset.Zero

                // We don't modify scrollOffset here because we want it pinned.
                // We only return Offset.Zero to show we aren't consuming any scroll.
                return Offset.Zero
            }

            override fun onPostScroll(
                consumed: Offset,
                available: Offset,
                source: NestedScrollSource,
            ): Offset {
                if (!canScroll()) return Offset.Zero

                // Update contentOffset so the UI knows how far the user has scrolled
                // This is used for "overlapped" state (changing colors/elevation)
                contentOffset += consumed.y
                return Offset.Zero
            }
        }
    companion object {
        fun Saver(
            canScroll: () -> Boolean
        ): Saver<PinnedScrollBehavior, *> =
            listSaver(
                save = {
                    listOf(
                        it.scrollOffset,
                        it.scrollOffsetLimit,
                        it.contentOffset,
                    )
                },
                restore = {
                    PinnedScrollBehavior(
                        canScroll = canScroll,
                    )
                },
            )
    }
}

@OptIn(ExperimentalMaterial3Api::class, ExperimentalMaterial3ExpressiveApi::class, ExperimentalLayoutApi::class)
@Composable
fun SearchAppBar(
    modifier: Modifier = Modifier,
    searchText: String,
    onSearchTextChange: (String) -> Unit,
    onBackClick: (() -> Unit)? = null,
    dropdownContent: @Composable (() -> Unit)? = null,
    navigationContent: @Composable (() -> Unit)? = null,
    scrollBehavior: SearchBarScrollBehavior? = null,
    searchBarPlaceHolderText: String,
    hazeState: HazeState? = null
) {
    val textFieldState = rememberTextFieldState(initialText = searchText)
    val searchBarState = rememberSearchBarState()

    val scope = rememberCoroutineScope()
    val keyboardController = LocalSoftwareKeyboardController.current
    val focusManager = LocalFocusManager.current
    val focusRequester = remember { FocusRequester() }

    var isExpanded = searchBarState.currentValue == SearchBarValue.Expanded
    val isKeyboardVisible = WindowInsets.isImeVisible

    LaunchedEffect(isKeyboardVisible) {
        if (!isKeyboardVisible) {
            searchBarState.animateToCollapsed()
        }
    }

    BackHandler(isExpanded) {
        scope.launch { searchBarState.animateToCollapsed() }
        keyboardController?.hide()
        focusManager.clearFocus()
        isExpanded = false
    }

    BackHandler(!isExpanded && textFieldState.text.isNotEmpty()) {
        textFieldState.edit {
            replace(0, length, "")
        }
    }

    LaunchedEffect(textFieldState.text) {
        onSearchTextChange(textFieldState.text.toString())
    }

    DisposableEffect(Unit) {
        onDispose { keyboardController?.hide() }
    }

    var modifier = modifier.fillMaxWidth()
    val surfaceContainerHigh = MaterialTheme.colorScheme.surfaceContainerHigh

    if (hazeState != null) modifier = modifier.hazeEffect(hazeState) {
        style = HazeStyle(
            backgroundColor = surfaceContainerHigh.copy(
                alpha = 0.8f
            ),
            tint = HazeTint(Color.Transparent)
        )
        blurRadius = 30.dp
        noiseFactor = 0f
    }

    AppBarWithSearch(
        modifier = modifier.background(
            if (ThemeConfig.backgroundImageLoaded) Color.Transparent
            else MaterialTheme.colorScheme.surfaceContainer
        ),
        state = searchBarState,
        inputField = {
            SearchBarDefaults.InputField(
                modifier = Modifier
                    .fillMaxWidth()
                    .focusRequester(focusRequester)
                    .padding(bottom = 4.dp)
                    .clip(SearchBarDefaults.inputFieldShape)
                    .height(52.dp), // box padding + icon padding + icon size
                searchBarState = searchBarState,
                textFieldState = textFieldState,
                onSearch = { text ->
                    scope.launch { searchBarState.animateToCollapsed() }
                    keyboardController?.hide()
                    focusManager.clearFocus()
                    onSearchTextChange(text)
                },
                colors = SearchBarDefaults.inputFieldColors(
                    unfocusedContainerColor = MaterialTheme.colorScheme.surfaceContainerHighest.copy(
                        alpha = CardConfig.cardAlpha
                    ),
                    focusedContainerColor = MaterialTheme.colorScheme.surfaceContainerHighest.copy(
                        alpha = CardConfig.cardAlpha
                    ),
                ),
                placeholder = {
                    Text(
                        modifier = Modifier
                            .fillMaxWidth()
                            .clearAndSetSemantics {},
                        text = searchBarPlaceHolderText,
                        style = MaterialTheme.typography.bodyLarge,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                },
                leadingIcon = {
                    Row {
                        if (onBackClick == null && isExpanded) {
                            Icon(
                                imageVector = Icons.AutoMirrored.TwoTone.ArrowBack,
                                contentDescription = stringResource(R.string.back),
                                tint = MaterialTheme.colorScheme.onSurfaceVariant,
                                modifier = Modifier
                                    .padding(start = 8.dp)
                                    .size(40.dp)
                                    .clip(CircleShape)
                                    .clickable {
                                        if (textFieldState.text.isNotEmpty()) {
                                            textFieldState.edit {
                                                replace(0, length, "")
                                            }
                                            return@clickable
                                        }
                                        scope.launch {
                                            searchBarState.animateToCollapsed()
                                            keyboardController?.hide()
                                            focusManager.clearFocus()
                                        }
                                    }
                                    .padding(8.dp)
                            )
                        }
                        else {
                            Icon(
                                imageVector = Icons.TwoTone.Search,
                                contentDescription = stringResource(R.string.search),

                                tint = MaterialTheme.colorScheme.onSurfaceVariant,
                                modifier = Modifier
                                    .padding(start = 8.dp)
                                    .size(40.dp)
                                    .clip(CircleShape)
                                    .clickable {
                                        scope.launch {
                                            searchBarState.animateToExpanded()
                                            focusRequester.requestFocus()
                                            keyboardController?.show()
                                        }
                                    }
                                    .padding(8.dp)
                            )
                        }
                        Spacer(modifier = Modifier.width(3.dp))
                    }
                }
            )
        },
        navigationIcon = {
            if (onBackClick != null) {
                AppBackButton(
                    onClick = {
                        if (isExpanded) {
                            if (textFieldState.text.isNotEmpty()) {
                                textFieldState.edit {
                                    replace(0, length, "")
                                }
                            } else {
                                scope.launch {
                                    searchBarState.animateToCollapsed()
                                    keyboardController?.hide()
                                    focusManager.clearFocus()
                                }
                            }
                            return@AppBackButton
                        }
                        onBackClick.invoke()
                    }
                )
            } else {
                navigationContent?.invoke()
            }
        },
        actions = {
            dropdownContent?.invoke()
        },
        scrollBehavior = scrollBehavior,
        windowInsets = TopAppBarDefaults.windowInsets.add(WindowInsets(left = 12.dp)),
        colors = SearchBarDefaults.appBarWithSearchColors(
            searchBarColors = SearchBarDefaults.colors(
                containerColor = Color.Transparent
            ),
            scrolledSearchBarContainerColor = Color.Transparent,
            appBarContainerColor = Color.Transparent,
            scrolledAppBarContainerColor = Color.Transparent,
        ),
    )
}

@OptIn(ExperimentalMaterial3Api::class)
@Preview
@Composable
private fun SearchAppBarPreview() {
    SearchAppBar(
        searchText = "",
        onSearchTextChange = {},
        searchBarPlaceHolderText = "",
        hazeState = null
    )
}