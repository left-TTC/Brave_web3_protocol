import os
import argparse
import json
import shutil

parser = argparse.ArgumentParser(description="Brave Patch Tool")
parser.add_argument("--src", required=True, help="Path to Brave source directory")

args = parser.parse_args()

# get the absolute src path
src_dir = os.path.abspath(args.src)

if not os.path.isdir(src_dir):
    print(f"Error: Source directory does not exist: {src_dir}")
else:
    print(f"Source directory absolute path: {src_dir}")




# confirm the version
package_json_path = os.path.join(src_dir, "brave", "package.json")

if not os.path.isfile(package_json_path):
    print(f"Error: package.json not found at {package_json_path}")
    exit(1)

with open(package_json_path, "r", encoding="utf-8") as f:
    package_data = json.load(f)

version = package_data.get("version")
if version:
    print(f"Brave version: {version}")
else:
    print("Error: 'version' field not found in package.json")
    exit(2)




# change the code

## check patch root floder
CURRENT_DIR = os.getcwd()
PATCH_DIR = os.path.join(CURRENT_DIR, "patch")

if not os.path.isdir(PATCH_DIR):
    print(f"Error: patch folder not found in: {CURRENT_DIR}")
    exit(4)

print("Patch folder found:", PATCH_DIR)


## Confirm version directory exists
def confirm_version(patch_dir, version):
    version_path = os.path.join(patch_dir, version)

    if os.path.isdir(version_path):
        print(f"Version patch folder found: {version_path}")
        return True
    else:
        print(f"Error: patch folder for version '{version}' not found in {patch_dir}")
        return False


if not confirm_version(PATCH_DIR, version):
    print("No this version's code")
    exit(4)

version_dir = os.path.join(PATCH_DIR, version)
patch_src_dir = os.path.join(version_dir, "src")
brave_src_dir = src_dir


## apply patchs
def apply_patch(brave_src, patch_src):
    """
    Recursively copy all files from patch_src to brave_src.
    Existing files are overwritten; new files are created.
    """
    if not os.path.isdir(patch_src):
        print(f"No src directory inside version patch: {patch_src}")
        return False

    for root, dirs, files in os.walk(patch_src):
        relative_path = os.path.relpath(root, patch_src)
        target_dir = os.path.join(brave_src, relative_path)

        os.makedirs(target_dir, exist_ok=True)

        for file in files:
            src_file = os.path.join(root, file)
            dest_file = os.path.join(target_dir, file)

            print(f"Patching: {src_file} -> {dest_file}")
            shutil.copy2(src_file, dest_file)

    return True


print("Applying patch...")
if apply_patch(brave_src_dir, patch_src_dir):
    print("Patch applied successfully.")
else:
    print("Patch failed.")
    exit(6)


    
