#!/bin/bash
# KTPReAPI Linux Build Script
# Run this on your Ubuntu server or via WSL

set -e  # Exit on error

echo "========================================"
echo "KTPReAPI Linux Build Script"
echo "========================================"
echo "Working directory: $(pwd)"

# Check for required tools
echo "Checking for required tools..."

if ! command -v cmake &> /dev/null; then
    echo "ERROR: cmake is not installed. Install with: sudo apt-get install cmake"
    exit 1
fi

if ! command -v gcc &> /dev/null; then
    echo "ERROR: gcc is not installed. Install with: sudo apt-get install build-essential g++-multilib"
    exit 1
fi

# Check for 32-bit build support
if ! dpkg --print-foreign-architectures 2>/dev/null | grep -q i386; then
    echo "WARNING: 32-bit architecture support may not be enabled"
    echo "You may need to run: sudo dpkg --add-architecture i386 && sudo apt-get update"
fi

echo "All required tools found!"
echo ""

# Make sure scripts have unix line endings and are executable
sed -i 's/\r$//' build.sh
sed -i 's/\r$//' reapi/version/appversion.sh
chmod +x build.sh
chmod +x reapi/version/appversion.sh

# Success is judged on the ARTIFACT, never on `build/` existing.
#
# This wrapper used to do `if [ -d "build" ]` — but build.sh does `rm -rf build;
# mkdir build` up front and then `exit 0` unconditionally (upstream ReAPI code, at
# its `main $*` tail), so `set -e` never fires and the directory always exists.
# A failed compile therefore printed "BUILD COMPLETE!" AND "Files staged at:",
# while the `cp` silently never ran and the PREVIOUS reapi_ktp_i386.so stayed in
# the staging tree — ready to be shipped fleet-wide as `.new` believing it was new.
# Fleet md5 verification cannot catch that, because the md5 is taken from the very
# same stale file. Do not "simplify" this back to a directory check.
#
# We do not patch build.sh's `exit 0`: it is upstream, and patching it costs
# merge-ability. Instead we (a) still honour its exit code should upstream ever fix
# it, and (b) require a reapi_ktp_i386.so that is NEWER than this run — which also
# catches a stale .so left behind in build/.

BUILD_STAMP="$(mktemp)"   # reference mtime: anything older than this is not ours
trap 'rm -f "$BUILD_STAMP"' EXIT

echo "Building ReAPI..."
set +e
bash build.sh -j=$(nproc)
BUILD_RC=$?
set -e
if [ "$BUILD_RC" -ne 0 ]; then
    echo ""
    echo "========================================"
    echo "BUILD FAILED! (build.sh exit $BUILD_RC)"
    echo "========================================"
    exit 1
fi

# The real gate: a freshly-produced artifact.
REAPI_SO=$(find build -name "reapi_ktp_i386.so" -newer "$BUILD_STAMP" 2>/dev/null | head -1)
if [ -z "$REAPI_SO" ] || [ ! -f "$REAPI_SO" ]; then
    STALE=$(find build -name "reapi_ktp_i386.so" 2>/dev/null | head -1)
    echo ""
    echo "========================================"
    echo "BUILD FAILED!"
    echo "========================================"
    if [ -n "$STALE" ]; then
        echo "A reapi_ktp_i386.so exists but predates this run — the compile did not"
        echo "produce it. Refusing to stage a stale artifact."
    else
        echo "No reapi_ktp_i386.so was produced. Check the compiler output above."
    fi
    echo "NOTE: build.sh exits 0 even on a failed compile, so its exit code proves"
    echo "      nothing. Nothing has been staged."
    exit 1
fi

echo ""
echo "========================================"
echo "BUILD COMPLETE!"
echo "========================================"
echo "Build output in: $(pwd)/build"
echo ""
echo "Built files:"
find build -name "*.so" 2>/dev/null | head -20

# Deploy to staging folder
DEPLOY_DIR="/mnt/n/Nein_/KTP Git Projects/KTP DoD Server/serverfiles"
if [ -d "$DEPLOY_DIR" ]; then
    echo ""
    echo "Deploying to staging folder..."
    mkdir -p "$DEPLOY_DIR/dod/addons/ktpamx/modules"
    if ! cp "$REAPI_SO" "$DEPLOY_DIR/dod/addons/ktpamx/modules/"; then
        echo "ERROR: failed to copy $REAPI_SO into the staging tree."
        exit 1
    fi
    echo "  -> Copied reapi_ktp_i386.so  (md5 $(md5sum "$REAPI_SO" | cut -d' ' -f1))"
    echo ""
    # Printed only when a copy actually happened — it used to print regardless.
    echo "Files staged at: $DEPLOY_DIR/dod/addons/ktpamx/"
else
    echo ""
    echo "Staging folder not found: $DEPLOY_DIR"
    echo "(build succeeded; nothing staged)"
fi
