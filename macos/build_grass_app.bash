#!/bin/bash

############################################################################
#
# TOOL:         build_grass_app.bash
# AUTHOR(s):    Nicklas Larsson
# PURPOSE:      Build and bundle GRASS GIS app for macOS
# COPYRIGHT:    (c) 2020-2025 Nicklas Larsson and the GRASS Development Team
#               (c) 2020 Michael Barton
#               (c) 2018 Eric Hutton, Community Surface Dynamics Modeling
#                   System
#
# This package is written by Nicklas Larsson and is heavily based
# on work by Eric Hutton with contributions by Michael Barton.
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
#############################################################################

this_script=$(basename "$0")
this_script_dir=$(cd "$(dirname "$0")" || exit; pwd)
arch=$(uname -m)
cache_dir="${this_script_dir}/cache"
config_home="${HOME}/.config"
sdk=
grassdir=$(cd "${this_script_dir}/.." || exit; pwd)
deployment_target=
grass_version=""
grass_version_major=""
grass_version_minor=""
grass_version_release=
grass_version_date=
patch_dir=
grass_app_name=""
grass_app_bundle=""
conda_stable_file="${this_script_dir}/files/conda-requirements-stable-${arch}.txt"
conda_dev_file="${this_script_dir}/files/conda-requirements-dev-${arch}.txt"
conda_req_file="$conda_stable_file"
conda_temp_dir=$(mktemp -d -t GRASS)
conda_bin="${conda_temp_dir}/bin/conda"
conda_update_stable=0
miniconda_url="https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-MacOSX-${arch}.sh"
dmg_title=
dmg_name=
dmg_out_dir=
bundle_version=
repackage=0
with_liblas=0
cs_entitlements="${this_script_dir}/files/grass.entitlements"
cs_ident=
cs_keychain_profile=
cs_provisionprofile=
notarize=0

bash=/bin/bash
codesign=/usr/bin/codesign
ditto=/usr/bin/ditto
install_name_tool=/usr/bin/install_name_tool
xcrun=/usr/bin/xcrun

# patch needed for GRASS 8.0+
IFS='' read -r -d '' inst_dir_patch <<'EOF'
--- include/Make/Platform.make.in.orig
+++ include/Make/Platform.make.in
@@ -37,7 +37,7 @@
 exec_prefix         = @exec_prefix@
 ARCH                = @host@
 UNIX_BIN            = @BINDIR@
-INST_DIR            = @INSTDIR@
+INST_DIR            = @exec_prefix@

 GRASS_HOME          = @GRASS_HOME@
 RUN_GISBASE         = @GISBASE@
EOF


# read in configurations
if [ -n "$XDG_CONFIG_HOME" ]; then
    config_home="$XDG_CONFIG_HOME"
fi
if [ -f "${config_home}/grass/configure-build-${arch}.sh" ]; then
    source "${config_home}/grass/configure-build-${arch}.sh"
fi

#############################################################################
# Functions
#############################################################################

function display_usage () { cat <<- _EOF_

GRASS GIS build script for Anaconda.

Description...

Usage:  $this_script [arguments]
Arguments:
  -s
  --sdk         [path]  MacOS SDK - full path to the SDK, which will be set as
                        -isysroot, required, spaces in path not allowed
  -t
  --target    [target]  Set deployment target version (MACOSX_DEPLOYMENT_TARGET),
                        e.g. "10.14", optional, default is set from SDK
  -o
  --dmg-out-dir [path]  Output directory path for DMG file creation
                        This is a requirement for creating .dmg files.
  -c
  --conda-file  [path]  Conda package requirement file, optional.
  --with-liblas         Include libLAS support, optional, default is no support.
  -u
  --update-conda-stable Update the stable explicit conda requirement file. This
                        is only allowed if conda-requirements-dev-[arm64|x86_64].txt
                        is used (with --conda-file), to keep the two files in sync.
  -r
  --repackage           Recreate dmg file from previously built app,
                        setting [-o | --dmg-out-dir] is a requirement.
  --notarize            Code sign and notarize app and dmg for distribution
                        Setting Apple developer ID, keychain profile, and
                        provisionprofile is a prerequisite.
  -h
  --help                Usage information

Example:
  ./$this_script
  ./$this_script -s /Library/Developer/CommandLineTools/SDKs/MacOSX10.14.sdk

_EOF_
}

function printtag() {
    # GitHub Actions tag format
    [[ "$CI" == "true" ]] && echo "::$1::${2-}"
}

function begingroup() {
    printtag "group" "$1"
}

function endgroup() {
    printtag "endgroup"
}

function exit_nice () {
    error_code=$1
    if [[ "$#" -eq 2 && $2 = "cleanup" ]]; then
        rm -rf "$conda_temp_dir"
    fi
    exit "$error_code"
}

function read_grass_version () {
    begingroup "Read GRASS version"
    local versionfile="${grassdir}/include/VERSION"
    local arr=()
    while read -r line; do
        arr+=("$line")
    done < "$versionfile"
    grass_version_major=${arr[0]}
    grass_version_minor=${arr[1]}
    grass_version_release=${arr[2]}
    grass_version_date=${arr[3]}
    grass_version="${grass_version_major}.${grass_version_minor}.${grass_version_release}"
    patch_dir="${this_script_dir}/patches/${grass_version}"
    grass_app_name="GRASS-${grass_version_major}.${grass_version_minor}.app"
    grass_app_bundle="/Applications/${grass_app_name}"
    dmg_title="GRASS-GIS-${grass_version}"
    dmg_name="grass-${grass_version}-${arch}.dmg"
    echo "GRASS_VERSION: ${grass_version}"
    endgroup
}

# This set the build version for CFBundleVersion, in case of dev version the
# git short commit hash number is added.
function set_bundle_version () {
    begingroup "Set bundle version"
    pushd "$grassdir" > /dev/null || exit
    bundle_version=$grass_version

    local is_git_repo
    is_git_repo=$(git rev-parse --is-inside-work-tree 2> /dev/null)
    if [[ ! $? -eq 0 && ! "$is_git_repo" = "true" ]]; then
        popd > /dev/null || exit
        return
    fi

    if [[ "$grass_version_release" = *"dev"* ]]; then
        local git_commit
        git_commit=$(git rev-parse --short HEAD)
        bundle_version="${bundle_version} \(${git_commit}\)"
    fi
    popd > /dev/null || exit
    echo "BUNDLE_VERSION: ${bundle_version}"
    endgroup
}

function make_app_bundle_dir () {
    begingroup "Make app bundle dir"
    local contents_dir="${grass_app_bundle}/Contents"
    local resources_dir="${contents_dir}/Resources"
    local macos_dir="${contents_dir}/MacOS"
    local grass_bin_in="grass.sh.in"
    mkdir -p "$resources_dir"
    chmod 0755 "$grass_app_bundle" "$contents_dir" "$resources_dir"
    mkdir -m 0755 "$macos_dir"

    local info_plist_in="${grassdir}/macos/files/Info.plist.in"

    sed "s|@GRASS_VERSION_DATE@|${grass_version_date}|g" "$info_plist_in" | \
        sed "s|@GRASS_VERSION_MAJOR@|${grass_version_major}|g" | \
        sed "s|@GRASS_VERSION_MINOR@|${grass_version_minor}|g" | \
        sed "s|@GRASS_VERSION_RELEASE@|${grass_version_release}|g" | \
        sed "s|@BUNDLE_VERSION@|${bundle_version}|g" | \
        sed "s|@DEPLOYMENT_TARGET@|${deployment_target}|g" \
            > "$contents_dir/Info.plist"

    local grassbin="grass"
    sed "s|@GRASSBIN@|$grassbin|g" \
        "${this_script_dir}/files/$grass_bin_in" > "${macos_dir}/grass.sh"
    cp -p "${this_script_dir}/files/grass.scpt" "${macos_dir}/grass.scpt"
    cp -p "${grassdir}/macos/files/AppIcon.icns" "${resources_dir}/AppIcon.icns"
    cp -p "${grassdir}/macos/files/GRASSDocument_gxw.icns" \
        "${resources_dir}/GRASSDocument_gxw.icns"

    chmod 0644 "${contents_dir}/Info.plist"
    chmod 0755 "${macos_dir}/grass.sh"
    chmod 0644 "${resources_dir}/AppIcon.icns"
    chmod 0644 "${resources_dir}/GRASSDocument_gxw.icns"

    # swiftc -v "${this_script_dir}/files/main.swift" \
    #     -sdk "$sdk" \
    #     -target "${arch}-apple-macos${deployment_target}" \
    #     -o "${macos_dir}/GRASS"
    clang -x objective-c "-mmacosx-version-min=${deployment_target}" \
        -target "${arch}-apple-macos${deployment_target}" \
        -mmacosx-version-min="$deployment_target"  \
        -isysroot "$sdk" -fobjc-arc -Os \
        -o "${macos_dir}/GRASS" "${this_script_dir}/files/main.m" || exit_nice 1
    echo "GRASS_APP_BUNDLE created: $grass_app_bundle"
    endgroup
}

function patch_grass () {
    begingroup "Apply patches"
    echo "$inst_dir_patch" | patch -d "$grassdir" -p0
    endgroup
}

function reset_grass_patches () {
    begingroup "Reverting patches"
    echo "Reverting patches..."
    echo "$inst_dir_patch" | patch -d "$grassdir" -p0 -R
    echo "Reverting patches done."
    endgroup
}

function set_up_conda () {
    begingroup "Set up Conda"
    mkdir -p "$cache_dir"
    # move existing miniconda script to new external directory
    if [ -f "${this_script_dir}/miniconda3.sh" ]; then
        mv "${this_script_dir}/miniconda3.sh" "${cache_dir}/miniconda3.sh"
    fi

    # download miniconda if not already existing
    local miniconda="${cache_dir}/miniconda3-${arch}.sh"
    if [ ! -f "$miniconda" ]; then
        curl -L "$miniconda_url" --output "$miniconda" || exit_nice 1 cleanup
    fi

    $bash "$miniconda" -b -f -p "$conda_temp_dir"
    if [ ! -f "$conda_bin" ]; then
        echo "Error, could not find conda binary file at ${conda_bin}"
        exit_nice 1 cleanup
    fi

    $conda_bin create --yes -p "${grass_app_bundle}/Contents/Resources" \
        --file="${conda_req_file}" -c conda-forge || exit_nice 1 cleanup

    export PATH="${grass_app_bundle}/Contents/Resources/bin:$PATH"

    # remove, causing Notarization issues
    rm -rf "${grass_app_bundle}/Contents/Resources/share/gdb"
    endgroup
}

function install_grass_session () {
    begingroup "Install grass-session"
    local python_bin="${grass_app_bundle}/Contents/Resources/bin/python"
    $python_bin -m pip install --upgrade pip
    $python_bin -m pip install grass-session
    endgroup
}

function create_dmg () {
    begingroup "Create dmg"
    echo
    echo "Create dmg file of $grass_app_bundle ..."

    if [ ! -d  "$grass_app_bundle" ]; then
        echo "Error, attempt to create dmg file, but no app could be found"
        exit_nice 1
    fi

    local dmg_size
    local dmg_tmpfile
    local exact_app_size
    local tmpdir

    if [[ "$CI" == "true" ]]; then
        tmpdir="$RUNNER_TEMP"
    else
        tmpdir=$(mktemp -d /tmp/org.osgeo.grass.XXXXXX)
    fi
    dmg_tmpfile=${tmpdir}/grass-tmp-$$.dmg
    exact_app_size=$(du -ks "$grass_app_bundle" | cut -f 1)
    dmg_size=$((exact_app_size*120/100))

    if [[ "$CI" == "true" ]]
    then
        # workaround for sometimes failed attempts on macos-13 (x86_64) runner
        local max_attempts=10
        local i=0
        until hdiutil create -srcfolder "$grass_app_bundle" \
                -volname "$dmg_title" \
                -fs HFS+ \
                -fsargs "-c c=64,a=16,e=16" \
                -format UDRW \
                -size ${dmg_size}k "$dmg_tmpfile"
        do
          if [ $i -eq $max_attempts ]; then
            echo "Error: hdiutil failed after ${max_attempts} attempts."
            exit 1
          fi
          i=$((i+1))
        done
    else
        if ! hdiutil create -srcfolder "$grass_app_bundle" \
                -volname "$dmg_title" \
                -fs HFS+ \
                -fsargs "-c c=64,a=16,e=16" \
                -format UDRW \
                -size ${dmg_size}k "$dmg_tmpfile"
        then
            rm -rf "$tmpdir"
            exit_nice 1
        fi
    fi

    if [[ "$CI" != "true" ]]; then
        DEVICE=$(hdiutil attach -readwrite -noverify -noautoopen "${dmg_tmpfile}" | grep -E '^/dev/' | sed -e "s/^\/dev\///g" -e 1q  | awk '{print $1}')
        hdiutil attach "$dmg_tmpfile" || error "Can't attach temp DMG"

        mkdir -p "/Volumes/${dmg_title}/.background"
        cp -p "${this_script_dir}/files/dmg-background.png" \
            "/Volumes/${dmg_title}/.background/background.png"

        osascript << EOF
tell application "Finder"
    tell disk "$dmg_title"
        open
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        set the bounds of container window to {400, 100, 1040, 460}
        set theViewOptions to the icon view options of container window
        set arrangement of theViewOptions to not arranged
        set icon size of theViewOptions to 100
        set background picture of theViewOptions to file ".background:background.png"
        make new alias file at container window to POSIX file "/Applications" with properties {name:"Applications"}
        set position of item "$grass_app_name" of container window to {187, 163}
        set position of item "Applications" of container window to {452, 163}
        update without registering applications
        delay 5
        close
    end tell
end tell
EOF

        sync
        sync
        sleep 3
        hdiutil detach "$DEVICE"
    fi

    DMG_FILE="${dmg_out_dir}/${dmg_name}"

    if ! hdiutil convert "${dmg_tmpfile}" \
        -format UDZO -imagekey zlib-level=9 -o "${DMG_FILE}"
    then
        [[ "$CI" != "true" ]] && rm -rf "$tmpdir"
        exit_nice 1
    fi

    [[ "$CI" != "true" ]] && rm -rf "$tmpdir"

    echo
    [[ "$CI" == "true" ]] && echo "DMG_NAME=${dmg_name}" >> "$GITHUB_ENV"
    [[ "$CI" == "true" ]] && echo "DMG_FILE=${DMG_FILE}" >> "$GITHUB_ENV"
    endgroup
}

function remove_dmg () {
    local disk
    if [ -d "/Volumes/${dmg_title}" ]; then
        disk=$(diskutil list | grep "$dmg_title" | awk -F\  '{print $NF}')
        diskutil unmount "$disk"
    fi
    rm -rf "${dmg_out_dir:?}/${dmg_name:?}"
}

function codesign_app () {
    local bins
    local grass_libs
    local libs
    begingroup "Code sign app"
    pushd "${grass_app_bundle}/Contents" > /dev/null || exit $?

    # remove build stage rpaths from grass libraries

    grass_libs=$(find ./Resources -type f \(  -name "*libgrass_*.dylib" \))

    while IFS= read -r file || [[ -n $file ]]; do
        rpath=$(otool -l "$file" | grep "dist.*/lib" | awk '{$1=$1};1' | cut -d " " -f 2)
        if [[ -n "$rpath" ]]; then
            "$install_name_tool" -delete_rpath "$rpath" "$file"
        fi
    done < <(printf '%s\n' "$grass_libs")

    # codesign embedded libraries

    libs=$(find ./Resources -type f \( -name "*.so" -or -name "*.dylib" \))

    while IFS= read -r file || [[ -n $file ]]; do
        "$codesign" --sign "${cs_ident}" --force --verbose --timestamp "${file}" &
    done < <(printf '%s\n' "$libs")
    wait

    # codesign embedded binaries

    bins=$(find ./Resources -type f -perm +111 ! \( -name "*.so" -or -name "*.dylib" -or -name "*.a" \
        -or -name "*.py" -or -name "*.sh" \) -exec file '{}' \; | \
        grep "x86_64\|arm64" | \
        cut -d ":" -f 1 | \
        grep -v "for architecture")
    wait

    while IFS= read -r file || [[ -n $file ]]; do
        "$codesign" --sign "${cs_ident}" --force --verbose --timestamp --options runtime \
          --entitlements "$cs_entitlements" "${file}" &
    done < <(printf '%s\n' "$bins")
    wait

    # codesign "extra" files in ./MacOS directory

    "$codesign" --sign "${cs_ident}" --force --verbose --timestamp --options runtime \
        --entitlements "$cs_entitlements" "./MacOS/grass.sh"
    "$codesign" --sign "${cs_ident}" --force --verbose --timestamp --options runtime \
        --entitlements "$cs_entitlements" "./MacOS/grass.scpt"

    cp "$cs_provisionprofile" embedded.provisionprofile
    # xattr -r -d com.apple.FinderInfo embedded.provisionprofile

    popd > /dev/null || exit $?

    # codesign the app bundle

    "$codesign" --force --verbose --timestamp --sign "$cs_ident" --options runtime \
        --entitlements "$cs_entitlements" "$grass_app_bundle"
    endgroup
}

function codesign_dmg () {
    begingroup "Code sign dmg"
    "$codesign" --force --verbose --timestamp --sign "$cs_ident" --options runtime \
        --entitlements "$cs_entitlements" "${dmg_out_dir}/${dmg_name}"
    endgroup
}

function notarize_app () {
    local tmpdir
    local zip_tmpfile

    begingroup "Notarize app"
    if [[ "$CI" == "true" ]]; then
        tmpdir="$RUNNER_TEMP"
    else
        tmpdir=$(mktemp -d /tmp/org.osgeo.grass.XXXXXX)
    fi
    zip_tmpfile="${tmpdir}/${grass_app_name}.zip"

    "$ditto" -c -k --keepParent "$grass_app_bundle" "$zip_tmpfile"

    "$xcrun" notarytool submit "$zip_tmpfile" \
        --keychain-profile "$cs_keychain_profile" --wait || exit 1

    "$xcrun" stapler staple "$grass_app_bundle"
    "$xcrun" stapler validate "$grass_app_bundle"

    if [[ "$CI" == "true" ]]; then
        rm -rf "$zip_tmpfile"
    else
        rm -rf "$tmpdir"
    fi
    endgroup
}

function notarize_dmg () {
    begingroup "Notarize dmg"
    "$xcrun" notarytool submit "${dmg_out_dir}/${dmg_name}" \
        --keychain-profile "$cs_keychain_profile" --wait

    "$xcrun" stapler staple "${dmg_out_dir}/${dmg_name}"
    endgroup
}

#############################################################################
# Read script arguments
#############################################################################

while [ "$1" != "" ]; do
    case $1 in
        -s | --sdk ) shift
        sdk=$1
        ;;
        -t | --target ) shift
        deployment_target=$1
        ;;
        -o | --dmg-out-dir ) shift
        dmg_out_dir=$1
        ;;
        -c | --conda-file ) shift
        conda_req_file=$1
        ;;
        --with-liblas )
        with_liblas=1
        ;;
        -r | --repackage )
        repackage=1
        ;;
        -u | --update-conda-stable )
        conda_update_stable=1
        ;;
        --notarize )
        notarize=1
        ;;
        -h | --help )
        display_usage
        exit 0
        ;;
        *)
         # unknown option
         echo "ERROR"
         display_usage
         exit 1
        ;;
    esac
    shift
done

#############################################################################
# Check arguments and files
#############################################################################

# make full path of CONDA_REQ_FILE
conda_req_file=$(cd "$(dirname "${conda_req_file}")" && pwd)/$(basename "$conda_req_file")

if [[ ! -d "$sdk" && -f "$xcrun" ]]; then
    sdk=$("$xcrun" --show-sdk-path)
fi

if [ ! -f  "${sdk}/SDKSettings.plist" ]; then
    echo "Error, could not find valid MacOS SDK at $sdk"
    display_usage
    exit_nice 1
fi

# if DEPLOYMENT_TARGET hasn't been set, extract from SDK
if [ -z "$deployment_target" ]; then
    deployment_target=$(plutil -extract DefaultProperties.MACOSX_DEPLOYMENT_TARGET xml1 \
-o - "${sdk}/SDKSettings.plist" | awk -F '[<>]' '/string/{print $3}')
fi

read_grass_version
set_bundle_version

if [[ ! -d  "$patch_dir" && "${grass_version_major}${grass_version_minor}" -le 80 ]]; then
    echo "Error, no patch directory \"$patch_dir\" found"
    exit_nice 1
fi

if [[ -n "$dmg_out_dir" && ! -d  "$dmg_out_dir" ]]; then
    echo "Error, dmg output directory \"${dmg_out_dir}\" does not exist."
    exit_nice 1
fi

if [[ -n "$dmg_out_dir" && -f "${dmg_out_dir}/${dmg_name}" ]]; then
    echo "Warning, there exists a dmg file \"${dmg_name}\" in \"${dmg_out_dir}\"."
    while true; do
        read -r -p "Do you wish to delete it (y|n)? " yn
        case $yn in
            [Yy]* ) remove_dmg; break;;
            [Nn]* ) exit_nice 0;;
            * ) echo "Please answer yes or no.";;
        esac
    done
fi

if [[ "$repackage" -eq 1 && ! -d  "$grass_app_bundle" ]]; then
    echo "Error, attempt to repackage a non-existing \"$grass_app_bundle\" app bundle."
    exit_nice 1
fi

# updating the stable conda explicit requirement file is only allowed if
# files/conda-requirements-dev-[arm64|x86_64].txt is used, to keep the two files in sync
if [[ "$conda_update_stable" -eq 1 ]]; then
    if [ "$conda_req_file" != "$conda_dev_file" ]; then
        echo "Note, conda requirement file used is ${conda_dev_file}!"
        conda_req_file="$conda_dev_file"
    fi
    grass_app_bundle=$(mktemp -d /tmp/org.osgeo.grass.XXXXXX)
fi

# check if destination app bundle exists, with option to cancel if true
if [[ -d  "$grass_app_bundle" && "$repackage" -eq 0 && "$conda_update_stable" -eq 0 ]]; then
    echo "Warning, \"${grass_app_bundle}\" already exists."
    while true; do
        read -r -p "Do you wish to delete it (y|n)? " yn
        case $yn in
            [Yy]* ) rm -rf "$grass_app_bundle"; break;;
            [Nn]* ) exit_nice 0;;
            * ) echo "Please answer yes or no.";;
        esac
    done
fi

if [[ "$notarize" -eq 1 && ( -z $cs_ident || -z $cs_keychain_profile || -z $cs_provisionprofile ) ]]; then
    echo "Error, attempt to notarize the app without setting necessary"
    echo "code signing identity, provision profile and keychain profile."
    exit_nice 1
fi

if [[ "$notarize" -eq 1 && ! -f $cs_provisionprofile ]]; then
    echo "Error, the provisioning profile file can not be found."
    exit_nice 1
fi

#############################################################################
# Start setting up and compiling procedures
#############################################################################

# only create a new dmg file of existing app bundle
if [[ -n "$dmg_out_dir" && "$repackage" -eq 1 ]]; then
    create_dmg
    if [[ "$notarize" -eq 1 ]]; then
        codesign_dmg
        notarize_dmg
    fi
    exit_nice 0
fi

make_app_bundle_dir

set_up_conda

# update the stable conda explicit (stable) requirement file and exit
if [[ "$conda_update_stable" -eq 1 ]]; then
    $conda_bin list -p "${grass_app_bundle}/Contents/Resources" \
        --explicit > "$conda_stable_file"
    rm -rf "$grass_app_bundle";
    exit_nice 0
fi

install_grass_session

if [[ "$with_liblas" -eq 1 ]]; then
    begingroup "Build and install libLAS"
    source "${this_script_dir}/files/liblas-install.sh"
    endgroup
fi

patch_grass

begingroup "Build and install GRASS GIS"
# configure and compile GRASS GIS

pushd "$grassdir" > /dev/null || exit

echo "Starting \"make distclean\"..."
make distclean &>/dev/null
echo "Finished \"make distclean\""

source "${this_script_dir}/files/configure-grass.sh"

if ! make -j"$(sysctl -n hw.ncpu)"
then
    echo "Compilation failed, you may need to reset the GRASS git repository."
    echo "This can be made with: \"cd [grass-source-dir] && git reset --hard\"."
    echo
    popd > /dev/null || exit
    exit_nice 1
fi

echo
echo "Start installation..."
if ! make install
then
    echo "Installation failed, you may need to reset the GRASS git repository."
    echo "This can be made with: \"cd [grass-source-dir] && git reset --hard\"."
    echo
    popd > /dev/null || exit
    exit_nice 1
fi
echo "Finished installation."

popd > /dev/null || exit
endgroup

reset_grass_patches

# replace SDK with a unversioned one of Command Line Tools
file="${grass_app_bundle}/Contents/Resources/include/Make/Platform.make"
sed -i .bak "s|-isysroot ${sdk}|-isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk|g" "$file" \
    || rm -f "${file}.bak"

begingroup "List of installed packages"
# print list of installed packages
echo "================================================================="
echo
$conda_bin list -p "${grass_app_bundle}/Contents/Resources"
if [[ "$with_liblas" -eq 1 ]]; then
    liblas_version=$("${grass_app_bundle}/Contents/Resources/bin/liblas-config" --version)
    echo "libLAS                    ${liblas_version}"
fi
echo
echo "================================================================="
endgroup

if [[ "$CI" == "true" ]]; then
    begingroup "Move app to RUNNER_TEMP"
    ditto "$grass_app_bundle" "${RUNNER_TEMP}${grass_app_bundle}"
    grass_app_bundle="${RUNNER_TEMP}${grass_app_bundle}"
    endgroup
fi

if [[ "$notarize" -eq 1 ]]; then
    codesign_app
    notarize_app
fi

# create dmg file
if [[ -n "$dmg_out_dir" ]]; then
    create_dmg
    if [[ "$notarize" -eq 1 ]]; then
        codesign_dmg
        notarize_dmg
    fi
fi

exit_nice 0 cleanup
