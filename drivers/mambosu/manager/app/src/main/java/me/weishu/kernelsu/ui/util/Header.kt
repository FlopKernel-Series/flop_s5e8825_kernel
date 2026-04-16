package me.weishu.kernelsu.ui.util

import android.content.Context

private const val PREFS_NAME = "settings"
private const val KEY_HEADER_IMAGE = "header_image_uri"
private const val KEY_LAYOUT_STYLE = "layout_style_classic"

/**
 * @return uri string of custom header image, or null if using default
 */
fun Context.getHeaderImage(): String? {
    return getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        .getString(KEY_HEADER_IMAGE, null)
}

/**
 * Save custom header image uri.
 * Default header should NOT be saved.
 */
fun Context.saveHeaderImage(uri: String) {
    getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        .edit()
        .putString(KEY_HEADER_IMAGE, uri)
        .apply()
}

/**
 * Clear custom header image and fallback to default.
 */
fun Context.clearHeaderImage() {
    getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        .edit()
        .remove(KEY_HEADER_IMAGE)
        .apply()
}

/**
 * Membaca status layout terakhir.
 * @return true = Classic (Atas-Bawah), false = Modern (Samping-Samping)
 * Defaultnya adalah true (Classic).
 */
fun Context.getLayoutStyle(): Boolean {
    val prefs = getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    return prefs.getBoolean(KEY_LAYOUT_STYLE, true)
}

/**
 * Menyimpan pilihan layout user.
 * @param isClassic true untuk Classic, false untuk Modern
 */
fun Context.setLayoutStyle(isClassic: Boolean) {
    val prefs = getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    prefs.edit().putBoolean(KEY_LAYOUT_STYLE, isClassic).apply()
}