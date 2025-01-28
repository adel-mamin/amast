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

import json
import os
import sys


def filter_compile_commands(input_file, output_file, switches_to_remove):
    """
    Parses the input compile_commands.json, removes specified switches,
    and writes the result to output_file.

    Args:
        input_file (str): Path to the input compile_commands.json file.
        output_file (str): Path to the output file.
        switches_to_remove (list): List of compiler switches to remove.
    """
    try:
        with open(input_file, 'r') as f:
            compile_commands = json.load(f)
    except Exception as e:
        print(f"Error reading input file: {e}")
        sys.exit(1)

    # Process each command and remove the specified switches
    for command in compile_commands:
        original_command = command.get('command', '')
        for switch in switches_to_remove:
            # Remove the switch along with any trailing whitespace
            original_command = original_command.replace(switch, '')
        command['command'] = ' '.join(original_command.split())  # Normalize spaces

    # Write the modified commands to the output file
    try:
        with open(output_file, 'w') as f:
            json.dump(compile_commands, f, indent=2)
    except Exception as e:
        print(f"Error writing to output file: {e}")
        sys.exit(1)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python filter_compile_commands.py <input_file> <output_file> <switches_to_remove>")
        print('Example: python filter_compile_commands.py compile_commands.json filtered_compile_commands.json "-fno-trap"')
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]
    # Switches are provided as a comma-separated list
    switches_to_remove = sys.argv[3].split(',')

    filter_compile_commands(input_file, output_file, switches_to_remove)
