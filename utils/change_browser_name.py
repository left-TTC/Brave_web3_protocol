
import os
import argparse

## find the src's absolute path
def find_common_src(txt):
    
    if not os.path.isfile(txt):
        raise FileNotFoundError(f"TXT file not found: {txt}")

    all_src_paths = []

    with open(txt, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if " copied to " not in line:
                continue

            src_path = line.split(" copied to ")[0].strip()

            if os.path.isabs(src_path):
                all_src_paths.append(os.path.abspath(src_path))

    if not all_src_paths:
        raise RuntimeError("No valid source paths found in txt file")

    sample = all_src_paths[0]
    idx = sample.lower().find("/src/")
    if idx == -1:
        raise RuntimeError("No '/src/' folder detected in paths")

    common = sample[:idx + len("/src/")]

    return common



def get_relative_path(full_path, common_src):
    full_path = os.path.abspath(full_path)
    common_src = os.path.abspath(common_src)

    if not common_src.endswith(os.sep):
        common_src += os.sep

    if not full_path.startswith(common_src):
        raise ValueError(f"{full_path} is not under src root {common_src}")

    return full_path[len(common_src):]



def process_one_file(src_file, common_src, out_root, find_text, replace_text):
    """读取文件 → 替换字符串 → 写到新src目录"""

    rel_path = get_relative_path(src_file, common_src)

    new_file_path = os.path.join(out_root, rel_path)

    os.makedirs(os.path.dirname(new_file_path), exist_ok=True)

    with open(src_file, "r", encoding="utf-8") as f:
        content = f.read()

    new_content = content.replace(find_text, replace_text)

    with open(new_file_path, "w", encoding="utf-8") as f:
        f.write(new_content)

    print(f"Processed: {src_file}")
    print(f"   --> {new_file_path}")



def main():
    parser = argparse.ArgumentParser(description="Replace text in copied source files")
    parser.add_argument("--txt", required=True, help="Path to copy log txt")
    parser.add_argument("--find", required=True, help="Text to find (case-sensitive)")
    parser.add_argument("--replace", required=True, help="Replacement text")
    parser.add_argument("--out", required=True, help="Output src directory")

    ### get the record txt
    args = parser.parse_args()

    ### the path of txt that record the brave changed string
    txt_path = os.path.abspath(args.txt)
    ### the nouns that will be replaced
    find_text = args.find
    ### the nouns that will be replaced to
    replace_text = args.replace
    ### output src must name src
    out_root = os.path.abspath(args.out)

    common_src = find_common_src(txt_path)
    print("Find common src path:", common_src)

    if not os.path.isdir(out_root):
        print("no output src floder")
        exit(1)
    
    with open(txt_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if " copied to " not in line:
                continue

            src_file = line.split(" copied to ")[0].strip()

            if not src_file.startswith(common_src):
                print(f"Skipped (not under src): {src_file}")
                continue

            process_one_file(
                src_file=src_file,
                common_src=common_src,
                out_root=out_root,
                find_text=find_text,
                replace_text=replace_text,
            )

    print("Find common src path:", common_src)


if __name__ == "__main__":
    main()
