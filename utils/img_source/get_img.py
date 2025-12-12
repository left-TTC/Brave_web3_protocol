import os
import argparse
import shutil


def find_common_src(txt):
    """根据 '/src/' 自动找到源码根路径，例如 /home/f/myproject/Brave/src/"""

    with open(txt, "r", encoding="utf-8") as f:
        for line in f:
            if " copied to " not in line:
                continue
            src_path = line.split(" copied to ")[0].strip()
            src_path = os.path.abspath(src_path)

            idx = src_path.lower().find("/src/")
            if idx != -1:
                return src_path[:idx + len("/src/")]

    raise RuntimeError("Could not detect '/src/' base directory from txt")


def get_relative_path(full_path, common_src):
    full_path = os.path.abspath(full_path)
    common_src = os.path.abspath(common_src)

    if not full_path.startswith(common_src):
        raise ValueError(f"{full_path} is not under common src {common_src}")

    return full_path[len(common_src):]


def copy_one(src_file, common_src, out_root):
    """复制文件，不改变目录结构"""

    rel_path = get_relative_path(src_file, common_src)
    new_file = os.path.join(out_root, rel_path)

    os.makedirs(os.path.dirname(new_file), exist_ok=True)

    shutil.copy2(src_file, new_file)

    print(f"Copied: {src_file}")
    print(f"   --> {new_file}")


def main():
    parser = argparse.ArgumentParser(description="Copy images/files according to 'copied to' log")
    parser.add_argument("--txt", required=True, help="Path to log txt")
    parser.add_argument("--out", required=True, help="Output src root directory")

    args = parser.parse_args()

    txt_path = os.path.abspath(args.txt)
    out_root = os.path.abspath(args.out)

    if not os.path.isfile(txt_path):
        raise FileNotFoundError(f"TXT file not found: {txt_path}")

    if not os.path.isdir(out_root):
        raise NotADirectoryError(f"Output folder does not exist: {out_root}")

    common_src = find_common_src(txt_path)
    print(f"Detected common src: {common_src}")

    with open(txt_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if " copied to " not in line:
                continue

            src_file = line.split(" copied to ")[0].strip()

            if not os.path.isfile(src_file):
                print(f"Warning: File not found, skipped: {src_file}")
                continue

            copy_one(src_file, common_src, out_root)


if __name__ == "__main__":
    main()
