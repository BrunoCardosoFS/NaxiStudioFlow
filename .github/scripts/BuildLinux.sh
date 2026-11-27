#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build_linux}"
CONFIG="${2:-Release}"

PROJECT_NAME="${PROGRAM_NAME:-NaxStudio Flow}"
VERSION_TAG="${VERSION:-v0.0.0}"
VERSION_MAJ="${VERSION_MAJOR:-0}"
VERSION_MIN="${VERSION_MINOR:-0}"
VERSION_PAT="${VERSION_PATCH:-0}"

mkdir -p "${DIST_FILES_DIR}" "${DIST_UBUNTU_DIR}"
ABS_DIST_FILES_DIR="$(cd "${DIST_FILES_DIR}" && pwd)"
ABS_DIST_UBUNTU_DIR="$(cd "${DIST_UBUNTU_DIR}" && pwd)"

echo "--- Starting CMake Configuration ---"
cmake -B "${BUILD_DIR}" -S . -G Ninja \
  -DCMAKE_BUILD_TYPE="${CONFIG}" \
  -DBUILD_TESTING=ON \
  -DAPP_VERSION="${VERSION_TAG}" \
  -DVERSION_MAJOR="${VERSION_MAJ}" \
  -DVERSION_MINOR="${VERSION_MIN}" \
  -DVERSION_PATCH="${VERSION_PAT}"

echo "--- Compiling ---"
cmake --build "${BUILD_DIR}" --config "${CONFIG}" --parallel

echo "--- Running Tests ---"
QT_QPA_PLATFORM=offscreen ctest --test-dir "${BUILD_DIR}" --output-on-failure

echo "--- Preparing Portable Layout (bin/ + data/) ---"
BIN_DIR="${ABS_DIST_UBUNTU_DIR}/bin"
LIB_DIR="${BIN_DIR}/lib"
PLUGINS_DIR="${BIN_DIR}/plugins"
DATA_DIR="${ABS_DIST_UBUNTU_DIR}/data"

rm -rf "${BIN_DIR}" "${DATA_DIR}"
mkdir -p "${BIN_DIR}" "${LIB_DIR}" "${PLUGINS_DIR}"
mkdir -p "${DATA_DIR}/blocks" "${DATA_DIR}/profiles" "${DATA_DIR}/logs" "${DATA_DIR}/config"

cat > "${DATA_DIR}/catalog.json" <<'EOF'
{
  "version": 1,
  "folders": [
    {
      "id": "comandos-playlist",
      "title": "Comandos",
      "type": "command",
      "items": [
        {
          "id": "pause",
          "title": "Pausa",
          "command": "pause"
        }
      ]
    }
  ]
}
EOF

cp "${BUILD_DIR}/flow" "${BIN_DIR}/flow"
chmod +x "${BIN_DIR}/flow"
cp "COPYING" "${ABS_DIST_UBUNTU_DIR}/COPYING"

QT_ROOT=""
if command -v qmake6 >/dev/null 2>&1; then
  QT_ROOT="$(qmake6 -query QT_INSTALL_PREFIX)"
elif command -v qmake >/dev/null 2>&1; then
  QT_ROOT="$(qmake -query QT_INSTALL_PREFIX)"
elif [ -n "${Qt6_DIR:-}" ]; then
  QT_ROOT="$(cd "${Qt6_DIR}/../../.." && pwd)"
fi

if [ -z "${QT_ROOT}" ] || [ ! -d "${QT_ROOT}" ]; then
  echo "Error: Could not locate Qt installation prefix." >&2
  exit 1
fi

echo "--- Bundling Qt Runtime from ${QT_ROOT} ---"
QT_PLUGINS_SRC="${QT_ROOT}/plugins"
QT_LIBS_SRC="${QT_ROOT}/lib"

REQUIRED_PLUGIN_DIRS=(
  platforms
  multimedia
  imageformats
  iconengines
  tls
  xcbglintegrations
  wayland-decoration-client
  wayland-graphics-integration-client
  wayland-shell-integration
)

for pdir in "${REQUIRED_PLUGIN_DIRS[@]}"; do
  if [ -d "${QT_PLUGINS_SRC}/${pdir}" ]; then
    mkdir -p "${PLUGINS_DIR}/${pdir}"
    cp -a "${QT_PLUGINS_SRC}/${pdir}/." "${PLUGINS_DIR}/${pdir}/"
  fi
done

copy_qt_dep() {
  local soname="$1"
  local candidate="${QT_LIBS_SRC}/${soname}"
  if [ ! -e "${candidate}" ]; then
    return 0
  fi
  if [ -e "${LIB_DIR}/${soname}" ]; then
    return 0
  fi

  cp -a "${candidate}" "${LIB_DIR}/"
  if [ -L "${candidate}" ]; then
    local real_target
    real_target="$(readlink -f "${candidate}")"
    local real_name
    real_name="$(basename "${real_target}")"
    if [ -e "${real_target}" ] && [ ! -e "${LIB_DIR}/${real_name}" ]; then
      cp -a "${real_target}" "${LIB_DIR}/${real_name}"
    fi
  fi
}

collect_deps_for_binary() {
  local bin="$1"
  LD_LIBRARY_PATH="${QT_LIBS_SRC}:${LIB_DIR}:${LD_LIBRARY_PATH:-}" ldd "${bin}" 2>/dev/null \
    | awk '/=>/ { if ($3 ~ /^\//) print $3; else print $1 }' \
    | while read -r dep_path; do
        local dep_name
        dep_name="$(basename "${dep_path}")"
        if [ -e "${QT_LIBS_SRC}/${dep_name}" ]; then
          copy_qt_dep "${dep_name}"
        fi
      done
}

collect_deps_for_binary "${BIN_DIR}/flow"
while IFS= read -r -d '' plugin_so; do
  collect_deps_for_binary "${plugin_so}"
done < <(find "${PLUGINS_DIR}" -type f -name "*.so" -print0)

# Iteratively resolve transitive Qt/FFmpeg dependencies inside bin/lib
prev_count=-1
while true; do
  curr_count="$(find "${LIB_DIR}" -maxdepth 1 \( -type f -o -type l \) | wc -l)"
  if [ "${curr_count}" -eq "${prev_count}" ]; then
    break
  fi
  prev_count="${curr_count}"
  while IFS= read -r -d '' lib_file; do
    collect_deps_for_binary "${lib_file}"
  done < <(find "${LIB_DIR}" -maxdepth 1 -type f -name "*.so*" -print0)
done

cat > "${BIN_DIR}/qt.conf" <<'EOF'
[Paths]
Prefix = .
Plugins = plugins
Libraries = lib
EOF

echo "--- Patching RPATHs ---"
patchelf --set-rpath '$ORIGIN/lib' "${BIN_DIR}/flow"
while IFS= read -r -d '' lib_file; do
  patchelf --set-rpath '$ORIGIN' "${lib_file}" || true
done < <(find "${LIB_DIR}" -maxdepth 1 -type f -name "*.so*" -print0)

while IFS= read -r -d '' plugin_so; do
  patchelf --set-rpath '$ORIGIN/../../lib' "${plugin_so}" || true
done < <(find "${PLUGINS_DIR}" -type f -name "*.so" -print0)

rm -rf "${BUILD_DIR}"

echo "--- Creating Portable Tarball ---"
TAR_NAME="${ABS_DIST_FILES_DIR}/${PROJECT_NAME}-${VERSION_TAG}-Portable-Linux-x86_64.tar.gz"
tar -czf "${TAR_NAME}" -C "${ABS_DIST_UBUNTU_DIR}" .
echo "Build and Packaging completed: ${TAR_NAME}"
