package me.weishu.kernelsu.ui.component

import android.annotation.SuppressLint
import android.content.Intent
import android.graphics.Color
import android.util.Base64
import android.util.Log
import android.view.View
import android.webkit.WebChromeClient
import android.webkit.WebResourceRequest
import android.webkit.WebResourceResponse
import android.webkit.WebSettings
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.FrameLayout
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.wrapContentHeight
import androidx.compose.material3.ExperimentalMaterial3ExpressiveApi
import androidx.compose.material3.LinearWavyProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clipToBounds
import androidx.compose.ui.platform.LocalLayoutDirection
import androidx.compose.ui.unit.LayoutDirection
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.net.toUri
import androidx.webkit.WebViewAssetLoader
import me.weishu.kernelsu.ksuApp
import okhttp3.Headers.Companion.toHeaders
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.Response
import okio.IOException
import java.io.ByteArrayInputStream
import java.nio.charset.StandardCharsets

fun androidx.compose.ui.graphics.Color.toCssRgba(): String {
    return "rgba(${(this.red * 255).toInt()}, ${(this.green * 255).toInt()}, ${(this.blue * 255).toInt()}, ${this.alpha})"
}

@OptIn(ExperimentalMaterial3ExpressiveApi::class)
@SuppressLint("SetJavaScriptEnabled")
@Composable
fun GithubMarkdown(
    content: String,
    containerColor: androidx.compose.ui.graphics.Color = androidx.compose.ui.graphics.Color.Transparent
) {
    val dir = if (LocalLayoutDirection.current == LayoutDirection.Rtl) "rtl" else "ltr"

    val cs = MaterialTheme.colorScheme
    val textColor = cs.onSurface.toCssRgba()
    val linkColor = cs.primary.toCssRgba()
    val codeBgColor = cs.secondaryContainer.toCssRgba()
    val codeTextColor = cs.onSecondaryContainer.toCssRgba()
    val quoteBorderColor = cs.primary.toCssRgba()
    val borderColor = cs.outlineVariant.toCssRgba()
    val stripeBgColor = cs.surfaceContainerHighest.toCssRgba()

    var progress by remember { mutableFloatStateOf(0f) }
    var isLoaded by remember { mutableStateOf(false) }

    val base64Content = remember(content) {
        Base64.encodeToString(content.toByteArray(StandardCharsets.UTF_8), Base64.NO_WRAP)
    }

    val customCss = """
        :root {
            --text-color: $textColor;
            --link-color: $linkColor;
            --code-bg: $codeBgColor;
            --code-text: $codeTextColor;
            --quote-border: $quoteBorderColor;
            --border-color: $borderColor;
            --stripe-bg: $stripeBgColor;
        }
        * { box-sizing: border-box; }
        html, body {
            margin: 0; padding: 0;
            background-color: transparent !important;
            color: var(--text-color) !important;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            font-size: 14.5px;
            line-height: 1.6;
            -webkit-user-select: none; user-select: none;
        }
        .markdown-body { padding: 8px 0px 24px 0px; word-wrap: break-word; color: var(--text-color) !important; }
        .markdown-body h1, .markdown-body h2, .markdown-body h3, .markdown-body h4, .markdown-body h5, .markdown-body h6 {
            margin-top: 24px; margin-bottom: 16px; font-weight: 600; line-height: 1.25; color: var(--text-color);
        }
        .markdown-body h1 { font-size: 2em; border-bottom: 1px solid var(--border-color); padding-bottom: .3em; }
        .markdown-body h2 { font-size: 1.5em; border-bottom: 1px solid var(--border-color); padding-bottom: .3em; }
        .markdown-body h3 { font-size: 1.25em; }
        .markdown-body p { margin-top: 0; margin-bottom: 16px; }
        .markdown-body a { color: var(--link-color) !important; text-decoration: none; font-weight: 500; }
        .markdown-body a:hover { text-decoration: underline; }
        .markdown-body img, .markdown-body video { max-width: 100%; height: auto; border-radius: 8px; margin: 8px 0; }
        .markdown-body hr { height: 1px; padding: 0; margin: 24px 0; background-color: var(--border-color); border: 0; }
        .markdown-body blockquote {
            margin: 0 0 16px 0; padding: 0 1em;
            color: var(--text-color); opacity: 0.85;
            border-left: 4px solid var(--quote-border);
            background-color: rgba(128, 128, 128, 0.05);
            border-top-right-radius: 4px; border-bottom-right-radius: 4px;
        }
        .markdown-body ul, .markdown-body ol { margin-top: 0; margin-bottom: 16px; padding-left: 2em; }
        .markdown-body li { margin-bottom: 4px; }
        .markdown-body pre, .markdown-body code {
            background-color: var(--code-bg) !important; 
            color: var(--code-text) !important; 
            border-radius: 6px;
            font-family: ui-monospace, SFMono-Regular, "SF Mono", Menlo, Consolas, monospace;
            -webkit-user-select: text; user-select: text;
        }
        .markdown-body code { padding: .2em .4em; font-size: 85%; }
        .markdown-body pre { padding: 16px; overflow: auto; font-size: 85%; line-height: 1.45; }
        .markdown-body pre code { padding: 0; background-color: transparent !important; color: inherit !important; }
        .markdown-body table { 
            border-spacing: 0; border-collapse: collapse; margin-top: 0; margin-bottom: 16px; 
            width: 100%; overflow: auto; display: block;
        }
        .markdown-body table th, .markdown-body table td { padding: 6px 13px; border: 1px solid var(--border-color); }
        .markdown-body table tr { background-color: transparent; border-top: 1px solid var(--border-color); }
        .markdown-body table tr:nth-child(2n) { background-color: var(--stripe-bg) !important; }
        .markdown-body details summary { cursor: pointer; font-weight: 600; outline: none; margin-bottom: 8px; }
    """.trimIndent()

    val markedJsHref = "https://unpkg.com/marked@12.0.1/marked.min.js"

    val html = """
        <!DOCTYPE html>
        <html>
        <head>
          <meta charset='utf-8'/>
          <meta name='viewport' content='width=device-width, initial-scale=1'/>
          <script src="$markedJsHref"></script>
          <style>$customCss</style>
        </head>
        <body dir='${dir}'>
          <article class='markdown-body' id='content'></article>
          <script>
            window.onload = function() {
              try {
                  const base64 = "$base64Content";
                  
                  const binary = window.atob(base64);
                  const bytes = new Uint8Array(binary.length);
                  for (let i = 0; i < binary.length; i++) {
                      bytes[i] = binary.charCodeAt(i);
                  }
                  const rawMarkdown = new TextDecoder('utf-8').decode(bytes);
                  
                  marked.use({ gfm: true, breaks: true });
                  document.getElementById('content').innerHTML = marked.parse(rawMarkdown);
              } catch (e) {
                  document.getElementById('content').innerHTML = "<p>Error rendering markdown.</p>";
                  console.error(e);
              }
            };
          </script>
        </body>
        </html>
    """.trimIndent()

    Box(
        modifier = Modifier
            .fillMaxWidth()
            .clipToBounds()
            .background(containerColor)
    ) {
        if (!isLoaded) {
            Box(modifier = Modifier.fillMaxWidth().padding(horizontal = 16.dp)) {
                LinearWavyProgressIndicator(
                    progress = { progress },
                    modifier = Modifier.fillMaxWidth()
                )
            }
        }
        AndroidView(
            factory = { ctx ->
                val frameLayout = FrameLayout(ctx)
                val webView = WebView(ctx).apply {
                    try {
                        setBackgroundColor(Color.TRANSPARENT)
                        setLayerType(View.LAYER_TYPE_NONE, null)

                        isVerticalScrollBarEnabled = false
                        isHorizontalScrollBarEnabled = false

                        settings.apply {
                            cacheMode = WebSettings.LOAD_CACHE_ELSE_NETWORK
                            domStorageEnabled = true
                            layoutAlgorithm = WebSettings.LayoutAlgorithm.NORMAL
                            offscreenPreRaster = false
                            mixedContentMode = WebSettings.MIXED_CONTENT_ALWAYS_ALLOW
                            allowContentAccess = false
                            allowFileAccess = false
                            textZoom = 90
                            setSupportZoom(false)
                            setGeolocationEnabled(false)
                            javaScriptEnabled = true
                        }

                        webChromeClient = object : WebChromeClient() {
                            override fun onProgressChanged(view: WebView?, newProgress: Int) {
                                progress = newProgress / 100f
                                if (newProgress >= 90) {
                                    isLoaded = true
                                }
                            }
                        }

                        webViewClient = object : WebViewClient() {
                            private val assetLoader = WebViewAssetLoader.Builder()
                                .addPathHandler("/assets/", WebViewAssetLoader.AssetsPathHandler(ctx))
                                .build()

                            override fun shouldOverrideUrlLoading(
                                view: WebView, request: WebResourceRequest
                            ): Boolean {
                                val url = request.url.toString()
                                val intent = Intent(Intent.ACTION_VIEW, url.toUri())
                                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                                ctx.startActivity(intent)
                                return true
                            }

                            override fun shouldInterceptRequest(
                                view: WebView, request: WebResourceRequest
                            ): WebResourceResponse? {
                                assetLoader.shouldInterceptRequest(request.url)?.let { return it }
                                val scheme = request.url.scheme ?: return null
                                if (!scheme.startsWith("http")) return null

                                val client: OkHttpClient = ksuApp.okhttpClient
                                val call = client.newCall(
                                    Request.Builder()
                                        .url(request.url.toString())
                                        .method(request.method, null)
                                        .headers(request.requestHeaders.toHeaders())
                                        .build()
                                )
                                return try {
                                    val reply: Response = call.execute()
                                    val header = reply.header("content-type", "text/plain; charset=utf-8")
                                    val contentTypes = header?.split(";\\s*".toRegex()) ?: emptyList()
                                    val mimeType = contentTypes.firstOrNull() ?: "image/*"
                                    val charset = contentTypes.getOrNull(1)?.split("=\\s*".toRegex())?.getOrNull(1) ?: "utf-8"
                                    val body = reply.body ?: return null
                                    WebResourceResponse(mimeType, charset, body.byteStream())
                                } catch (e: IOException) {
                                    WebResourceResponse(
                                        "text/html", "utf-8",
                                        ByteArrayInputStream(Log.getStackTraceString(e).toByteArray(StandardCharsets.UTF_8))
                                    )
                                }
                            }
                        }

                        loadDataWithBaseURL(
                            "https://appassets.androidplatform.net", html,
                            "text/html", StandardCharsets.UTF_8.name(), null
                        )
                    } catch (e: Throwable) {
                        Log.e("GithubMarkdown", "WebView setup failed", e)
                    }
                }
                frameLayout.addView(webView)
                frameLayout
            },
            update = { view ->
                view.visibility = if (isLoaded) View.VISIBLE else View.INVISIBLE
            },
            modifier = Modifier
                .fillMaxWidth()
                .wrapContentHeight()
        )
    }
}