import os
import sys

def compress_files(path):
    if not os.path.exists(path):
        print(f"Error: Path '{path}' does not exist.")
        return

    lst = os.listdir(path)
    for i in lst:
        full_path = os.path.join(path, i)
        if os.path.isfile(full_path) and i.endswith(".mxmod"):
            output_file = full_path + ".z"
            print(f"Compressing {i}...")
            cmd = f"mxmod_compress -c \"{full_path}\" -o \"{output_file}\""
            result = os.system(cmd)
            
            if result != 0:
                print(f"Failed to compress {i}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        compress_files(sys.argv[1])
        