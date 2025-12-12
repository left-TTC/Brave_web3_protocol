import os
import argparse
import shutil


def find_common_src(txt):
    """扫描 txt，找到包含 '/src/' 或 '\\src\\' 的源码根路径"""
    with open(txt, "r", encoding="utf-8") as f:
        for line in f:
            if " copied to " not in line:
                continue

            src_path = line.split(" copied to ")[0].strip()
            src_path = os.path.abspath(src_path)

            # 支持 Linux 和 Windows
            markers = ["/src/", "\\src\\"]
            for mk in markers:
                idx = src_path.lower().find(mk)
                if idx != -1:
                    return src_path[:idx + len(mk)]

    raise RuntimeError("ERROR: Cannot detect '/src/' base directory from txt")


def get_relative_path(full_path, common_src):
    """返回 full_path 相对 common_src 的路径，永不以 / 开头"""
    full_path = os.path.abspath(full_path)
    common_src = os.path.abspath(common_src)

    if not full_path.startswith(common_src):
        raise ValueError(f"{full_path} is not under common src {common_src}")

    rel = full_path[len(common_src):]

    # 关键修复：避免 rel 以 '/' 开头
    rel = rel.lstrip("/\\")
    return rel


def copy_one(src_file, common_src, out_root):
    """复制文件，保持整个相对目录结构"""
    rel_path = get_relative_path(src_file, common_src)
    new_file = os.path.join(out_root, rel_path)

    os.makedirs(os.path.dirname(new_file), exist_ok=True)
    shutil.copy2(src_file, new_file)

    print(f"Copied:\n  {src_file}\n    --> {new_file}")


def main():
    parser = argparse.ArgumentParser(description="Copy images/files according to 'copied to' log")
    parser.add_argument("--txt", required=True, help="Path to log txt")
    parser.add_argument("--out", required=True, help="Output src root directory")
    args = parser.parse_args()

    txt_path = os.path.abspath(args.txt)
    out_root = os.path.abspath(args.out)

    if not os.path.isfile(txt_path):
        raise FileNotFoundError(f"TXT file not found: {txt_path}")

    # 自动创建输出目录，而不是报错
    if not os.path.isdir(out_root):
        print(f"Output folder does not exist, creating: {out_root}")
        os.makedirs(out_root, exist_ok=True)

    common_src = find_common_src(txt_path)
    print(f"Detected common src: {common_src}")

    # 主循环
    with open(txt_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if " copied to " not in line:
                continue

            src_file = line.split(" copied to ")[0].strip()
            src_file = os.path.abspath(src_file)

            if not os.path.isfile(src_file):
                print(f"Warning: missing file, skipped: {src_file}")
                continue

            copy_one(src_file, common_src, out_root)


if __name__ == "__main__":
    main()
