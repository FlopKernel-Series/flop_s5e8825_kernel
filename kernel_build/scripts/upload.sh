CAPTION_BUILD="Build info:
*Device*: \`${DEVICE} [${CODENAME}]\`
*Kernel Version*: \`${LINUX_VER}\`
*Compiler*: \`${KBUILD_COMPILER_STRING}\`
*Linker*: \`$("${LINKER}" -v | head -n1 | sed 's/(compatible with [^)]*)//' |
            head -n 1 | perl -pe 's/\(http.*?\)//gs' | sed -e 's/  */ /g' -e 's/[[:space:]]*$//')\`
*Build host*: \`${BUILD_HOST}\`
*Branch*: \`$(git rev-parse --abbrev-ref HEAD)\`
*Commit*: \`$(git rev-parse --short HEAD)\`
*Build type*: \`${BUILD_TYPE}\`
*Clean build*: \`$( [ "$DO_CLEAN" -eq 1 ] && echo Yes || echo No )\`
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

    if [[ "$DO_BASHUP" == "1" ]]; then
        echo -e "\nINFO: Uploading to bashupload.com\n"
        curl -T "$ZIP_PATH" bashupload.com
    fi

    if [[ "$DO_TG" == "1" ]]; then
        echo -e "\nINFO: Uploading to Telegram\n"
        tgs "$TAR_PATH"
    fi

    if [[ "$BUILD_LOG" == "1" ]]; then
        echo -e "\nINFO: Uploading log to bashupload.com\n"
        curl -T log.txt bashupload.com
    fi
}
