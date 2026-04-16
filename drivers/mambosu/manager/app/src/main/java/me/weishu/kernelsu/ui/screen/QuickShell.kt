@file:OptIn(ExperimentalMaterial3Api::class)

package me.weishu.kernelsu.ui.screen

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import com.ramcosta.composedestinations.annotation.Destination
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.Close
import androidx.compose.material.icons.outlined.Delete
import androidx.compose.material.icons.outlined.PlayArrow
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.ramcosta.composedestinations.annotation.RootGraph
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

// libsu
import com.topjohnwu.superuser.CallbackList
import com.topjohnwu.superuser.Shell

@Destination<RootGraph>
@Composable
fun QuickShellScreen() {
    var cmd by rememberSaveable { mutableStateOf("") }
    var isRunning by rememberSaveable { mutableStateOf(false) }

    val logs = remember { mutableStateListOf<String>() }
    val scope = rememberCoroutineScope()
    val listState = rememberLazyListState()

    fun append(line: String) {
        logs.add(line)
    }

    fun runCommand() {
        val script = cmd.trim()
        if (script.isEmpty()) return

        isRunning = true
        append("[start] ---- TIME: ${System.currentTimeMillis()} ----")

        val stdoutCb = object : CallbackList<String>() {
            override fun onAddElement(e: String) {
                scope.launch(Dispatchers.Main) { append(e) }
            }
        }
        val stderrCb = object : CallbackList<String>() {
            override fun onAddElement(e: String) {
                scope.launch(Dispatchers.Main) { append("E: $e") }
            }
        }

        Shell.cmd(script)
            .to(stdoutCb, stderrCb)
            .submit { result ->
                scope.launch(Dispatchers.Main) {
                    append("[exit] code=${result.code}")
                    isRunning = false
                }
            }
    }

    fun cancelCommand() {
        scope.launch(Dispatchers.IO) {
            try {
                Shell.getCachedShell()?.close()

                launch(Dispatchers.Main) {
                    append("!! [CANCELLED] Process terminated by user !!")
                    isRunning = false
                }
            } catch (e: Exception) {
                launch(Dispatchers.Main) {
                    append("Error cancelling: ${e.message}")
                }
            }
        }
    }

    LaunchedEffect(logs.size) {
        if (logs.isNotEmpty()) listState.animateScrollToItem(logs.lastIndex)
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Text(
                        text = "QuickShell",
                        fontWeight = FontWeight.Bold
                    )
                },
                actions = {
                    IconButton(
                        onClick = { cancelCommand() },
                        enabled = isRunning
                    ) {
                        Icon(Icons.Outlined.Close, contentDescription = "Stop Command")
                    }
                    IconButton(onClick = { logs.clear() }) {
                        Icon(Icons.Outlined.Delete, contentDescription = "Clear Logs")
                    }
                }
            )
        }
    ) { padding ->
        Column(
            modifier = Modifier
                .padding(padding)
                .fillMaxSize()
                .padding(horizontal = 16.dp, vertical = 10.dp)
        ) {
            TonalCard(modifier = Modifier.fillMaxWidth()) {
                Column(modifier = Modifier.padding(14.dp)) {
                    Text(
                        text = "Commands",
                        style = MaterialTheme.typography.labelMedium,
                        color = MaterialTheme.colorScheme.primary
                    )

                    Spacer(Modifier.height(8.dp))

                    OutlinedTextField(
                        value = cmd,
                        onValueChange = { cmd = it },
                        modifier = Modifier.fillMaxWidth(),
                        singleLine = false,
                        minLines = 1,
                        maxLines = 6,
                        placeholder = { Text("Input Command") },
                        trailingIcon = {
                            IconButton(
                                onClick = { runCommand() },
                                enabled = cmd.isNotBlank() && !isRunning
                            ) {
                                Icon(Icons.Outlined.PlayArrow, contentDescription = "Run")
                            }
                        }
                    )

                }
            }

            Spacer(Modifier.height(10.dp))

            TonalCard(modifier = Modifier.fillMaxSize()) {
                LazyColumn(
                    state = listState,
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(14.dp)
                ) {
                    itemsIndexed(logs) { _, line ->
                        Text(
                            text = line,
                            style = MaterialTheme.typography.bodySmall,
                            fontFamily = FontFamily.Monospace,
                            color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.85f)
                        )
                    }
                }
            }
        }
    }
}