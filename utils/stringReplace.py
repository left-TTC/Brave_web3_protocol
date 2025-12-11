import os
import argparse

def main():
    parser = argparse.ArgumentParser(description="Simple text replace tool")
    parser.add_argument("--file", required=True, help="Path to the input file")
    parser.add_argument("--find", required=True, help="Text to find (case-sensitive)")
    parser.add_argument("--replace", required=True, help="Text to replace with")

    args = parser.parse_args()

    file_path = os.path.abspath(args.file)

    if not os.path.isfile(file_path):
        print(f"Error: File not found: {file_path}")
        return

    find_text = args.find
    replace_text = args.replace

    # read original file
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # do replacement
    new_content = content.replace(find_text, replace_text)

    # output file path
    base, ext = os.path.splitext(file_path)
    new_file = f"{base}_replaced_ture{ext}"

    # write new file
    with open(new_file, "w", encoding="utf-8") as f:
        f.write(new_content)

    print(f"Replacement completed. New file created:")
    print(new_file)


if __name__ == "__main__":
    main()
