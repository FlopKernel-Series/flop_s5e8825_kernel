#!/bin/env bash

str="$COMMIT_MESSAGE"
len=${#str}

echo "INFO: Char length: $len"

if [ $len -gt 1024 ]; then
msg="*$TITLE*
\\#ci\\_$VERSION

[Commit]($COMMIT_URL)
[Workflow run]($RUN_URL)
"
else
msg="*$TITLE*
\\#ci\\_$VERSION
\`\`\`
$COMMIT_MESSAGE
\`\`\`
[Commit]($COMMIT_URL)
[Workflow run]($RUN_URL)
"
fi

file="$1"

curl -s -F document=@$file "https://api.telegram.org/bot$BOT_TOKEN/sendDocument" \
	-F chat_id="$CHAT_ID" \
	-F "disable_web_page_preview=true" \
	-F "parse_mode=markdownv2" \
	-F caption="$msg"
