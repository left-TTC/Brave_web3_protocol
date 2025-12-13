import os
import argparse
from PIL import Image

def png_to_ico(src_png, out_ico, sizes):
    """
    Convert PNG to ICO with specific sizes.
    sizes: list of int, e.g., [256, 128, 64, 48, 32, 24, 16]
    """
    if not os.path.isfile(src_png):
        raise FileNotFoundError(f"PNG file not found: {src_png}")

    # Load PNG
    img = Image.open(src_png)

    # Ensure output directory (only when it has a directory part)
    out_dir = os.path.dirname(out_ico)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    # Save ICO with specified sizes
    img.save(out_ico, format="ICO", sizes=[(s, s) for s in sizes])
    print(f"ICO saved: {out_ico}")


def main():
    parser = argparse.ArgumentParser(description="Convert PNG to ICO with custom sizes.")
    parser.add_argument("--png", required=True, help="Source PNG file")
    parser.add_argument("--out", required=True, help="Output ICO file")
    parser.add_argument(
        "--sizes",
        help="Comma-separated sizes, e.g. 256,128,64",
        default="256,128,64,48,32,24,16",
    )

    args = parser.parse_args()

    sizes = []
    for s in args.sizes.split(","):
        s = s.strip()
        if s.isdigit():
            sizes.append(int(s))

    png_to_ico(args.png, args.out, sizes)


if __name__ == "__main__":
    main()
