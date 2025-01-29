import os
import platform
import sys
import argparse
import shutil
import subprocess
from enum import Enum, auto
from urllib.request import Request, urlopen
from zipfile import ZipFile
from typing import List, Dict, BinaryIO

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


# Win32 Console Color Printing

_win32_legacy_con = False
_win32_handle = None

if os.name == "nt":
    if platform.release().startswith("10"):
        # hack to enter virtual terminal mode,
        # could do it properly, but that's a lot of lines and this works just fine
        import subprocess

        subprocess.call('', shell=True)
    else:
        import ctypes

        _win32_handle = ctypes.windll.kernel32.GetStdHandle(-11)
        _win32_legacy_con = True


class Color(Enum):
    if _win32_legacy_con:
        RED = "4"
        DGREEN = "2"
        GREEN = "10"
        YELLOW = "6"
        BLUE = "1"
        MAGENTA = "13"
        CYAN = "3"  # or 9

        DEFAULT = "7"
    else:  # ansi escape chars
        RED = "\033[0;31m"
        DGREEN = "\033[0;32m"
        GREEN = "\033[1;32m"
        YELLOW = "\033[0;33m"
        BLUE = "\033[0;34m"
        MAGENTA = "\033[1;35m"
        CYAN = "\033[0;36m"

        DEFAULT = "\033[0m"


class Severity(Enum):
    WARNING = Color.YELLOW
    ERROR = Color.RED


WARNING_COUNT = 0


def warning(*text):
    _print_severity(Severity.WARNING, "\n          ", *text)
    global WARNING_COUNT
    WARNING_COUNT += 1


def error(*text):
    _print_severity(Severity.ERROR, "\n        ", *text, "\n")
    quit(1)


def verbose(*text):
    print("".join(text))


def verbose_color(color: Color, *text):
    print_color(color, "".join(text))


def _print_severity(level: Severity, spacing: str, *text):
    print_color(level.value, f"[{level.name}] {spacing.join(text)}")


def win32_set_fore_color(color: int):
    if not ctypes.windll.kernel32.SetConsoleTextAttribute(_win32_handle, color):
        print(f"[ERROR] WIN32 Changing Colors Failed, Error Code: {str(ctypes.GetLastError())},"
              f" color: {color}, handle: {str(_win32_handle)}")


def stdout_color(color: Color, *text):
    if _win32_legacy_con:
        win32_set_fore_color(int(color.value))
        sys.stdout.write("".join(text))
        win32_set_fore_color(int(Color.DEFAULT.value))
    else:
        sys.stdout.write(color.value + "".join(text) + Color.DEFAULT.value)


def print_color(color: Color, *text):
    stdout_color(color, *text, "\n")


def set_con_color(color: Color):
    if _win32_legacy_con:
        win32_set_fore_color(int(color.value))
    else:
        sys.stdout.write(color.value)


# TODO: make enum flags or something later on if you add another platform here
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


ERROR_LIST = []
CUR_PROJECT = "PROJECT"
ROOT_DIR = os.path.dirname(os.path.realpath(__file__))
DIR_STACK = []


def set_project(name: str):
    global CUR_PROJECT
    CUR_PROJECT = name
    print(f"\n"
          f"---------------------------------------------------------\n"
          f" Current Project: {name}\n"
          f"---------------------------------------------------------\n")


def push_dir(path: str):
    abs_dir = os.path.abspath(path)
    DIR_STACK.append(abs_dir)
    os.chdir(abs_dir)


def pop_dir():
    if len(DIR_STACK) == 0:
        print("ERROR: DIRECTORY STACK IS EMPTY!")
        return

    abs_dir = DIR_STACK.pop()
    os.chdir(abs_dir)


def reset_dir():
    os.chdir(ROOT_DIR)


# System Command Failed!
def syscmd_err(ret: int, string: str):
    global CUR_PROJECT
    error = f"{CUR_PROJECT}: {string}: return code {ret}\n"
    ERROR_LIST.append(error)
    sys.stderr.write(error)


def syscmd(cmd: str, string: str) -> bool:
    ret = os.system(cmd)
    if ret == 0:
        return True

    # System Command Failed!
    syscmd_err(ret, string)
    return False


def syscall(cmd: list, string: str) -> bool:
    ret = subprocess.call(cmd)
    if ret == 0:
        return True

    # System Command Failed!
    syscmd_err(ret, string)
    return False


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


def parse_args() -> argparse.Namespace:
    args = argparse.ArgumentParser()
    args.add_argument("-nb", "--no-build", action="store_true", help="Don't build the libraries")
    # args.add_argument("-d", "--no-download", action="store_true", help="Don't download the libraries")
    args.add_argument("-t", "--target", nargs="+", help="Only download and build specific libraries")
    args.add_argument("-f", "--force", help="Force Run Everything")
    args.add_argument("-c", "--clean", help="Clean Everything in the thirdparty folder")
    return args.parse_args()


# =================================================================================================


def build_ktx():
    if ARGS.no_build:
        return

    set_project("KTX")

    os.chdir("KTX-Software")

    # disable AVX2 for the astc encoder
    # we have a friend who has a pc without AVX2 lol
    build_options = "-DISA_SSE2=ON"

    # TODO: this --config thing doesn't actually work on linux,
    # use build/Release and build/Debug folders instead?
    if SYS_OS == OS.Windows:
        if not syscmd(f"cmake -S . -B build {build_options}", "Failed to run cmake"):
            return

        print("Building KTX - Release\n")
        if not syscmd("cmake --build build --config Release", "Failed to build in Release"):
            return

        print("Building KTX - Debug\n")
        if not syscmd("cmake --build build --config Debug", "Failed to build in Debug"):
            return
        
    else:
        if not syscmd(f"cmake -S . -B build {build_options} -DCMAKE_BUILD_TYPE=Release", "Failed to run cmake"):
            return

        print("Building KTX - Release\n")
        if not syscmd("cmake --build build --config Release", "Failed to build in Release"):
            return

        # print("Building KTX - Debug\n")
        # if not syscmd("cmake --build build --config Debug", "Failed to build in Debug"):
        #     return


# =================================================================================================


def build_jolt(build_dir, build_options):
    if SYS_OS == OS.Windows:
        if not syscmd(f"cmake -S . -B {build_dir} -A x64 {build_options}", "Failed to run cmake"):
            return

        print(f"Building JoltPhysics - Release - {build_dir}\n")
        if not syscmd(f"cmake --build {build_dir} --config Release", "Failed to build in Release"):
            return

        print(f"Building JoltPhysics - Debug - {build_dir}\n")
        if not syscmd(f"cmake --build {build_dir} --config Debug", "Failed to build in Debug"):
            return

    else:
        print(f"Building JoltPhysics - Release - {build_dir}\n")
        if not syscmd(f"cmake -S . -B {build_dir}/Release -DCMAKE_CXX_COMPILER=g++ -DCMAKE_POSITION_INDEPENDENT_CODE=ON {build_options}",
               "Failed to run cmake for Release build"):
            return

        if not syscmd(f"cmake --build {build_dir}/Release --config Release", "Failed to build in Release"):
            return

        print(f"Building JoltPhysics - Debug - {build_dir}\n")
        if not syscmd(f"cmake -S . -B {build_dir}/Debug -DCMAKE_CXX_COMPILER=g++ -DCMAKE_POSITION_INDEPENDENT_CODE=ON {build_options}",
               "Failed to run cmake for Debug build"):
            return

        if not syscmd(f"cmake --build {build_dir}/Debug --config Debug", "Failed to build in Debug"):
            return


def post_jolt_extract():
    if ARGS.no_build:
        return

    set_project("JoltPhysics")

    os.chdir("JoltPhysics/Build")
    shared_build_options = "-DTARGET_UNIT_TESTS=OFF -DTARGET_HELLO_WORLD=OFF -DTARGET_PERFORMANCE_TEST=OFF -DTARGET_SAMPLES=OFF -DTARGET_VIEWER=OFF"

    # Build Jolt without any modern CPU features first
    build_options = "-DUSE_AVX2=OFF -DUSE_F16C=OFF -DUSE_FMADD=OFF " + shared_build_options
    build_jolt("min", build_options)

    # Build Jolt with all modern CPU features (maybe make more inbetween options?)
    build_options = "-DUSE_AVX2=ON -DUSE_F16C=ON -DUSE_FMADD=ON " + shared_build_options
    build_jolt("avx2", build_options)


# =================================================================================================


def build_mimalloc():
    if ARGS.no_build:
        return

    set_project("mimalloc")
    os.chdir("mimalloc")

    if SYS_OS == OS.Windows:
        os.chdir("ide/vs2022")

        for proj in {"mimalloc.vcxproj", "mimalloc-override.vcxproj"}:
            for cfg in {"Debug", "Release"}:
                # cmd = f"\"{VS_MSBUILD}\" \"{proj}\" -property:Configuration={cfg} -property:Platform=x64"
                cmd = [VS_MSBUILD, proj, f"-property:Configuration={cfg}", "-property:Platform=x64"]
                if not syscall(cmd, f"Failed to compile {proj} in {cfg}"):
                    return

        pass

    else:
        if not syscmd(f"cmake -S . -B build", "Failed to run cmake"):
            return

        print("Building mimalloc - Release\n")
        if not syscmd(f"cmake --build build --config Release", "Failed to build in Release"):
            return

        print("Building mimalloc - Debug\n")
        if not syscmd(f"cmake --build build --config Debug", "Failed to build in Debug"):
            return


# =================================================================================================


def post_openal_extract():
    if ARGS.no_build:
        return

    set_project("OpenAL")

    os.chdir("openal-soft/build")

    if not syscmd("cmake ..", "Failed to run cmake"):
        return

    print("Building OpenAL-Soft - Release\n")
    if not syscmd("cmake --build . --config Release", "Failed to build in Release"):
        return

    print("Building OpenAL-Soft - Debug\n")
    if not syscmd("cmake --build . --config Debug", "Failed to build in Debug"):
        return


# =================================================================================================


def post_flatbuffers_extract():
    if ARGS.no_build:
        return
        
    set_project("FlatBuffers")

    os.chdir("flatbuffers")

    build_options = "-DFLATBUFFERS_BUILD_TESTS=OFF"

    # sets -fPIC
    # if SYS_OS == OS.Linux:
    #     build_options += " -DCMAKE_POSITION_INDEPENDENT_CODE=ON"

    if not syscmd(f"cmake -B build {build_options} .", "Failed to run cmake"):
        return

    print("Building FlatBuffers - Release\n")
    if not syscmd(f"cmake --build ./build --config Release", "Failed to build in Release"):
        return

    print("Building FlatBuffers - Debug\n")
    if not syscmd(f"cmake --build ./build --config Debug", "Failed to build in Debug"):
        return


# =================================================================================================
# Importer Libraries


def post_mozjpeg_extract():
    if ARGS.no_build:
        return
        
    set_project("MozJPEG")

    os.chdir("mozjpeg")

    build_options = "-DPNG_SUPPORTED=0"

    if not syscmd(f"cmake -B build {build_options} .", "Failed to run cmake"):
        return

    print("Building MozJPEG - Release\n")
    if not syscmd(f"cmake --build ./build --config Release", "Failed to build in Release"):
        return

    print("Building MozJPEG - Debug\n")
    if not syscmd(f"cmake --build ./build --config Debug", "Failed to build in Debug"):
        return


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
    set_project("libogg")

    post_vorbis_extract(
        "libogg",
        "",
        [["win32\\VS2015\\libogg.vcxproj", "v140", "win32\\VS2015\\x64"]])


def post_libvorbis_extract():
    set_project("libvorbis")

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


def post_vma_extract():
    pass
    

# =================================================================================================


'''
Basic Structure for a file here:
{
    # URL for downloading the file
    "url":  "ENTRY",
    
    # Filename that gets downloaded, most stuff from github is different from the url filename
    "file": "FILENAME.zip/7z",
    
    # Doubles as the folder name and the "package" name in this script, so you can skip it or only use it, etc.
    "name": "ENTRY",
    
    # OPTIONAL: A compile function to run after extracting it
    "func": function_pointer,
    
    # OPTIONAL: Name of the folder the file is extracted to, and will be renamed to what "name" is, usually not needed
    "extracted_folder": "",
},
'''


FILE_LIST = {
    # All Platforms
    OS.Any: [
        {
            "url":  "https://github.com/ozxybox/SpeedyKeyV/archive/refs/heads/master.zip",
            "file": "SpeedyKeyV-master.zip",
            "name": "SpeedyKeyV",
        },
        {
            "url":  "https://github.com/jkuhlmann/cgltf/archive/refs/tags/v1.14.zip",
            "file": "cgltf-1.14.zip",
            "name": "cgltf",
        },
        {
            "url":  "https://github.com/thisistherk/fast_obj/archive/refs/tags/v1.3.zip",
            "file": "fast_obj-1.3.zip",
            "name": "fast_obj",
        },
        {
            "url":  "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/archive/refs/tags/v3.1.0.zip",
            "file": "VulkanMemoryAllocator-3.1.0.zip",
            "name": "VulkanMemoryAllocator",
            "func": post_vma_extract,
        },
        {
            "url":  "https://github.com/mozilla/mozjpeg/archive/refs/tags/v4.1.1.zip",
            "file": "mozjpeg-4.1.1.zip",
            "name": "mozjpeg",
            "func": post_mozjpeg_extract,
        },
        {
            "url":  "https://github.com/google/flatbuffers/archive/refs/tags/v23.5.26.zip",
            "file": "flatbuffers-23.5.26.zip",
            "name": "flatbuffers",
            "func": post_flatbuffers_extract,
        },
        {
            "url":  "https://github.com/jrouwe/JoltPhysics/archive/refs/tags/v5.2.0.zip",
            "file": "JoltPhysics-5.2.0.zip",
            "name": "JoltPhysics",
            "func": post_jolt_extract,
        },
        {
            "url":  "https://github.com/wolfpld/tracy/archive/f1fea0331aa7222df5b0a8b0ffdf6610547fb336.zip",
            "file": "tracy-f1fea0331aa7222df5b0a8b0ffdf6610547fb336.zip",
            "name": "Tracy",
        },

        # NOTE: KTX has plenty of downloads for pre-built binaries now, maybe use one of those instead depending on the platform?
        {
            "url":  "https://github.com/KhronosGroup/KTX-Software/archive/refs/tags/v4.3.2.zip",
            "name": "KTX-Software",
            "file": "KTX-Software-4.3.2.zip",
            "func": build_ktx,
        },
        # TODO: Update this to the open source one and compile it yourself for debug symbols
        {
            "url":  "https://github.com/ValveSoftware/steam-audio/releases/download/v4.0.3/steamaudio_4.0.3.zip",
            "name": "steamaudio",
            "file": "steamaudio_4.0.3.zip",
            "extracted_folder": "steamaudio",
        },
    ],

    # Windows Only
    OS.Windows: [

        # MUST BE FIRST FOR VSWHERE !!!!!
        {
            "url":  "https://github.com/microsoft/vswhere/releases/download/2.8.4/vswhere.exe",
            "name": "vswhere",
            "file": "vswhere.exe",
            "func": setup_vs_env,
        },
        {
            "url":  "https://github.com/libsdl-org/SDL/releases/download/release-2.26.5/SDL2-devel-2.26.5-VC.zip",
            "name": "SDL2",
            "file": "SDL2-2.26.5.zip",
            "func": post_sdl_extract,
        },
        {
            "url":  "https://downloads.xiph.org/releases/ogg/libogg-1.3.5.zip",
            "name": "libogg",
            "file": "libogg-1.3.5.zip",
            "func": post_libogg_extract,
        },
        {
            "url":  "https://github.com/xiph/vorbis/releases/download/v1.3.7/libvorbis-1.3.7.zip",
            "name": "libvorbis",
            "file": "libvorbis-1.3.7.zip",
            "func": post_libvorbis_extract,
        },
        {
            "url":  "https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip",
            "name": "glm",
            "file": "glm-1.0.1.zip",
        },
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
        response = urlopen(req, timeout=120)
        file_data: bytes = response.read()
        # time_print("received request")
    except Exception as F:
        error("Error opening url: ", url, "\n", str(F), "\n")
        return b""

    return file_data


def extract_file(file_data: bytes, file: str, file_ext: str, folder: str) -> bool:
    # i hate this, why is there no library that can load it from bytes directly
    tmp_file = "tmp." + file
    tmp_file_io: BinaryIO
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
        error(f"No support for file extension - extract it manually: {file_ext}")
        return False

    os.remove(tmp_file)

    return True


def handle_item(item: dict):
    url = item["url"] if "url" in item else ""
    file = item["file"] if "file" in item else ""
    name = item["name"] if "name" in item else ""
    func = item["func"] if "func" in item else None
    extracted_folder = item["extracted_folder"] if "extracted_folder" in item else None

    # Check if we want this item
    if ARGS.target:
        if name not in ARGS.target:
            return

    # folder, file_ext = os.path.splitext(file)
    folder, file_ext = file.rsplit(".", 1)
    is_zip = file_ext in ("zip", "7z")

    if extracted_folder:
        folder = extracted_folder

    if not os.path.isdir(name) or ARGS.force or not is_zip:

        print_color(Color.CYAN, f"Downloading \"{name}\"")

        file_data: bytes = download_file(url)
        if file_data == b"":
            return

        # is this just a full filename
        if not is_zip:
            with open(file, mode="wb") as file_io:
                file_io.write(file_data)
        else:
            if not extract_file(file_data, file, file_ext, "."):
                return
                
            # rename it
            if folder != name:
                os.rename(folder, name)

    else:
        print_color(Color.CYAN, f"Already Downloaded: {name}")

    if func:
        print_color(Color.CYAN, f"Running Post Build Func: \"{name}\"")
        func()

    reset_dir()


def main():
    if ARGS.clean:
        print("NOT IMPLEMENTED YET!!!")
        return

    # Do your platform first (need to be first on windows for vswhere.exe)
    for item in FILE_LIST[SYS_OS]:
        handle_item(item)

    print("\n---------------------------------------------------------\n")

    # Do all platforms last
    for item in FILE_LIST[OS.Any]:
        handle_item(item)

    # check for any errors and print them
    errors_str = "Error" if len(ERROR_LIST) == 1 else "Errors"
    if len(ERROR_LIST) > 0:
        print("\n---------------------------------------------------------")
        sys.stderr.write(f"\n{len(ERROR_LIST)} {errors_str}:\n")
        for error in ERROR_LIST:
            sys.stderr.write(error)

    print(f"\n"
          f"---------------------------------------------------------\n"
          f" Finished - {len(ERROR_LIST)} {errors_str}\n"
          f"---------------------------------------------------------\n")

    if len(ERROR_LIST) > 1:
        exit(-1)


if __name__ == "__main__":
    ARGS = parse_args()
    main()

