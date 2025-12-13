import os
import argparse
import traceback
from PIL import Image, UnidentifiedImageError

def scale_image(input_path, output_path, width, height, keep_aspect=False, force_upscale=True):
    """
    Scale image to width x height.
    - keep_aspect: if True, keep aspect ratio (thumbnail). If original is smaller and force_upscale=False, won't enlarge.
    - force_upscale: when keep_aspect=True, allow enlarging by resizing explicitly if True.
    """
    input_path = os.path.abspath(input_path)
    output_path = os.path.abspath(output_path)

    print(f"INPUT:  {input_path}")
    print(f"OUTPUT: {output_path}")
    print(f"TARGET: {width} x {height}   keep_aspect={keep_aspect}  force_upscale={force_upscale}")

    if not os.path.isfile(input_path):
        raise FileNotFoundError(f"Input image not found: {input_path}")

    try:
        with Image.open(input_path) as img:
            print(f"Original format: {img.format}, mode: {img.mode}, size: {img.width}x{img.height}")

            # Normalize mode for processing
            if img.mode not in ("RGB", "RGBA"):
                img = img.convert("RGBA")
                print(f"Converted mode -> {img.mode}")

            if keep_aspect:
                # If original smaller and we don't want to upscale, thumbnail will not enlarge.
                # To allow enlargement while preserving aspect, compute scale manually.
                orig_w, orig_h = img.width, img.height
                if (orig_w >= width and orig_h >= height) or not force_upscale:
                    # use thumbnail (in-place) which preserves aspect and does not upscale
                    img.thumbnail((width, height), Image.LANCZOS)
                    print(f"Thumbnail applied -> size now {img.width}x{img.height}")
                else:
                    # need to upscale while preserving aspect: compute scale factor and resize
                    scale = min(width / orig_w, height / orig_h)
                    new_w = max(1, int(round(orig_w * scale)))
                    new_h = max(1, int(round(orig_h * scale)))
                    img = img.resize((new_w, new_h), Image.LANCZOS)
                    print(f"Upscaled preserving aspect -> size now {img.width}x{img.height}")
            else:
                # forced resize (may change aspect ratio)
                img = img.resize((width, height), Image.LANCZOS)
                print(f"Forced resize -> size now {img.width}x{img.height}")

            # Ensure output directory exists
            out_dir = os.path.dirname(output_path)
            if out_dir:
                os.makedirs(out_dir, exist_ok=True)

            # Determine save params by extension
            _, ext = os.path.splitext(output_path)
            ext = ext.lower().lstrip(".")
            save_kwargs = {}
            save_format = None
            if ext in ("jpg", "jpeg"):
                save_format = "JPEG"
                # JPEG doesn't support alpha
                if img.mode == "RGBA":
                    img = img.convert("RGB")
                save_kwargs["quality"] = 95
            elif ext == "png":
                save_format = "PNG"
                save_kwargs["optimize"] = True
            elif ext == "webp":
                save_format = "WEBP"
                save_kwargs["quality"] = 95
            elif ext == "bmp":
                save_format = "BMP"
            else:
                # Let Pillow infer format from output_path or use original
                save_format = None

            img.save(output_path, format=save_format, **save_kwargs)
            print(f"SAVED: {output_path}  (format={save_format or 'auto'})")
            return output_path

    except UnidentifiedImageError:
        print("ERROR: The file is not a valid image or Pillow cannot identify it.")
        raise
    except Exception:
        print("ERROR during image processing:")
        traceback.print_exc()
        raise


def main():
    parser = argparse.ArgumentParser(description="Scale a single image (debug mode).")
    parser.add_argument("--in", required=True, dest="input_path", help="Input image path")
    parser.add_argument("--out", required=True, dest="output_path", help="Output image path")
    parser.add_argument("--w", required=True, type=int, help="Target width")
    parser.add_argument("--h", required=True, type=int, help="Target height")
    parser.add_argument("--keep-aspect", action="store_true", help="Keep aspect ratio (thumbnail mode)")
    parser.add_argument("--no-upscale", action="store_true", help="When keep-aspect, do not upscale smaller images")
    args = parser.parse_args()

    scale_image(args.input_path, args.output_path, args.w, args.h,
                keep_aspect=args.keep_aspect, force_upscale=not args.no_upscale)


if __name__ == "__main__":
    main()
