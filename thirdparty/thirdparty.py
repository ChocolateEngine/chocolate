import os
import sys
import argparse
import shutil
import subprocess
from enum import Enum, auto
from urllib.request import Request, urlopen
from zipfile import ZipFile
from typing import List, Dict

# -------------------------------------------------------------------------
# Script made to download thirdparty stuff quickly
# maybe i can improve this later to be able to build it for you automatically
# not everything uses cmake here anyway so
#
# TODO: split up into more files, this is already like a mini build system,
#  but very messy as it's one file
# -------------------------------------------------------------------------

try:
    import py7zr
except ImportError:
    print("Error importing py7zr: install with pip and run this again")
    quit(2)


class OS(Enum):
    Any = auto(),
    Windows = auto(),
    Linux = auto(),


# maybe obsolete
PUBLIC_DIR = "../public"
BUILD_DIR = "build"

if sys.platform in {"linux", "linux2"}:
    SYS_OS = OS.Linux
elif sys.platform == "win32":
    SYS_OS = OS.Windows
else:
    SYS_OS = None
    print("Error: unsupported/untested platform: " + sys.platform)
    quit(1)


if SYS_OS == OS.Windows:
    DLL_EXT = ".dll"
    EXE_EXT = ".exe"
    LIB_EXT = ".lib"
elif SYS_OS == OS.Linux:
    DLL_EXT = ".so"
    EXE_EXT = ""
    LIB_EXT = ".a"


VS_PATH = ""
VS_DEVENV = ""
VS_MSBUILD = ""
VS_MSVC = "v143"


def setup_vs_env():
    #cmd = "vswhere.exe -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe"
    cmd = "vswhere.exe -latest"

    global VS_PATH
    global VS_DEVENV
    global VS_MSBUILD

    vswhere = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
    for line in vswhere.stdout:
        if line.startswith("productPath"):
            VS_DEVENV = line[len("productPath: "):-1]
            print(f"Set VS_DEVENV to \"{VS_DEVENV}\"")

        elif line.startswith("installationPath"):
            VS_PATH = line[len("installationPath: "):-1]
            print(f"Set VS_PATH to \"{VS_PATH}\"")

    if VS_PATH == "" or VS_DEVENV == "":
        print("FATAL ERROR: Failed to find VS_DEVENV or VS_PATH?")
        quit(3)

    # VS_MSBUILD=C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\amd64\msbuild.exe
    VS_MSBUILD = VS_PATH + "\\Msbuild\\Current\\Bin\\amd64\\msbuild.exe"

    # C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\amd64\msbuild.exe

    return ""


def parse_args() -> argparse.Namespace:
    args = argparse.ArgumentParser()
    args.add_argument("-b", "--no-build", action="store_true", help="Don't build the projects")
    args.add_argument("-f", "--force", help="Force Run Everything")
    return args.parse_args()


# =================================================================================================


def post_ktx_extract():
    if ARGS.no_build:
        return

    os.chdir("KTX-Software")

    #print("Building KTX\n")

    if not os.path.isdir("build"):
        os.mkdir("build")
    #else:
    #    print("Build Dir exists, skipping")
    #    #return

    os.chdir("build")

    # initial setup
    os.system("cmake ..")

    print("Building KTX - Release\n")
    os.system("cmake --build . --config Release")

    print("Building KTX - Debug\n")
    os.system("cmake --build . --config Debug")

    os.chdir("../..")


# =================================================================================================


def post_jolt_extract():
    if ARGS.no_build:
        return

    os.chdir("JoltPhysics/Build")
    
    build_options = "-DUSE_AVX2=OFF -DTARGET_UNIT_TESTS=OFF -DTARGET_HELLO_WORLD=OFF -DTARGET_PERFORMANCE_TEST=OFF -DTARGET_SAMPLES=OFF -DTARGET_VIEWER=OFF"

    if SYS_OS == OS.Windows:
        build_dir = "build"
        os.system(f"cmake -S . -B {build_dir} -A x64 {build_options}")

        print("Building JoltPhysics - Release\n")
        os.system(f"cmake --build {build_dir} --config Release")

        print("Building JoltPhysics - Debug\n")
        os.system(f"cmake --build {build_dir} --config Debug")

    else:
        build_dir = "build"
        print("Building JoltPhysics - Release\n")
        os.system(f"cmake -S . -B {build_dir}/Release -DCMAKE_CXX_COMPILER=g++ -DCMAKE_POSITION_INDEPENDENT_CODE=ON {build_options}")
        os.system(f"cmake --build {build_dir}/Release --config Release")

        print("Building JoltPhysics - Debug\n")
        os.system(f"cmake -S . -B {build_dir}/Debug -DCMAKE_CXX_COMPILER=g++ -DCMAKE_POSITION_INDEPENDENT_CODE=ON {build_options}")
        os.system(f"cmake --build {build_dir}/Debug --config Debug")

    os.chdir("../..")


# =================================================================================================


def post_openal_extract():
    if ARGS.no_build:
        return

    os.chdir("openal-soft/build")

    os.system("cmake ..")

    print("Building OpenAL-Soft - Release\n")
    os.system("cmake --build . --config Release")

    print("Building OpenAL-Soft - Debug\n")
    os.system("cmake --build . --config Debug")

    os.chdir("../..")


# =================================================================================================


def vorbis_copy_built_files(proj: str, cfg: str, build_dir: str, out_dir: str):
    if not out_dir:
        return

    out_dir_cfg = out_dir + "\\" + cfg
    if not os.path.isdir(out_dir_cfg):
        os.makedirs(out_dir_cfg)

    proj_name = os.path.splitext(os.path.basename(proj))[0]
    item_path = build_dir + "\\" + cfg + "\\"
    out_path_base = out_dir_cfg + "\\"
    for file_ext in {".pdb", ".lib"}:
        # HACK:
        if proj_name == "libvorbisfile_static" and file_ext == ".pdb":
            copy_file = item_path + "libvorbisfile" + file_ext
            out_path = out_path_base + "libvorbisfile" + file_ext
        else:
            copy_file = item_path + proj_name + file_ext
            out_path = out_path_base + proj_name + file_ext

        shutil.copy(copy_file, out_path)
        # shutil.rmtree(build_dir)


def fix_ogg_proj(proj: str, old_msvc: str):
    path, name = os.path.split(proj)
    old_proj = path + "\\old_" + name

    if os.path.isfile(old_proj):
        return

    with open(proj, mode="r") as proj_io:
        vcxproj = proj_io.read()

    # set build tools in vcxproj to v143
    vcxproj = vcxproj.replace(old_msvc, VS_MSVC)

    os.rename(proj, old_proj)

    with open(proj, mode="w") as proj_io:
        proj_io.write(vcxproj)


def fix_vorbis_proj(proj: str, proj_line_nums: List[int]):
    path, name = os.path.split(proj)
    old_proj = path + "\\old_" + name

    if os.path.isfile(old_proj):
        return

    with open(proj, mode="r") as proj_io:
        vcxproj = proj_io.readlines()
        
    for line_num, line in enumerate(vcxproj):
        if "<WindowsTargetPlatformVersion>8.1</WindowsTargetPlatformVersion>" in line:
            vcxproj[line_num] = "    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>"

    # set build tools in vcxproj to the latest MSVC version
    for index, line_num in enumerate(proj_line_nums):
        vcxproj.insert(line_num + index, f"    <PlatformToolset>{VS_MSVC}</PlatformToolset>\n")

    os.rename(proj, old_proj)

    with open(proj, mode="w") as proj_io:
        proj_io.writelines(vcxproj)


def build_vorbis_project(proj, build_dir: str, out_dir: str):
    for cfg in {"Debug", "Release"}:
        # cmd = f"\"{VS_MSBUILD}\" \"{fix_proj}\" -property:Configuration={cfg} -property:Platform=x64"
        cmd = [VS_MSBUILD, proj, f"-property:Configuration={cfg}", "-property:Platform=x64"]
        subprocess.call(cmd)

        vorbis_copy_built_files(proj, cfg, build_dir, out_dir)

    print(f"Built {proj}\n")

    pass


def post_vorbis_extract(path: str, copy_path: str, msbuild_projects):
    if ARGS.no_build:
        return

    os.chdir(path)

    if SYS_OS == OS.Windows:
        if copy_path and not os.path.isdir(copy_path):
            os.makedirs(copy_path)

        print(f"Building {path}\n")

        for proj_list in msbuild_projects:
            proj, line_nums, build_dir = proj_list

            if type(line_nums) == list:
                fix_vorbis_proj(proj, line_nums)
            else:
                fix_ogg_proj(proj, line_nums)  # actually msvc version to change

            build_vorbis_project(proj, build_dir, copy_path)

            # move build if needed
            # if build_dir != copy_path:
            #    shutil.copytree(build_dir, copy_path, dirs_exist_ok=True)
            #    shutil.rmtree(build_dir)

    else:
        print("Not setup for unknown system\n")

    os.chdir("..")


def post_libogg_extract():
    post_vorbis_extract(
        "libogg",
        "",
        [["win32\\VS2015\\libogg.vcxproj", "v140", "win32\\VS2015\\x64"]])


def post_libvorbis_extract():
    # BLECH, WTF
    post_vorbis_extract(
        "libvorbis",
        "win32\\VS2010\\x64",
        [
            [
                "win32\\VS2010\\libvorbis\\libvorbis_static.vcxproj",
                [38, 43],
                "win32\\VS2010\\libvorbis\\x64",
            ],
            [
                "win32\\VS2010\\libvorbisfile\\libvorbisfile_static.vcxproj",
                [30, 34, 38, 42],
                "win32\\VS2010\\libvorbisfile\\x64",
            ],
        ]
    )


# =================================================================================================


def post_sdl_extract():
    shutil.copytree("SDL2/include", "../public/SDL2", dirs_exist_ok=True)
    

# =================================================================================================


FILE_LIST = {
    # NOTE: this kinda sucks because you can only put an item on one platform,
    # and would have to duplicate it for other platforms
    # also the ordering is iffy too

    # IDEA: somehow add something to install packages for linux? idk

    # All Platforms
    OS.Any: [
        [
            "https://github.com/ValveSoftware/steam-audio/releases/download/v4.0.3/steamaudio_4.0.3.zip",
            "zip",                  # file extension it's stored as
            "steamaudio",           # folder to check for if it exists already
            "",                     # folder it extracts as to rename to another folder (optional)
            ".",                    # extract into this folder (optional)
            None,                   # function to run post extraction (optional)
        ],
        [
            "https://codeload.github.com/KhronosGroup/KTX-Software/zip/refs/tags/v4.0.0",
            "zip",                  # file extension it's stored as
            "KTX-Software",         # folder to check for if it exists already
            "KTX-Software-4.0.0",   # folder it extracts as to rename to the folder above (optional)
            ".",                    # extract into this folder (optional)
            post_ktx_extract,       # function to run post extraction (optional)
        ],
        [
            "https://github.com/jrouwe/JoltPhysics/archive/refs/tags/v1.1.0.zip",
            "zip",                  # file extension it's stored as
            "JoltPhysics",          # folder to check for if it exists already
            "JoltPhysics-1.1.0",    # folder it extracts as to rename to the folder above (optional)
            ".",                    # extract into this folder (optional)
            post_jolt_extract,
        ],
        [
            # https://github.com/wolfpld/tracy/archive/refs/tags/v0.7.8.zip
            "https://github.com/wolfpld/tracy/archive/f1fea0331aa7222df5b0a8b0ffdf6610547fb336.zip",
            "zip",                   # file extension it's stored as
            "Tracy",              # folder to check for if it exists already
            "tracy-f1fea0331aa7222df5b0a8b0ffdf6610547fb336",         # folder it extracts as to rename to the folder above (optional)
            ".",                    # extract into this folder (optional)
            None,
        ],
        [
            "https://github.com/kcat/openal-soft/archive/6f9311b1ba26211895b507670b421356cc305bee.zip",
            "zip",                   # file extension it's stored as
            "openal-soft",              # folder to check for if it exists already
            "openal-soft-6f9311b1ba26211895b507670b421356cc305bee",         # folder it extracts as to rename to the folder above (optional)
            ".",                    # extract into this folder (optional)
            post_openal_extract,
        ],
    ],

    # Windows Only
    OS.Windows: [
        [
            # MUST BE FIRST FOR VSWHERE !!!!!
            "https://github.com/microsoft/vswhere/releases/download/2.8.4/vswhere.exe",
            "vswhere.exe",          # file extension it's stored as
            ".",                    # folder to check for if it exists already
            "",                     # folder it extracts as to rename to another folder (optional)
            ".",                    # extract into this folder (optional)
            setup_vs_env,           # function to run post extraction (optional)
        ],
        [
            "https://www.libsdl.org/release/SDL2-devel-2.0.16-VC.zip",
            "zip",                  # file extension it's stored as
            "SDL2",                 # folder to check for if it exists already
            "SDL2-2.0.16",          # folder it extracts as to rename to another folder (optional)
            ".",                    # extract into this folder (optional)
            post_sdl_extract,       # function to run post extraction (optional)
        ],
        [
            "https://downloads.xiph.org/releases/ogg/libogg-1.3.5.zip",
            "zip",
            "libogg",
            "libogg-1.3.5",
            ".",                    # extract into this folder (optional)
            post_libogg_extract,    # function to run post extraction (optional)
        ],
        [
            "https://github.com/xiph/vorbis/releases/download/v1.3.7/libvorbis-1.3.7.zip",
            "zip",
            "libvorbis",
            "libvorbis-1.3.7",
            ".",                    # extract into this folder (optional)
            post_libvorbis_extract,           # function to run post extraction (optional)
        ],
        [
            "https://github.com/g-truc/glm/releases/download/0.9.9.8/glm-0.9.9.8.7z",
            "7z",
            "glm",
        ],
    ],

    # Linux Only
    OS.Linux: [
    ],
}


# =================================================================================================


def download_file(url: str) -> bytes:
    req = Request(url, headers={'User-Agent': 'Mozilla/5.0'})
    try:
        # time_print("sending request")
        # time_print("Attempting Download: " + url)
        response = urlopen(req, timeout=4)
        file_data: bytes = response.read()
        # time_print("received request")
    except Exception as F:
        print("Error opening url: ", url, "\n", str(F), "\n")
        return b""

    return file_data


def extract_file(file_data: bytes, file_ext: str, folder: str) -> bool:
    # i hate this, why is there no library that can load it from bytes directly
    tmp_file = "tmp." + file_ext
    with open(tmp_file, mode="wb") as tmp_file_io:
        tmp_file_io.write(file_data)

    if not os.path.isdir(folder):
        os.makedirs(folder)

    if file_ext == "7z":
        py7zr.unpack_7zarchive(tmp_file, folder)
    elif file_ext == "zip":
        zf = ZipFile(tmp_file)
        zf.extractall(folder)
        zf.close()
    elif file_ext == "exe":
        pass
    else:
        print(f"No support for file extension - extract it manually: {file_ext}")
        return False

    os.remove(tmp_file)

    return True


def handle_item(url: str, file_ext: str, folder: str, extract_folder: str = "", working_dir: str = ".", func=None):
    is_zip = file_ext == "zip" or file_ext == "7z"
    print_name = folder if is_zip else file_ext

    if (folder != "." and not os.path.isdir(folder)) or ARGS.force or not is_zip:

        print(f"Downloading \"{print_name}\"")

        file_data: bytes = download_file(url)
        if file_data == b"":
            return

        # is this just a full filename
        if not is_zip:
            with open(file_ext, mode="wb") as file_io:
                file_io.write(file_data)
        else:
            if not extract_file(file_data, file_ext, working_dir):
                return

        # rename it
        if extract_folder and extract_folder != folder:
            os.rename(extract_folder, folder)
    else:
        print(f"Already Downloaded: {print_name}")

    if func:
        print(f"Running Post Build Func: \"{print_name}\"")
        func()


def main():
    # Do your platform first
    for item in FILE_LIST[SYS_OS]:
        handle_item(*item)

    # Do all platforms next
    for item in FILE_LIST[OS.Any]:
        handle_item(*item)

    print("Finished")


if __name__ == "__main__":
    ARGS = parse_args()
    main()

