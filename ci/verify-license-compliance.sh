#!/usr/bin/env bash

SCAN_COMMENT="${BUILDKITE_BUILD_URL:-UnrealGDK Manual Scan}"
SCAN_TARGET="$(git rev-parse --show-toplevel)"
UNREALGDK_SETUP_SCRIPT="./Setup.sh"
WSS_AGENT_VERSION="19.3.1"
WSS_JAR_FILE="wss-unified-agent-${WSS_AGENT_VERSION}.jar"
WSS_PROJECT_NAME="${CI_REPOSITORY_NAME:-unrealgdk_manual_scan}"
WSS_SECRET_BK_ORG="improbable"
WSS_SECRET_ENV="production"
WSS_SECRET_NAME="whitesource-scanner-api-token"
WSS_SECRET_TYPE="whitesource-token"
WSS_URL="https://app-eu.whitesourcesoftware.com/agent"

# Helper func for the callable log_group funcs below
_log_group() {
  local name="${1}"
  local parent="${2}"
  local which="${3:-start}"
  if [[ -n "${BUILDKITE}" ]]; then
    imp-ci events new --name "${name}" --child-of "${parent}"
  fi
  # BuildKite log groups terminate when another begins, so only emit one on 'start'.
  if [[ "${which}" == "start" ]]; then
    echo "--- ${name}"
  fi
}

# log_group_start <name> <parent> creates a build event with given name and parent name
# and starts a new BuildKite log group (see doc: https://buildkite.com/docs/pipelines/managing-log-output)
log_group_start() {
  _log_group "${1}" "${2}" "start"
}

# log_group_end <name> <parent> closes a build event with given name and parent name
log_group_end() {
  _log_group "${1}" "${2}" "end"
}

# Run the script used to fetch the UnrealGDK deps so we can include them in the scan.
log_group_start "fetch-unrealgdk-dependencies" "command"
"${UNREALGDK_SETUP_SCRIPT}"
log_group_end "fetch-unrealgdk-dependencies" "command"

pushd ci/license-check

# Documented at: https://whitesource.atlassian.net/wiki/spaces/WD/pages/33718339/Unified+Agent
log_group_start "whitesource-agent-download" "command"
curl -L "https://s3.amazonaws.com/unified-agent/${WSS_JAR_FILE}" -O
log_group_end "whitesource-agent-download" "command"

# Retrieve the secret token from vault and use that to talk to Whitesource.
log_group_start "whitesource-retrieve-api-key" "command"
WSS_API_KEY="$(imp-ci secrets read --secret-type="${WSS_SECRET_TYPE}" --secret-name="${WSS_SECRET_NAME}" --environment="${WSS_SECRET_ENV}" --buildkite-org="${WSS_SECRET_BK_ORG}" --field=secretToken)"
log_group_end "whitesource-retrieve-api-key" "command"

# Run the actual scan and report results to the Improbable Whitesouce console.
log_group_start "whitesource-scan" "command"
echo "Running WhiteSource Unified Agent. Expect no output because it is evil."
java -jar "${WSS_JAR_FILE}" \
  -apiKey "${WSS_API_KEY}" \
  -wss.url "${WSS_URL}" \
  -project "${WSS_PROJECT_NAME}" \
  -d "${SCAN_TARGET}" \
  -scanComment "${SCAN_COMMENT}"
log_group_end "whitesource-scan" "command"
