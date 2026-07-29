#!/usr/bin/env bash

set -euo pipefail

readonly PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly ROS_SETUP="/opt/ros/humble/setup.bash"
readonly PROJECT_SETUP="${PROJECT_ROOT}/install/setup.bash"

qtCreatorExecutable="${QT_CREATOR_EXECUTABLE:-}"
if [[ -z "${qtCreatorExecutable}" ]]; then
	qtCreatorCandidates=(
		"${PROJECT_ROOT}/../qtcreator-20.0.0/bin/qtcreator"
		"${PROJECT_ROOT}/../Qt/Tools/QtCreator/bin/qtcreator"
	)

	for candidate in "${qtCreatorCandidates[@]}"; do
		if [[ -x "${candidate}" ]]; then
			qtCreatorExecutable="${candidate}"
			break
		fi
	done
fi

if [[ -z "${qtCreatorExecutable}" ]] && command -v qtcreator >/dev/null 2>&1; then
	qtCreatorExecutable="$(command -v qtcreator)"
fi

shopt -s nullglob
workspaceFiles=("${PROJECT_ROOT}"/*.workspace)
shopt -u nullglob

if [[ ${#workspaceFiles[@]} -ne 1 ]]; then
	echo "Error: Exactly one .workspace file must exist in ${PROJECT_ROOT}." >&2
	exit 1
fi
readonly WORKSPACE_FILE="${workspaceFiles[0]}"

if [[ ! -f "${ROS_SETUP}" ]]; then
	echo "Error: ROS 2 setup file was not found: ${ROS_SETUP}" >&2
	exit 1
fi

if [[ ! -f "${PROJECT_SETUP}" ]]; then
	echo "Error: Project setup file was not found: ${PROJECT_SETUP}" >&2
	echo "Run colcon build in ${PROJECT_ROOT} first." >&2
	exit 1
fi

if [[ ! -f "${WORKSPACE_FILE}" ]]; then
	echo "Error: Qt Creator workspace was not found: ${WORKSPACE_FILE}" >&2
	exit 1
fi

if [[ ! -x "${qtCreatorExecutable}" ]]; then
	echo "Error: Qt Creator executable was not found." >&2
	echo "Set QT_CREATOR_EXECUTABLE to the Qt Creator executable path." >&2
	exit 1
fi

# ROS 2のsetup.bashは未定義の環境変数を参照するため、読み込み中はnounsetを無効にする。
set +u
# shellcheck disable=SC1090
source "${ROS_SETUP}"
# shellcheck disable=SC1090
source "${PROJECT_SETUP}"
set -u

cd "${PROJECT_ROOT}"
exec "${qtCreatorExecutable}" "${WORKSPACE_FILE}"
