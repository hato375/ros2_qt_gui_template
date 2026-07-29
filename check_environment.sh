#!/usr/bin/env bash

set -uo pipefail

readonly PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly ROS_DISTRO_NAME="humble"
readonly ROS_ROOT="/opt/ros/${ROS_DISTRO_NAME}"

okCount=0
warningCount=0
errorCount=0

printOk() {
	printf '[OK]    %s\n' "$1"
	okCount=$((okCount + 1))
}

printWarning() {
	printf '[WARN]  %s\n' "$1"
	if [[ $# -ge 2 ]]; then
		printf '        %s\n' "$2"
	fi
	warningCount=$((warningCount + 1))
}

printError() {
	printf '[ERROR] %s\n' "$1"
	if [[ $# -ge 2 ]]; then
		printf '        %s\n' "$2"
	fi
	errorCount=$((errorCount + 1))
}

commandVersion() {
	local commandName="$1"
	shift

	if command -v "${commandName}" >/dev/null 2>&1; then
		local version
		version="$("${commandName}" "$@" 2>/dev/null | head -n 1)"
		printOk "${commandName}: ${version:-installed}"
	else
		printError "${commandName} was not found"
	fi
}

findQtCreator() {
	local configuredExecutable="${QT_CREATOR_EXECUTABLE:-}"
	if [[ -n "${configuredExecutable}" && -x "${configuredExecutable}" ]]; then
		printf '%s\n' "${configuredExecutable}"
		return
	fi

	local candidates=(
		"${PROJECT_ROOT}/../qtcreator-20.0.0/bin/qtcreator"
		"${PROJECT_ROOT}/../Qt/Tools/QtCreator/bin/qtcreator"
	)
	local candidate
	for candidate in "${candidates[@]}"; do
		if [[ -x "${candidate}" ]]; then
			printf '%s\n' "${candidate}"
			return
		fi
	done

	if command -v qtcreator >/dev/null 2>&1; then
		command -v qtcreator
	fi
}

printf 'ROS 2 + Qt GUI environment check\n'
printf 'Project: %s\n\n' "${PROJECT_ROOT}"

if command -v lsb_release >/dev/null 2>&1; then
	distributor="$(lsb_release -is 2>/dev/null)"
	release="$(lsb_release -rs 2>/dev/null)"
	if [[ "${distributor}" == "Ubuntu" && "${release}" == "22.04" ]]; then
		printOk "OS: Ubuntu ${release}"
	else
		printError "OS: ${distributor} ${release}" "Ubuntu 22.04 is required."
	fi
else
	printError "lsb_release was not found" "Unable to verify the Ubuntu version."
fi

if [[ -f "${ROS_ROOT}/setup.bash" ]]; then
	printOk "ROS 2 ${ROS_DISTRO_NAME}: ${ROS_ROOT}"
else
	printError "ROS 2 ${ROS_DISTRO_NAME} was not found" "Expected: ${ROS_ROOT}/setup.bash"
fi

if [[ -x "${ROS_ROOT}/bin/ros2" ]]; then
	printOk "ros2 command: ${ROS_ROOT}/bin/ros2"
else
	printError "ros2 command was not found in ${ROS_ROOT}/bin"
fi

commandVersion colcon version-check

if command -v rosdep >/dev/null 2>&1; then
	printOk "rosdep: $(command -v rosdep)"
else
	printError "rosdep was not found" "Install python3-rosdep."
fi

commandVersion cmake --version
commandVersion g++ --version
commandVersion gdb --version

if command -v g++ >/dev/null 2>&1; then
	if printf 'static_assert(__cplusplus >= 201703L); int main() { return 0; }\n' |
		g++ -std=c++17 -x c++ -fsyntax-only - >/dev/null 2>&1; then
		printOk "C++17 compiler support"
	else
		printError "The configured g++ does not support C++17"
	fi
fi

if command -v pkg-config >/dev/null 2>&1; then
	qtVersion="$(pkg-config --modversion Qt5Widgets 2>/dev/null || true)"
	if [[ -n "${qtVersion}" ]]; then
		printOk "Qt 5 Widgets: ${qtVersion}"
	else
		printError "Qt 5 Widgets development package was not found" "Install qtbase5-dev."
	fi
else
	printError "pkg-config was not found"
fi

qtCreatorExecutable="$(findQtCreator)"
if [[ -n "${qtCreatorExecutable}" ]]; then
	qtCreatorVersionOutput="$(
		QT_QPA_PLATFORM=offscreen timeout 10s "${qtCreatorExecutable}" -version 2>/dev/null || true
	)"
	qtCreatorVersion="$(printf '%s\n' "${qtCreatorVersionOutput}" | sed -n 's/^Version: /Version /p' | head -n 1)"
	printOk "Qt Creator ${qtCreatorVersion:-version unknown}: ${qtCreatorExecutable}"

	if printf '%s\n' "${qtCreatorVersionOutput}" | grep -qi 'rosprojectmanager'; then
		printOk "Qt Creator ROSProjectManager plugin"
	else
		printWarning \
			"Qt Creator ROSProjectManager plugin could not be verified" \
			"Check Help > About Plugins in Qt Creator."
	fi
else
	printError \
		"Qt Creator executable was not found" \
		"Set QT_CREATOR_EXECUTABLE to the executable path."
fi

shopt -s nullglob
workspaceFiles=("${PROJECT_ROOT}"/*.workspace)
packageFiles=("${PROJECT_ROOT}"/src/*/package.xml)
shopt -u nullglob

if [[ ${#workspaceFiles[@]} -eq 1 ]]; then
	printOk "Qt Creator workspace: ${workspaceFiles[0]}"
elif [[ ${#workspaceFiles[@]} -eq 0 ]]; then
	printError "No .workspace file was found in the project root"
else
	printError "Multiple .workspace files were found" "Keep exactly one file in the project root."
fi

if [[ ${#packageFiles[@]} -gt 0 ]]; then
	printOk "ROS 2 package files: ${#packageFiles[@]}"
else
	printError "No package.xml was found under src"
fi

if [[ -f "${PROJECT_ROOT}/install/setup.bash" ]]; then
	printOk "Workspace setup: ${PROJECT_ROOT}/install/setup.bash"
else
	printWarning \
		"Workspace has not been built" \
		"Run: source ${ROS_ROOT}/setup.bash && colcon build"
fi

installedExecutables=("${PROJECT_ROOT}"/install/*/lib/*/*)
executableFound=false
for installedExecutable in "${installedExecutables[@]}"; do
	if [[ -x "${installedExecutable}" && -f "${installedExecutable}" ]]; then
		executableFound=true
		break
	fi
done
if [[ "${executableFound}" == true ]]; then
	printOk "Built executable: ${installedExecutable}"
else
	printWarning "No built ROS 2 executable was found under install"
fi

if [[ -x "${PROJECT_ROOT}/open_qtcreator.sh" ]]; then
	printOk "Qt Creator launcher is executable"
else
	printError "open_qtcreator.sh is missing or not executable"
fi

if [[ -n "${DISPLAY:-}" || -n "${WAYLAND_DISPLAY:-}" ]]; then
	printOk "GUI session: ${XDG_SESSION_TYPE:-available}"
else
	printWarning \
		"No X11 or Wayland display was detected" \
		"GUI display requires DISPLAY or WAYLAND_DISPLAY."
fi

if command -v git >/dev/null 2>&1; then
	printOk "Git: $(git --version)"
else
	printError "Git was not found"
fi

if [[ -f "${PROJECT_ROOT}/.gitignore" ]]; then
	printOk ".gitignore"
else
	printWarning ".gitignore was not found"
fi

printf '\nSummary: %d OK, %d warning(s), %d error(s)\n' \
	"${okCount}" "${warningCount}" "${errorCount}"

if [[ ${errorCount} -gt 0 ]]; then
	printf 'Environment check failed. Resolve the errors above and run this script again.\n'
	exit 1
fi

printf 'Environment check passed.\n'
