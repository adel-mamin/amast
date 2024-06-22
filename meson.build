#
# The MIT License (MIT)
#
# Copyright (c) 2020 Adel Mamin
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

project('tikli', 'c',
        meson_version: '>= 1.0.1')

CLANG_FORMAT = find_program('clang-format', required: false, disabler: true)
CLANG_TIDY = find_program('clang-tidy', required: false, disabler: true)
CPPCHECK = find_program('cppcheck', required: false, disabler: true)
FIND = find_program('find', required: false, disabler: true)
PYTHON = find_program('python3')

unit_test = get_option('unit_test')

subdir('src')

files_to_check = run_command(
    FIND,
    meson.project_source_root() / 'src',
    '-name',
    '*.h',
    '-or',
    '-name',
    '*.c',
    check: true,
).stdout().strip().split('\n')

run_target(
    'style',
    command: [
        CLANG_FORMAT,
        '--style=file',
        '-i',
        files_to_check,
    ]
)

run_target(
    'style-dry',
    command: [
        CLANG_FORMAT,
        '--style=file',
        '-i',
        files_to_check,
        '--dry-run',
        '--Werror',
    ]
)

run_target(
    'tidy',
    command: [
        PYTHON,
        meson.project_source_root() / 'tools' / 'python' / 'run-clang-tidy.py',
        '-clang-tidy-binary',
        CLANG_TIDY,
        '-quiet',
        '-p',
        meson.project_build_root(),
        files_to_check,
    ],
)

run_target(
    'cppcheck',
    command: [
        CPPCHECK,
        '-j',
        '@0@'.format(run_command('nproc').stdout().strip()),
        '--error-exitcode=2',
        '--enable=warning,style',
        '--inline-suppr',
        '--suppress=sizeofDivisionMemfunc',
        '--suppress=*:/opt/*',
        '--suppress=*:/usr/*',
        '--project=@0@'.format(meson.project_build_root() / 'compile_commands.json')
    ],
)
