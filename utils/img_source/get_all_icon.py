import os
import argparse
import cv2

def classify_images(folder, out_dir):
    folder = os.path.abspath(folder)
    out_dir = os.path.abspath(out_dir)

    if not os.path.isdir(folder):
        raise NotADirectoryError(f"Folder does not exist: {folder}")

    os.makedirs(out_dir, exist_ok=True)

    image_ext = {".png", ".jpg", ".jpeg", ".bmp", ".gif", ".webp"}
    image_files = []

    for root, _, files in os.walk(folder):
        for name in files:
            if os.path.splitext(name.lower())[1] in image_ext:
                image_files.append(os.path.abspath(os.path.join(root, name)))

    print(f"Found {len(image_files)} images.")
    print("Input a number to classify, or q to quit.")
    print("Each category will be written to class_<number>.txt in output folder.")
    print()

    for img_path in image_files:
        img = cv2.imread(img_path)

        if img is None:
            print(f"Cannot open image: {img_path}")
            continue

        # 显示图片（自动覆盖上一张）
        cv2.imshow("Image", img)
        cv2.waitKey(1)  # 立即显示，不阻塞

        print(f"Classify image: {img_path}")
        label = input("Enter class number (or q to quit): ").strip()

        if label.lower() == "q":
            print("Exit.")
            break

        if not label.isdigit():
            print("Invalid input, skipped.")
            continue

        out_txt = os.path.join(out_dir, f"class_{label}.txt")
        with open(out_txt, "a", encoding="utf-8") as f:
            f.write(img_path + "\n")

        print(f" -> Saved to {out_txt}")
        print("-" * 40)

    # 关闭窗口
    cv2.destroyAllWindows()


def main():
    parser = argparse.ArgumentParser(description="Interactive image classifier (OpenCV)")
    parser.add_argument("--folder", required=True, help="Image folder to classify")
    parser.add_argument("--out", required=True, help="Folder to store classification TXT files")

    args = parser.parse_args()
    classify_images(args.folder, args.out)


if __name__ == "__main__":
    main()
