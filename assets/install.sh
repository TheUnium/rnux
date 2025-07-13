#!/bin/bash

# thanks gemini
set -eo pipefail

readonly RNUX_BUILD_DIR="$HOME/.rnux/build"
readonly TIMELIB_REPO="https://github.com/TheUnium/timelib.git"
readonly RNUX_REPO="https://github.com/TheUnium/rnux.git"
readonly INSTALL_PREFIX="/usr/local"
readonly LOCAL_INSTALL_DIR="$HOME/.local/bin"
readonly DESKTOP_ENTRY_DIR="$HOME/.local/share/applications"

readonly C_RESET='\033[0m'
readonly C_RED='\033[0;31m'
readonly C_GREEN='\033[0;32m'
readonly C_YELLOW='\033[1;33m'
readonly C_BLUE='\033[0;34m'

msg() {
    echo -e "${1}[${2}]${C_RESET} ${3}"
}

info() { msg "${C_BLUE}" "INFO" "$1"; }
success() { msg "${C_GREEN}" "SUCCESS" "$1"; }
warn() { msg "${C_YELLOW}" "WARNING" "$1"; }
error() { msg "${C_RED}" "ERROR" "$1"; }
die() {
    error "$1" >&2
    exit 1
}

check_root() {
    if [[ "$EUID" -eq 0 ]]; then
        die "This script must not be run as root. It will request sudo permissions when necessary."
    fi
}

detect_os() {
    if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        OS="${ID}"
        VERSION="${VERSION_ID}"
        info "Detected OS: ${OS} ${VERSION}"
    else
        die "Could not detect OS. The /etc/os-release file was not found."
    fi
}

install_dependencies() {
    info "Installing dependencies for ${OS}..."
    case "${OS}" in
        ubuntu|debian)
            sudo apt-get update
            sudo apt-get install -y build-essential cmake git qt6-base-dev qt6-base-dev-tools \
                libqt6core6 libqt6gui6 libqt6widgets6 libqt6network6 libx11-dev libxcb1-dev \
                libxcb-keysyms1-dev pkg-config libcurl4-openssl-dev curl
            ;;
        fedora|rhel|centos|rocky|almalinux)
            if [[ "${OS}" == "centos" || "${OS}" == "rhel" || "${OS}" == "rocky" || "${OS}" == "almalinux" ]]; then
                [[ "${VERSION}" == "8"* ]] && sudo dnf config-manager --set-enabled powertools &>/dev/null || true
                [[ "${VERSION}" == "9"* ]] && sudo dnf config-manager --set-enabled crb &>/dev/null || true
            fi
            sudo dnf install -y gcc-c++ cmake git qt6-qtbase-devel qt6-qtbase libX11-devel \
                xcb-util-keysyms-devel libxcb-devel pkgconfig libcurl-devel curl
            ;;
        arch|manjaro)
            sudo pacman -Syu --noconfirm base-devel cmake git qt6-base libx11 xcb-util-keysyms \
                libxcb pkgconf curl
            ;;
        opensuse*)
            sudo zypper install -y gcc-c++ cmake git qt6-base-devel libX11-devel \
                xcb-util-keysyms-devel libxcb-devel pkg-config libcurl-devel curl
            ;;
        *)
            die "Your OS (${OS}) is not supported by this script. Please install dependencies manually."
            ;;
    esac
    success "Dependencies installed."
}

# args: $1 - repo url, $2 - destination dir
clone_or_update_repo() {
    local repo_url="$1"
    local dest_dir="$2"
    local repo_name
    repo_name=$(basename "${dest_dir}")

    info "Handling ${repo_name} repository..."
    if [[ -d "${dest_dir}" ]]; then
        info "Updating ${repo_name}..."
        (cd "${dest_dir}" && git pull)
    else
        info "Cloning ${repo_name}..."
        git clone "${repo_url}" "${dest_dir}"
    fi
}

build_timelib() {
    local timelib_dir="${RNUX_BUILD_DIR}/timelib"
    info "Building timelib..."
    cd "${timelib_dir}"

    git submodule update --init --recursive

    local build_dir="${timelib_dir}/build"
    mkdir -p "${build_dir}"
    cd "${build_dir}"

    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
        -DUSE_SYSTEM_TZ_DB=ON \
        -DBUILD_SHARED_LIBS=OFF

    make "-j$(nproc)"
    info "Installing timelib (sudo required)..."
    sudo make install
    command -v ldconfig &>/dev/null && sudo ldconfig
    success "timelib built and installed."
}

build_rnux() {
    local rnux_dir="${RNUX_BUILD_DIR}/rnux"
    info "Building rnux..."

    local build_dir="${rnux_dir}/build"
    mkdir -p "${build_dir}"
    cd "${build_dir}"

    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="${INSTALL_PREFIX}" \
        -Dtimelib_DIR="${INSTALL_PREFIX}/lib/cmake/timelib"

    make "-j$(nproc)"
    success "rnux built successfully. Binary is at: ${build_dir}/rnux"
}

install_rnux() {
    info "Installing rnux..."
    mkdir -p "${LOCAL_INSTALL_DIR}"
    cp "${RNUX_BUILD_DIR}/rnux/build/rnux" "${LOCAL_INSTALL_DIR}/"
    chmod +x "${LOCAL_INSTALL_DIR}/rnux"
    success "rnux installed to ${LOCAL_INSTALL_DIR}/rnux"

    if [[ ":$PATH:" != *":${LOCAL_INSTALL_DIR}:"* ]]; then
        warn "${LOCAL_INSTALL_DIR} is not in your PATH."
        info "Add the following line to your ~/.bashrc or ~/.zshrc:"
        info "  export PATH=\"${LOCAL_INSTALL_DIR}:\$PATH\""
    fi
}

create_desktop_entry() {
    info "Creating desktop entry..."
    mkdir -p "${DESKTOP_ENTRY_DIR}"
    cat > "${DESKTOP_ENTRY_DIR}/rnux.desktop" << EOF
[Desktop Entry]
Name=rnux
Comment=Application launcher and productivity tool
Exec=${LOCAL_INSTALL_DIR}/rnux
Icon=utilities-terminal
Terminal=false
Type=Application
Categories=Utility;
EOF
    success "Desktop entry created."
}

clean_all() {
    info "Cleaning build directories and previous installations..."
    rm -rf "${RNUX_BUILD_DIR}"
    sudo rm -rf "${INSTALL_PREFIX}/include/time.hpp" \
                 "${INSTALL_PREFIX}/include/location.hpp" \
                 "${INSTALL_PREFIX}/include/zones.hpp" \
                 "${INSTALL_PREFIX}/include/date" \
                 "${INSTALL_PREFIX}/lib/cmake/timelib" \
                 "${INSTALL_PREFIX}/lib/libtimelib.a"
    rm -f "${LOCAL_INSTALL_DIR}/rnux"
    rm -f "${DESKTOP_ENTRY_DIR}/rnux.desktop"
    success "Cleanup complete."
}

show_help() {
    cat << EOF
rnux Build and Installation Script

This script builds and installs the rnux application and its dependencies.

Build directory: ${RNUX_BUILD_DIR}

Usage:
  ./install.sh [COMMAND]

Commands:
  (no command)   Run the full installation (default).
  deps-only      Install dependencies and exit.
  build-only     Build and install rnux, skipping dependency checks.
  clean          Remove all build files and installed components.
  help           Show this help message.
EOF
}

main() {
    check_root
    detect_os

    case "${1:-}" in
        "")
            info "Starting full rnux installation..."
            install_dependencies
            mkdir -p "${RNUX_BUILD_DIR}"
            clone_or_update_repo "${TIMELIB_REPO}" "${RNUX_BUILD_DIR}/timelib"
            clone_or_update_repo "${RNUX_REPO}" "${RNUX_BUILD_DIR}/rnux"
            build_timelib
            build_rnux
            install_rnux
            create_desktop_entry
            success "Installation complete!"
            info "Run 'rnux' from your terminal or find it in your applications menu."
            ;;
        deps-only)
            install_dependencies
            ;;
        build-only)
            info "Starting build-only process..."
            if [[ ! -d "${RNUX_BUILD_DIR}/timelib" || ! -d "${RNUX_BUILD_DIR}/rnux" ]]; then
                die "Repositories not found. Run a full installation first."
            fi
            build_timelib
            build_rnux
            install_rnux
            create_desktop_entry
            success "Build and installation complete!"
            ;;
        clean)
            clean_all
            ;;
        help|--help|-h)
            show_help
            ;;
        *)
            error "Unknown command: $1"
            show_help
            exit 1
            ;;
    esac
}

main "$@"