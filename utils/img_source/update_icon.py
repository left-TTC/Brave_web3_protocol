import sys
import os
import argparse
from PIL import Image


if sys.platform.startswith("win"):
    print("Windows detected")
    common_path = ".\\img_path\\common.txt"
    dev_path = ".\\img_path\\dev.txt"
    out_path = ".\\changed\\src"
    common_from = ".\\icon_source.png"
    dev_from = ".\\icon_dev_source.png"
    out_path_src = ".\\..\\..\\patch\\1.87.3\\src"
else:
    print("Linux or mac detected")
    common_path = "./img_path/common.txt"
    dev_path = "./img_path/dev.txt"
    out_path = "./changed/src"
    common_from = "./icon_source.png"
    dev_from = "./icon_dev_source.png"
    out_path_src = "./../../patch/1.87.3/src"


def detect_src_root(path):
    """
    自动检测 Brave 项目的 src 根路径：
    例如：
    D:/Project/Brave/src/brave/app/... → D:/Project/Brave/src/
    """
    p = path.replace("\\", "/").lower()
    idx = p.find("/src/")
    if idx == -1:
        raise RuntimeError(f"Cannot detect 'src' root for: {path}")
    
    return os.path.abspath(path[:idx + len("/src/")])


def get_rel_from_src(full_path):
    """
    获取文件相对于 Brave/src 的路径
    """
    src_root = detect_src_root(full_path)
    rel = os.path.abspath(full_path)[len(src_root):]
    return rel.lstrip("/\\"), src_root


def get_image_size(path):
    with Image.open(path) as img:
        return img.width, img.height


def scale_image(src_img, out_img, size_w, size_h):
    with Image.open(src_img) as img:
        if img.mode not in ("RGB", "RGBA"):
            img = img.convert("RGBA")

        img = img.resize((size_w, size_h), Image.LANCZOS)

        os.makedirs(os.path.dirname(out_img), exist_ok=True)
        img.save(out_img, quality=95)

        print(f"Saved scaled image: {out_img}")


def process_list(list_file, base_icon, out_root):
    """
    使用 Brave/src 相对路径保存输出文件
    """
    list_file = os.path.abspath(list_file)
    base_icon = os.path.abspath(base_icon)
    out_root = os.path.abspath(out_root)

    if not os.path.isfile(list_file):
        print(f"[ERROR] List file not found: {list_file}")
        return

    if not os.path.isfile(base_icon):
        print(f"[ERROR] Base icon not found: {base_icon}")
        return

    with open(list_file, "r", encoding="utf-8") as f:
        for line in f:
            img_path = line.strip()
            if not img_path:
                continue

            img_path = os.path.abspath(img_path)

            if not os.path.isfile(img_path):
                print(f"[WARNING] Not a file, skipped: {img_path}")
                continue

            # 读取原图尺寸
            w, h = get_image_size(img_path)

            # 精确获取 src-relative 路径
            rel_path, src_root = get_rel_from_src(img_path)

            # 输出到 patch/src 下对应路径
            out_file = os.path.abspath(os.path.join(out_root, rel_path))

            scale_image(base_icon, out_file, w, h)


def main():
    parser = argparse.ArgumentParser(description="Scale icons to match target image sizes.")
    parser.add_argument("--src", action="store_true",
                        help="Use out_path_src instead of default out_path")
    args = parser.parse_args()

    final_out = out_path_src if args.src else out_path
    print(f"Using output directory: {final_out}")

    print("Processing common icons...")
    process_list(common_path, common_from, final_out)

    print("Processing dev icons...")
    process_list(dev_path, dev_from, final_out)

    print("Done.")


if __name__ == "__main__":
    main()
