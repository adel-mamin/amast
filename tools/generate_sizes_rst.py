#!/usr/bin/env python3

#
# The MIT License (MIT)
#
# Copyright (c) Adel Mamin
#
# Source: https://github.com/adel-mamin/amast
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

import os
import subprocess
import argparse
import re

def get_sizes(library_path):
    """
    Extracts code size and data size from the library using the nm utility.
    """
    try:
        # Run the nm command and capture its output
        result = subprocess.run(["nm", "--print-size", "--size-sort", library_path],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        if result.returncode != 0:
            raise Exception(f"Error running nm on {library_path}: {result.stderr.strip()}")

        code_size = 0
        data_size = 0

        # Parse the nm output to accumulate sizes
        for line in result.stdout.splitlines():
            fields = line.split()
            if len(fields) < 3:
                continue

            size = int(fields[1], 16)  # Size is in hexadecimal
            section_type = fields[2].lower()

            if section_type == "t":  # (.text)
                code_size += size
            elif section_type in {"d", "b", "r"}:  # (.data, .bss, .rodata)
                data_size += size

        # Convert sizes to kilobytes
        return code_size / 1024, data_size / 1024

    except Exception as e:
        print(f"Failed to process {library_path}: {e}")
        return 0, 0

def extract_name(lib_name):
    # Use a regex to extract the substring between 'lib' and '.a'
    match = re.match(r'lib(.*)\.a', lib_name)
    if match:
        return match.group(1)
    return None  # If the string doesn't match the expected pattern

def generate_rst_table(output_file, libraries):
    """
    Generates an RST table with library names and their sizes.
    """
    table_header = (
        "Library name | Code size [kB] | Data size [kB]\n"
        "-------------|----------------|---------------\n"
    )

    rows = []
    for library in libraries:
        library_name = extract_name(os.path.basename(library))
        code_size, data_size = get_sizes(library)
        rows.append(f"{library_name} | {code_size:.2f} | {data_size:.2f}")

    # Write the table to the output file
    with open(output_file, "w") as f:
        f.write(table_header)
        f.write("\n".join(rows))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate RST table for static libraries.")
    parser.add_argument("output_file",
                        help="Path to the output RST file.")
    parser.add_argument("libraries", nargs="+",
                        help="List of static library files with full paths.")

    args = parser.parse_args()

    generate_rst_table(args.output_file, args.libraries)
