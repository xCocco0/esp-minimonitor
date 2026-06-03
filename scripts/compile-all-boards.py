#!/usr/bin/env python3

import os
import shutil
import subprocess
import time
import sys
import argparse

def run_build(version=None, release_dir=None):
    targets = [
        "waveshare_c6_lcd_1_47",
        "waveshare_s3_touch_lcd_1_47",
        "waveshare_s3_touch_lcd_2"
    ]

    # determine version

    versionfile = "version.txt"
    if version is None:
        if os.path.exists(versionfile):
            with open(versionfile, "r") as f:
                version = f.read().strip()
                print(f"Found version: {version}")
        if not version:
            version = input("Enter version [Default: none]: ").strip()
            if not version:
                version = "none"

    # get output directory

    if release_dir is None:
        default_path = "releases"
        release_dir = input(f"Enter output directory [Default: {default_path}]: ").strip()
        if not release_dir:
            release_dir = default_path

    # create directories
    os.makedirs(release_dir, exist_ok=True)

    # build for each target

    for target in targets:

        title = f"Building for target: {target}"
        space = 80 - (len(title) + 2)
        print("\n" + "-"*80)
        print("-"*int(space/2) + f" {title} " + "-"*(space-int(space/2)))
        print("-"*80)

        # Cleanup: Remove old build artifacts and components
        paths_to_clean = ["build", "managed_components", "sdkconfig", "dependencies.lock"]
        for path in paths_to_clean:
            if os.path.isdir(path):
                shutil.rmtree(path)
            elif os.path.isfile(path):
                os.remove(path)

        # Build process
        try:
            os.makedirs("build", exist_ok=True)

            subprocess.run(
                ["cmake", "..", f"-DMM_TARGET={target}", "-G", "Ninja"],
                cwd="build",
                check=True
            )

            # Compile/Merge with Ninja
            subprocess.run(["ninja", "merge-bin"], cwd="build", check=True)

            # Export the result
            file_name = f"esp-minimonitor-{version}-{target}.bin"
            destination = os.path.join(release_dir, file_name)
            shutil.copy("build/merged-binary.bin", destination)

            print(f"Success: Saved to {destination}")

        except subprocess.CalledProcessError as e:
            print(f"Error building {target}: {e}")
            continue

    print("-"*80)

if __name__ == "__main__":

    SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
    ROOT_DIR = os.path.dirname(SCRIPT_DIR)
    os.chdir(ROOT_DIR)

    parser = argparse.ArgumentParser(description=__file__)
    parser.add_argument("-V", "--version", default=None,
            help="The version string of the release")
    parser.add_argument("-o", "--outdir", default=None,
            help="The directory where to copy firmware binaries")

    args = parser.parse_args()

    start = time.time()

    run_build(version=args.version, release_dir=args.outdir)

    elaps = time.time() - start

    print("Completed.")
    if elaps > 0:
        print("Elapsed:", end="")
        if elaps / 60 >= 1:  print(" %d minutes" % (elaps / 60), end="")
        if elaps % 60 >= 1:  print(" %d seconds" % (elaps % 60), end="")
        print(".")
