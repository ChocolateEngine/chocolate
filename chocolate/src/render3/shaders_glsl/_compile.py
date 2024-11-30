import os
import argparse


SHADER_LIST = [
    "guide_compute",
]


SHADER_FAIL_LIST = []


def parse_args() -> argparse.Namespace:
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument("-a", "--args", help="shader compiling arguments", default="")
    arg_parser.add_argument("-o", "--output", help="shader output directory", default="../../../../../chocolate_output/core")
    return arg_parser.parse_args()


def compile_shader(shader: str):
    args = f"glslc \"{shader}\" {ARGS.args} -o \"{SHADER_OUTPUT}/{shader}.spv\""
    print(args)
    ret = os.system(args)
    if ret != 0:
        SHADER_FAIL_LIST.append(shader)


def main():
    for shader in SHADER_LIST:
        found_shader = False

        # First, check if this a file on disk
        if os.path.isfile(shader):
            compile_shader(shader)
            found_shader = True

        # Otherwise, check for a vertex, fragment, or compute shader
        else:
            if os.path.isfile(shader + ".vert"):
                compile_shader(shader + ".vert")
                found_shader = True

            if os.path.isfile(shader + ".frag"):
                compile_shader(shader + ".frag")
                found_shader = True

            if os.path.isfile(shader + ".comp"):
                compile_shader(shader + ".comp")
                found_shader = True

        if not found_shader:
            print(f"Shader Not Found: \"{shader}\"")

    if len(SHADER_FAIL_LIST) > 0:
        print(f"\nFailed to compile {len(SHADER_FAIL_LIST)} shaders:")
        for shader in SHADER_FAIL_LIST:
            print(f"\t{shader}")
    else:
        print("\nAll shaders compiled successfully")


if __name__ == "__main__":
    ARGS = parse_args()
    GAME_PATH = os.path.abspath(ARGS.output)
    SHADER_OUTPUT = os.path.join(GAME_PATH, "shaders/render3")
    print(f"Game Directory: \"{GAME_PATH}\"")
    print(f"Shader Output Directory: \"{SHADER_OUTPUT}\"\n")

    if not os.path.isdir(GAME_PATH):
        print("ERROR: Game Directory does not exist!")
        quit()

    if not os.path.isdir(SHADER_OUTPUT):
        os.makedirs(SHADER_OUTPUT)

    main()

