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

project('amast', 'c',
        version : run_command(['python3', '-m', 'setuptools_scm'], check: true).stdout().strip(),
        default_options : [
            'c_std=c99',
            'warning_level=3',
            'buildtype=minsize',
            'b_lto=true',
            'werror=true',
            'default_library=static',
        ],
        meson_version: '>= 1.0.1')

add_project_arguments('-DAMAST_VERSION="@0@"'.format(meson.project_version()), language : 'c')

compiler = meson.get_compiler('c')
compiler_id = compiler.get_id()

extra_warning_args = []
if compiler_id == 'gcc' or compiler_id == 'clang'
    extra_warning_args += [
         '-Wchar-subscripts',
         '-Winit-self',
         '-Wmain',
         '-Wmissing-include-dirs',
         '-Wswitch-default',
         #'-Wswitch-enum',
         '-Wunused',
         '-Wunused-parameter',
         '-Wunused-variable',
         '-Wuninitialized',
         '-Wunknown-pragmas',
         '-Wstrict-overflow=5',
         '-Wmissing-format-attribute',
         '-Wfloat-equal',
         '-Wshadow',
         '-Wpointer-arith',
         #'-Wundef',
         '-Wunused-macros',
         '-Wcast-qual',
         '-Wwrite-strings',
         '-Wconversion',
         '-Wdangling-else',
         '-Wdate-time',
         '-Wmissing-declarations',
         '-Wpacked',
         '-Wredundant-decls',
         # '-Winline',
         '-Wdisabled-optimization',
         '-Winvalid-pch',
         '-Wmissing-noreturn',
         '-Wunreachable-code']

    extra_warning_args += [
         '-Wformat',
         '-Wformat=2',
         '-Wformat-nonliteral',
         '-Wformat-security',
         '-Wformat-y2k',
         '-Wcast-align']

    if compiler.has_argument('-Wno-gnu-zero-variadic-macro-arguments')
         extra_warning_args += ['-Wno-gnu-zero-variadic-macro-arguments']
    endif
    if compiler.has_argument('-Wnull-dereference')
         extra_warning_args += ['-Wnull-dereference']
    endif
    if compiler.has_argument('-Wno-deprecated-copy-with-user-provided-copy')
         extra_warning_args += ['-Wno-deprecated-copy-with-user-provided-copy']
    endif

    extra_warning_args += ['-Wno-declaration-after-statement']

endif

add_project_arguments(
    [
        extra_warning_args,
        '-fno-strict-aliasing',
        '-fmessage-length=0',
        '-fno-common',
        '-ffunction-sections',
        '-fdata-sections',
        '-fno-builtin',
        '-ftrapv',
        '-funwind-tables',
        '-fno-omit-frame-pointer'
    ],
    language : 'c')

add_project_link_arguments([
    '-Wl,--gc-sections',
    '-Wl,-Map=output.map'],
    language : 'c')


CLANG_FORMAT = find_program('clang-format', required: false, disabler: true)
CLANG_TIDY = find_program('clang-tidy', required: false, disabler: true)
RUN_CLANG_TIDY = find_program('run-clang-tidy', required: false, disabler: true)
CPPCHECK = find_program('cppcheck', required: false, disabler: true)
FIND = find_program('find', required: false, disabler: true)
PYTHON = find_program('python3')
IWYU = find_program('include-what-you-use', required: false, disabler: true)

unit_test = get_option('unit_test')

libm_dep = compiler.find_library('m', required: true)

libs_inc = include_directories(['libs', '.'])
subdir('libs')
subdir('apps' / 'unity')
subdir('apps' / 'examples')

files_to_check = run_command(
    FIND,
    meson.project_source_root() / 'libs',
    meson.project_source_root() / 'apps',
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
        RUN_CLANG_TIDY,
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

run_target(
    'iwyu',
    command: [
        PYTHON,
        '../.pixi/envs/default/bin/iwyu_tool.py',
        '-j',
        '@0@'.format(run_command('nproc', check: true).stdout().strip()),
        '-p',
        meson.project_build_root(),
        files_to_check,
        '--',
        '-Xiwyu',
        '--error=1',
        '-Xiwyu',
        '--no_comments',
        '-Xiwyu',
        '--no_fwd_decls'
    ],
)
