CAPTION_BUILD="Build info:
*Device*: \`${DEVICE} [${CODENAME}]\`
*Kernel Version*: \`${LINUX_VER}\`
*Compiler*: \`${KBUILD_COMPILER_STRING}\`
*Build host*: \`${BUILD_HOST}\`
*Commit / Branch*: [($(git rev-parse HEAD | cut -c -7))]($(echo $KERNEL_URL)/commit/$(git rev-parse HEAD)) / \`$(git rev-parse --abbrev-ref HEAD)\`
*Build variant*: \`${FK_TYPE}\` / \`${BUILD_TYPE}$( [ "$DO_CLEAN" -eq 1 ] && echo " (clean)" || echo " (dirty)")\`
"

tgs() {
    local FILE="$1"
    local MD5=$(md5sum "$FILE" | cut -d' ' -f1)
    local CAPTION_FINAL="${CAPTION_BUILD}*MD5*: \`${MD5}\`"
    curl -fsSL -X POST "https://api.telegram.org/bot${TELEGRAM_BOT_TOKEN}/sendDocument" \
        -F chat_id="${TELEGRAM_CHAT_ID}" \
        -F document=@"$FILE" \
        -F parse_mode="Markdown" \
        -F disable_web_page_preview="true" \
        -F caption="${CAPTION_FINAL}" &>/dev/null
}

upload() {
    cd "$KDIR"

    if [ "$DO_BASHUP" = "1" ]; then
        echo -e "\nINFO: Uploading build and log to bashupload.com\n"
        curl -T "$ZIP_PATH" bashupload.com || echo "WARNING: bashupload operation failed (ignored)"
        curl -T log.txt bashupload.com || echo "WARNING: bashupload operation failed (ignored)"
    fi

    if [ "$DO_TG" = "1" ]; then
        echo -e "\nINFO: Uploading to Telegram\n"
        tgs "$ZIP_PATH"
    fi
}
