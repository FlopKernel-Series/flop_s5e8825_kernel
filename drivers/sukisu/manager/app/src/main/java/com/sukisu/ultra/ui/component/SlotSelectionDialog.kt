package com.sukisu.ultra.ui.component

import android.content.Context
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import com.sukisu.ultra.R
import com.sukisu.ultra.ui.theme.ThemeConfig
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.SdStorage
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.vector.ImageVector

/**
 * 槽位选择对话框组件
 * 用于HorizonKernel刷写时选择目标槽位
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SlotSelectionDialog(
    show: Boolean,
    onDismiss: () -> Unit,
    onSlotSelected: (String) -> Unit
) {
    val context = LocalContext.current
    var currentSlot by remember { mutableStateOf<String?>(null) }
    var errorMessage by remember { mutableStateOf<String?>(null) }

    LaunchedEffect(Unit) {
        try {
            currentSlot = getCurrentSlot(context)
            errorMessage = null
        } catch (e: Exception) {
            errorMessage = e.message
            currentSlot = null
        }
    }

    if (show) {
        val cardColor = if (!ThemeConfig.useDynamicColor) {
            ThemeConfig.currentTheme.ButtonContrast
        } else {
            MaterialTheme.colorScheme.surfaceContainerHigh
        }

        AlertDialog(
            onDismissRequest = onDismiss,
            title = {
                Text(
                    text = stringResource(id = R.string.select_slot_title),
                    style = MaterialTheme.typography.headlineSmall,
                    color = MaterialTheme.colorScheme.onSurface
                )
            },
            text = {
                Column(
                    modifier = Modifier.padding(vertical = 8.dp),
                    verticalArrangement = Arrangement.spacedBy(16.dp)
                ) {
                    if (errorMessage != null) {
                        Text(
                            text = "Error: $errorMessage",
                            style = MaterialTheme.typography.bodyMedium,
                            color = MaterialTheme.colorScheme.error,
                            textAlign = TextAlign.Center
                        )
                    } else {
                        Text(
                            text = stringResource(
                                id = R.string.current_slot,
                                currentSlot ?: "Unknown"
                            ),
                            style = MaterialTheme.typography.bodyMedium,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                            textAlign = TextAlign.Center
                        )
                    }

                    Spacer(modifier = Modifier.height(12.dp))

                    Text(
                        text = stringResource(id = R.string.select_slot_description),
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                        textAlign = TextAlign.Center
                    )

                    Spacer(modifier = Modifier.height(24.dp))

                    // Horizontal arrangement for slot options with highlighted current slot
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .horizontalScroll(rememberScrollState()),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        val slotOptions = listOf(
                            ListOption(
                                titleText = stringResource(id = R.string.slot_a),
                                subtitleText = if (currentSlot == "a" || currentSlot == "_a") stringResource(id = R.string.currently_selected) else null,
                                icon = Icons.Filled.SdStorage
                            ),
                            ListOption(
                                titleText = stringResource(id = R.string.slot_b),
                                subtitleText = if (currentSlot == "b" || currentSlot == "_b") stringResource(id = R.string.currently_selected) else null,
                                icon = Icons.Filled.SdStorage
                            )
                        )

                        slotOptions.forEachIndexed { index, option ->
                            Column(
                                modifier = Modifier
                                    .weight(1f)
                                    .padding(horizontal = 8.dp)
                            ) {
                                Row(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .clip(MaterialTheme.shapes.medium)
                                        .background(
                                            color = if (option.subtitleText != null) {
                                                MaterialTheme.colorScheme.primary.copy(alpha = 0.9f)
                                            } else {
                                                MaterialTheme.colorScheme.primaryContainer.copy(alpha = 0.3f)
                                            }
                                        )
                                        .clickable {
                                            onSlotSelected(
                                                when (index) {
                                                    0 -> "a"
                                                    else -> "b"
                                                }
                                            )
                                        }
                                        .padding(vertical = 12.dp, horizontal = 16.dp),
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    Icon(
                                        imageVector = option.icon,
                                        contentDescription = null,
                                        tint = if (option.subtitleText != null) {
                                            MaterialTheme.colorScheme.onPrimary
                                        } else {
                                            MaterialTheme.colorScheme.primary
                                        },
                                        modifier = Modifier
                                            .padding(end = 16.dp)
                                            .size(24.dp)
                                    )
                                    Column(
                                        modifier = Modifier.weight(1f)
                                    ) {
                                        Text(
                                            text = option.titleText,
                                            style = MaterialTheme.typography.titleMedium,
                                            color = if (option.subtitleText != null) {
                                                MaterialTheme.colorScheme.onPrimary
                                            } else {
                                                MaterialTheme.colorScheme.primary
                                            }
                                        )
                                        option.subtitleText?.let {
                                            Text(
                                                text = it,
                                                style = MaterialTheme.typography.bodyMedium,
                                                color = if (true) {
                                                    MaterialTheme.colorScheme.onPrimary.copy(alpha = 0.8f)
                                                } else {
                                                    MaterialTheme.colorScheme.onSurfaceVariant
                                                }
                                            )
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            },
            confirmButton = {
                TextButton(
                    onClick = {
                        currentSlot?.let { onSlotSelected(it) }
                        onDismiss()
                    }
                ) {
                    Text(
                        text = stringResource(android.R.string.ok),
                        color = MaterialTheme.colorScheme.primary
                    )
                }
            },
            dismissButton = {
                TextButton(
                    onClick = onDismiss
                ) {
                    Text(
                        text = stringResource(android.R.string.cancel),
                        color = MaterialTheme.colorScheme.primary
                    )
                }
            },
            containerColor = cardColor,
            shape = MaterialTheme.shapes.extraLarge,
            tonalElevation = 4.dp
        )
    }
}

// Data class for list options
data class ListOption(
    val titleText: String,
    val subtitleText: String?,
    val icon: ImageVector
)

// Utility function to get current slot
private fun getCurrentSlot(context: Context): String? {
    return runCommandGetOutput(true, "getprop ro.boot.slot_suffix")?.let {
        if (it.startsWith("_")) it.substring(1) else it
    }
}

private fun runCommandGetOutput(su: Boolean, cmd: String): String? {
    return try {
        val process = ProcessBuilder(if (su) "su" else "sh").start()
        process.outputStream.bufferedWriter().use { writer ->
            writer.write("$cmd\n")
            writer.write("exit\n")
            writer.flush()
        }
        process.inputStream.bufferedReader().use { reader ->
            reader.readText().trim()
        }
    } catch (_: Exception) {
        null
    }
}