#!/usr/bin/env bash

set -euo pipefail

readonly PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly ROS_DISTRO_NAME="humble"
readonly ROS_SETUP="/opt/ros/${ROS_DISTRO_NAME}/setup.bash"
readonly TEMPLATE_NAME="ros2_qt_gui_template"
readonly PROJECT_SETUP_FILE="${PROJECT_ROOT}/.project_setup"

printSection() {
	printf '\n== %s ==\n' "$1"
}

printError() {
	printf 'Error: %s\n' "$1" >&2
}

confirm() {
	local prompt="$1"
	local defaultAnswer="${2:-n}"
	local answer
	local choice

	if [[ "${defaultAnswer}" == "y" ]]; then
		choice="[Y/n]"
	else
		choice="[y/N]"
	fi

	read -r -p "${prompt} ${choice} " answer
	answer="${answer:-${defaultAnswer}}"
	[[ "${answer}" =~ ^[Yy]$ ]]
}

requireCommand() {
	local commandName="$1"
	local installHint="$2"

	if ! command -v "${commandName}" >/dev/null 2>&1; then
		printError "${commandName} command was not found."
		printf 'Install: %s\n' "${installHint}" >&2
		exit 1
	fi
}

readRequiredValue() {
	local prompt="$1"
	local currentValue="$2"
	local value

	while true; do
		if [[ -n "${currentValue}" ]]; then
			read -r -p "${prompt} [${currentValue}]: " value
			value="${value:-${currentValue}}"
		else
			read -r -p "${prompt}: " value
		fi

		if [[ -n "${value}" ]]; then
			printf '%s\n' "${value}"
			return
		fi

		printf 'A value is required.\n' >&2
	done
}

initializeProject() {
	printSection "Project initialization"

	local projectName
	local configuredProjectName
	local templateWorkspace
	local projectWorkspace
	local readmeFile
	local setupGuideFile

	projectName="$(basename -- "${PROJECT_ROOT}")"
	if [[ ! "${projectName}" =~ ^[a-z][a-z0-9_]*$ ]]; then
		printError "Invalid project directory name: ${projectName}"
		printf 'Use lowercase letters, numbers, and underscores, starting with a letter.\n' >&2
		printf 'Example: ros2_test\n' >&2
		exit 1
	fi

	if [[ "${projectName}" == "${TEMPLATE_NAME}" ]]; then
		printError "Rename the template directory before running this script."
		printf 'Example:\n' >&2
		printf '  mv %s ros2_test\n' "${TEMPLATE_NAME}" >&2
		printf '  cd ros2_test\n' >&2
		printf '  ./setup_dev.sh\n' >&2
		exit 1
	fi

	if [[ -f "${PROJECT_SETUP_FILE}" ]]; then
		configuredProjectName="$(
			sed -n 's/^PROJECT_NAME=//p' "${PROJECT_SETUP_FILE}" | head -n 1
		)"
		if [[ "${configuredProjectName}" != "${projectName}" ]]; then
			printError "The project directory was renamed after initialization."
			printf 'Configured name: %s\n' "${configuredProjectName}" >&2
			printf 'Directory name:  %s\n' "${projectName}" >&2
			exit 1
		fi

		printf 'Project is already initialized: %s\n' "${projectName}"
		return
	fi

	templateWorkspace="${PROJECT_ROOT}/${TEMPLATE_NAME}.workspace"
	projectWorkspace="${PROJECT_ROOT}/${projectName}.workspace"
	readmeFile="${PROJECT_ROOT}/README.md"
	setupGuideFile="${PROJECT_ROOT}/docs/development_setup_guide.md"

	printf 'Detected project name: %s\n' "${projectName}"
	printf 'The following changes will be made:\n'
	printf '  %s -> %s\n' \
		"$(basename -- "${templateWorkspace}")" \
		"$(basename -- "${projectWorkspace}")"
	printf '  Replace workspace paths for %s with %s in project documentation.\n' \
		"${TEMPLATE_NAME}" "${projectName}"

	if ! confirm "Initialize this project?" "y"; then
		printError "Project initialization was canceled."
		exit 1
	fi

	if [[ -f "${templateWorkspace}" ]]; then
		mv "${templateWorkspace}" "${projectWorkspace}"
	elif [[ ! -f "${projectWorkspace}" ]]; then
		printError "Workspace file was not found: ${templateWorkspace}"
		exit 1
	fi

	if [[ ! -f "${readmeFile}" || ! -f "${setupGuideFile}" ]]; then
		printError "README.md or development_setup_guide.md was not found."
		exit 1
	fi

	sed -i \
		"s#/home/ros/${TEMPLATE_NAME}#/home/ros/${projectName}#g" \
		"${readmeFile}" "${setupGuideFile}"
	sed -i \
		"s/${TEMPLATE_NAME}\\.workspace/${projectName}.workspace/g" \
		"${setupGuideFile}"

	printf 'PROJECT_NAME=%s\n' "${projectName}" > "${PROJECT_SETUP_FILE}"
	printf 'TEMPLATE_NAME=%s\n' "${TEMPLATE_NAME}" >> "${PROJECT_SETUP_FILE}"
	printf 'Project initialized: %s\n' "${projectName}"
}

configureGit() {
	printSection "Git settings"

	if [[ ! -d "${PROJECT_ROOT}/.git" ]]; then
		printf 'Git repository is not initialized.\n'
		if confirm "Initialize a Git repository with the main branch?" "y"; then
			git -C "${PROJECT_ROOT}" init -b main
		else
			printError "Git repository initialization was canceled."
			exit 1
		fi
	fi

	local currentName
	local currentEmail
	local gitUserName
	local gitUserEmail

	currentName="$(git -C "${PROJECT_ROOT}" config --local user.name || true)"
	currentEmail="$(git -C "${PROJECT_ROOT}" config --local user.email || true)"

	gitUserName="$(readRequiredValue "Git user name" "${currentName}")"

	while true; do
		gitUserEmail="$(readRequiredValue "Git email address" "${currentEmail}")"
		if [[ "${gitUserEmail}" == *@*.* ]]; then
			break
		fi
		printf 'Enter a valid email address.\n' >&2
		currentEmail="${gitUserEmail}"
	done

	git -C "${PROJECT_ROOT}" config --local user.name "${gitUserName}"
	git -C "${PROJECT_ROOT}" config --local user.email "${gitUserEmail}"

	printf 'Configured for this repository:\n'
	printf '  user.name  = %s\n' "${gitUserName}"
	printf '  user.email = %s\n' "${gitUserEmail}"
}

installQtIfNeeded() {
	if pkg-config --exists Qt5Widgets; then
		printf 'Qt 5 Widgets: %s\n' "$(pkg-config --modversion Qt5Widgets)"
		return
	fi

	printf 'Qt 5 Widgets development package was not found.\n'
	if confirm "Install qtbase5-dev with apt?" "n"; then
		sudo apt update
		sudo apt install qtbase5-dev
	else
		printError "Qt 5 Widgets is required to build this project."
		exit 1
	fi
}

installRosDependencies() {
	printSection "ROS dependencies"

	if confirm "Resolve package dependencies with rosdep?" "y"; then
		rosdep install \
			--from-paths "${PROJECT_ROOT}/src" \
			--ignore-src \
			--rosdistro "${ROS_DISTRO_NAME}" \
			-y
	else
		printf 'Skipped rosdep.\n'
	fi
}

buildWorkspace() {
	printSection "Build"

	if confirm "Build the workspace with colcon?" "y"; then
		(
			set +u
			# shellcheck disable=SC1090
			source "${ROS_SETUP}"
			set -u
			cd "${PROJECT_ROOT}"
			colcon build
		)
	else
		printf 'Skipped colcon build.\n'
	fi
}

main() {
	printf 'ROS 2 + Qt GUI development setup\n'
	printf 'Project: %s\n' "${PROJECT_ROOT}"

	requireCommand git "sudo apt install git"
	requireCommand lsb_release "sudo apt install lsb-release"
	requireCommand pkg-config "sudo apt install pkg-config"
	requireCommand colcon "sudo apt install python3-colcon-common-extensions"
	requireCommand rosdep "sudo apt install python3-rosdep"
	requireCommand cmake "sudo apt install cmake"
	requireCommand g++ "sudo apt install build-essential"
	requireCommand gdb "sudo apt install gdb"

	if [[ "$(lsb_release -is)" != "Ubuntu" || "$(lsb_release -rs)" != "22.04" ]]; then
		printError "Ubuntu 22.04 is required."
		exit 1
	fi

	if [[ ! -f "${ROS_SETUP}" ]]; then
		printError "ROS 2 ${ROS_DISTRO_NAME} was not found: ${ROS_SETUP}"
		exit 1
	fi

	initializeProject
	configureGit

	printSection "Development dependencies"
	installQtIfNeeded
	installRosDependencies
	buildWorkspace

	printSection "Environment check"
	"${PROJECT_ROOT}/check_environment.sh"

	printf '\nSetup completed.\n'
	printf 'Start Qt Creator with:\n'
	printf '  %s/open_qtcreator.sh\n' "${PROJECT_ROOT}"
}

main "$@"
