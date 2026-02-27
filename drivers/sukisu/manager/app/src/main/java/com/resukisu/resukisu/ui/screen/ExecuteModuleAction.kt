package com.resukisu.resukisu.ui.screen

import android.annotation.SuppressLint
import android.os.Environment
import android.widget.Toast
import androidx.activity.compose.BackHandler
import androidx.activity.compose.LocalActivity
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawing
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.Save
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExtendedFloatingActionButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.key.Key
import androidx.compose.ui.input.key.key
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import com.ramcosta.composedestinations.annotation.Destination
import com.ramcosta.composedestinations.annotation.RootGraph
import com.ramcosta.composedestinations.navigation.DestinationsNavigator
import com.resukisu.resukisu.R
import com.resukisu.resukisu.ui.component.KeyEventBlocker
import com.resukisu.resukisu.ui.util.LocalSnackbarHost
import com.resukisu.resukisu.ui.util.runModuleAction
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

@SuppressLint("LocalContextGetResourceValueCall")
@Composable
@Destination<RootGraph>
fun ExecuteModuleActionScreen(navigator: DestinationsNavigator, moduleId: String) {
    var text by rememberSaveable { mutableStateOf("") }
    var tempText : String
    val logContent = rememberSaveable { StringBuilder() }
    val snackBarHost = LocalSnackbarHost.current
    val scope = rememberCoroutineScope()
    val scrollState = rememberScrollState()
    var isActionRunning by rememberSaveable { mutableStateOf(true) }
    val context = LocalContext.current
    val activity = LocalActivity.current

    BackHandler(enabled = isActionRunning) {
        // Disable back button if action is running
    }

    val fromShortcut = remember(activity) {
        val intent = activity?.intent
        intent?.getStringExtra("shortcut_type") == "module_action"
    }

    LaunchedEffect(Unit) {
        if (text.isNotEmpty()) {
            return@LaunchedEffect
        }
        withContext(Dispatchers.IO) {
            runModuleAction(
                moduleId = moduleId,
                onStdout = {
                    tempText = "$it\n"
                    if (tempText.startsWith("[H[J")) { // clear command
                        text = tempText.substring(6)
                    } else {
                        text += tempText
                    }
                    logContent.append(it).append("\n")
                },
                onStderr = {
                    logContent.append(it).append("\n")
                }
            ).let { actionResult ->
                if (actionResult && fromShortcut) {
                    withContext(Dispatchers.Main) {
                        Toast.makeText(
                            context,
                            context.getString(R.string.module_action_success),
                            Toast.LENGTH_SHORT
                        ).show()

                        activity?.finishAndRemoveTask()
                    }
                }
            }
        }
        isActionRunning = false
    }

    Scaffold(
        topBar = {
            TopBar(
                isActionRunning = isActionRunning,
                onSave = {
                    if (!isActionRunning) {
                        scope.launch {
                            val format = SimpleDateFormat("yyyy-MM-dd-HH-mm-ss", Locale.getDefault())
                            val date = format.format(Date())
                            val file = File(
                                Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS),
                                "KernelSU_module_action_log_${date}.log"
                            )
                            file.writeText(logContent.toString())
                            snackBarHost.showSnackbar("Log saved to ${file.absolutePath}")
                        }
                    }
                }
            )
        },
        floatingActionButton = {
            if (!isActionRunning) {
                ExtendedFloatingActionButton(
                    text = { Text(text = stringResource(R.string.close)) },
                    icon = { Icon(Icons.Filled.Close, contentDescription = null) },
                    onClick = {
                        navigator.popBackStack()
                    }
                )
            }
        },
        contentWindowInsets = WindowInsets.safeDrawing,
        snackbarHost = { SnackbarHost(snackBarHost) }
    ) { innerPadding ->
        KeyEventBlocker {
            it.key == Key.VolumeDown || it.key == Key.VolumeUp
        }
        Column(
            modifier = Modifier
                .fillMaxSize(1f)
                .padding(innerPadding)
                .verticalScroll(scrollState),
        ) {
            LaunchedEffect(text) {
                scrollState.animateScrollTo(scrollState.maxValue)
            }
            Text(
                modifier = Modifier.padding(8.dp),
                text = text,
                fontSize = MaterialTheme.typography.bodySmall.fontSize,
                fontFamily = FontFamily.Monospace,
                lineHeight = MaterialTheme.typography.bodySmall.lineHeight,
            )
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun TopBar(isActionRunning: Boolean, onSave: () -> Unit = {}) {
    TopAppBar(
        title = { Text(stringResource(R.string.action)) },
        actions = {
            IconButton(
                onClick = onSave,
                enabled = !isActionRunning
            ) {
                Icon(
                    imageVector = Icons.Filled.Save,
                    contentDescription = stringResource(id = R.string.save_log),
                )
            }
        }
    )
}
