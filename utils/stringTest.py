import os
import argparse

def main():
    parser = argparse.ArgumentParser(description="Replace text with indexed text + filename in file")
    parser.add_argument("--file", required=True, help="Path to the input file")
    parser.add_argument("--find", required=True, help="Text to find (case-sensitive)")
    parser.add_argument("--replace", required=True, help="Replacement base text")

    args = parser.parse_args()

    file_path = os.path.abspath(args.file)

    if not os.path.isfile(file_path):
        print(f"Error: File not found: {file_path}")
        return

    find_text = args.find
    replace_text = args.replace

    # extract file name without extension
    filename = os.path.splitext(os.path.basename(file_path))[0]

    # read original file
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # perform indexed replacement
    index = 1
    result = ""
    pos = 0

    while True:
        idx = content.find(find_text, pos)
        if idx == -1:
            result += content[pos:]
            break

        # add unchanged part
        result += content[pos:idx]

        # replacement: base + index + filename
        replacement = f"{replace_text}{index}_{filename}"
        result += replacement
        index += 1

        # move pointer
        pos = idx + len(find_text)

    # output new file path
    base, ext = os.path.splitext(file_path)
    new_file = f"{base}_replaced{ext}"

    with open(new_file, "w", encoding="utf-8") as f:
        f.write(result)

    print(f"Replacement completed. New file created:")
    print(new_file)


if __name__ == "__main__":
    main()
