import sys
import re

def compute_translation_id(message_xml: str) -> int:
    import re
    # 提取 name
    name_match = re.search(r'name="([^"]+)"', message_xml)
    if not name_match:
        raise ValueError("无法找到 message name")
    name = name_match.group(1)

    # 提取内容
    content_match = re.search(r'>(.*?)</message>', message_xml, re.DOTALL)
    if not content_match:
        raise ValueError("无法找到 message 内容")
    content_lines = content_match.group(1).splitlines()
    content = "".join(line.strip() for line in content_lines)

    combined = name + content
    data = combined.encode("utf-8")

    # 64-bit FNV-1a
    fnv_offset_basis = 0xcbf29ce484222325
    fnv_prime = 0x100000001b3

    h = fnv_offset_basis
    for byte in data:
        h ^= byte
        h = (h * fnv_prime) & 0xffffffffffffffff
    return h


def main():
    if len(sys.argv) != 2:
        print("用法: python calculateTranslation.py <message_file>")
        sys.exit(1)

    file_path = sys.argv[1]
    with open(file_path, 'r', encoding='utf-8') as f:
        message_xml = f.read()

    translation_id = compute_translation_id(message_xml)
    # 格式化为十进制无符号整数
    print(f"Translation ID: {translation_id}")

if __name__ == "__main__":
    main()
