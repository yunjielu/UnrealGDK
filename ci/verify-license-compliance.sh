#!/usr/bin/env bash
set -eET -o pipefail
[[ -n "${DEBUG:-}" ]] && set -x

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

run_in_docker() {
exec docker run \
  "${interactive_args[@]}" \
  -e="BUILD_MESSAGE=${BUILD_MESSAGE:-$BUILDKITE_MESSAGE}" \
  -e="BUILD_NUMBER=${BUILD_NUMBER:-$BUILDKITE_JOB_ID}" \
  -e="BUILD_URL=${BUILD_URL:-$BUILDKITE_BUILD_URL}" \
  -e="BUILDKITE_AGENT_ACCESS_TOKEN=${BUILDKITE_AGENT_ACCESS_TOKEN}" \
  -e="BUILDKITE_AGENT_ENDPOINT=${BUILDKITE_AGENT_ENDPOINT}" \
  -e="BUILDKITE_AGENT_META_DATA_ENVIRONMENT=${BUILDKITE_AGENT_META_DATA_ENVIRONMENT}" \
  -e="BUILDKITE_AGENT_META_DATA_QUEUE=${BUILDKITE_AGENT_META_DATA_QUEUE}" \
  -e="BUILDKITE_ARTIFACT_UPLOAD_DESTINATION=${BUILDKITE_ARTIFACT_UPLOAD_DESTINATION}" \
  -e="BUILDKITE_BRANCH=${BUILDKITE_BRANCH}" \
  -e="BUILDKITE_BUILD_CREATOR_EMAIL=${BUILDKITE_BUILD_CREATOR_EMAIL}" \
  -e="BUILDKITE_BUILD_CREATOR=${BUILDKITE_BUILD_CREATOR}" \
  -e="BUILDKITE_BUILD_ID=${BUILDKITE_BUILD_ID}" \
  -e="BUILDKITE_COMMIT=${BUILDKITE_COMMIT}" \
  -e="BUILDKITE_JOB_ID=${BUILDKITE_JOB_ID}" \
  -e="BUILDKITE_LABEL=${BUILDKITE_LABEL}" \
  -e="BUILDKITE_LOCAL_META=${BUILDKITE_LOCAL_META}" \
  -e="BUILDKITE_MESSAGE=${BUILDKITE_MESSAGE}" \
  -e="BUILDKITE_ORGANIZATION_SLUG=${BUILDKITE_ORGANIZATION_SLUG}" \
  -e="BUILDKITE_PARALLEL_JOB_COUNT=${BUILDKITE_PARALLEL_JOB_COUNT}" \
  -e="BUILDKITE_PARALLEL_JOB=${BUILDKITE_PARALLEL_JOB}" \
  -e="BUILDKITE_PIPELINE_SLUG=${BUILDKITE_PIPELINE_SLUG}" \
  -e="BUILDKITE_SOURCE=${BUILDKITE_SOURCE}" \
  -e="BUILDKITE=${BUILDKITE:-false}" \
  -e="LOCAL_USER_ID=$(id -u)" \
  --tmpfs /tmp:exec,mode=777,size=8G \
  --volume="${repo_base}:/code" \
  -w /code \
  "$1:latest" "${@:2}"
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

# Build the Docker container we need to run this inside of
exec docker build \
  -t "wss-license-scan" \
  --force-rm \
  --cache-from "wss-license-scan:latest" \
  "." \
  && docker tag "wss-license-scan:" "wss-license-scan:latest"


# Run the actual scan and report results to the Improbable Whitesouce console.
log_group_start "whitesource-scan" "command"
echo "Running WhiteSource Unified Agent. Expect no output because it is evil."
run_in_docker "wss-license-scan" java -jar "${WSS_JAR_FILE}" \
  -apiKey "${WSS_API_KEY}" \
  -wss.url "${WSS_URL}" \
  -project "${WSS_PROJECT_NAME}" \
  -d "${SCAN_TARGET}" \
  -scanComment "${SCAN_COMMENT}"
log_group_end "whitesource-scan" "command"
