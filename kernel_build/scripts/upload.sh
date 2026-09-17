CAPTION_BUILD="Build info:
*Device*: \`${DEVICE} [${CODENAME}]\`
*Kernel Version*: \`${LINUX_VER}\`
*Compiler*: \`${KBUILD_COMPILER_STRING}\`
*Build host*: \`${BUILD_HOST}\`
*Commit / Branch*: [($(git rev-parse HEAD | cut -c -7))]($(echo $KERNEL_URL)/commit/$(git rev-parse HEAD)) / \`$(git rev-parse --abbrev-ref HEAD)\`
*Build variant*: \`${FK_TYPE}\` / \`${BUILD_TYPE}$( [ "$DO_CLEAN" -eq 1 ] && echo " (clean)" || echo " (dirty)")\`
*Timestamp*: \`${DATE}\`
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

tgs_nhmod() {
    local FILE="$1"
    local MD5
    local CAPTION_NH

    MD5=$(md5sum "$FILE" | cut -d' ' -f1)
    CAPTION_NH=$(printf "Nethunter Extras for %s %s (%s) @ %s\n*MD5*: \`%s\`\n*Build*: \`%s\`" \
        "$FK_VER" "$FK_TYPE" "$CODENAME" "$DATE" "$MD5" "$(basename "$ZIP_PATH")")

    curl -fsSL -X POST "https://api.telegram.org/bot${TELEGRAM_BOT_TOKEN}/sendDocument" \
        -F chat_id="${TELEGRAM_CHAT_ID}" \
        -F document=@"$FILE" \
        -F parse_mode="Markdown" \
        -F disable_web_page_preview="true" \
        -F caption="${CAPTION_NH}" &>/dev/null
}

upload() {
    cd "$KDIR"

    if [ "$DO_BASHUP" = "1" ]; then
        echo -e "\n$(log_info "Uploading build and log to bashupload.com")\n"
        curl -T "$ZIP_PATH" bashupload.com || log_warn "bashupload operation failed (ignored)"
        curl -T log.txt bashupload.com || log_warn "bashupload operation failed (ignored)"
    fi

    if [ "$DO_TG" = "1" ]; then
        echo -e "\n$(log_info "Uploading to Telegram")\n"
        tgs "$ZIP_PATH"

        if [ -n "${NH_MODULE_PATH:-}" ] && [ -f "$NH_MODULE_PATH" ]; then
            echo
            log_info "Uploading Nethunter Extras to Telegram"
            tgs_nhmod "$NH_MODULE_PATH"
        fi
    fi
}
