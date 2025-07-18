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
            'buildtype=release',
            'werror=true',
            'b_lto=true',
            'default_library=static',
            'b_sanitize=address,undefined'
        ],
        meson_version: '>= 1.4.0')

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
         '-Wswitch-enum',
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
         '-Wundef',
         '-Wunused-macros',
         '-Wcast-qual',
         '-Wwrite-strings',
         '-Wconversion',
         '-Wdangling-else',
         '-Wdate-time',
         '-Wmissing-declarations',
         '-Wpacked',
         '-Wredundant-decls',
         '-Winline',
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
IWYU = find_program('iwyu_tool.py', required: false, disabler: true)
CSPELL = find_program('cspell', required: false, disabler: true)
GENERATE_LIB_SIZES = find_program('tools' / 'generate_lib_sizes.py')
FILTER_COMPDB = find_program('tools' / 'filter_compdb.py')
DOXYGEN = find_program('doxygen')
SPHINX = find_program('sphinx-build')

unit_test = get_option('unit_test')
pal = get_option('pal')

if unit_test
    add_project_arguments(['-DAMAST_PAL_STUBS'], language: 'c')
endif

libm_dep = compiler.find_library('m', required: true)

libs_inc = include_directories(['libs', '.'])

subdir('libs')
subdir('tools' / 'unity')
subdir('apps' / 'examples')

files_to_check = run_command(
    FIND,

    meson.project_source_root() / 'libs' / 'ao',
    meson.project_source_root() / 'libs' / 'async',
    meson.project_source_root() / 'libs' / 'bit',
    meson.project_source_root() / 'libs' / 'common',
    meson.project_source_root() / 'libs' / 'crc',
    meson.project_source_root() / 'libs' / 'dlist',
    meson.project_source_root() / 'libs' / 'event',
    meson.project_source_root() / 'libs' / 'fsm',
    meson.project_source_root() / 'libs' / 'hsm',
    meson.project_source_root() / 'libs' / 'ihsm',
    meson.project_source_root() / 'libs' / 'onesize',
    meson.project_source_root() / 'libs' / 'pal',
    meson.project_source_root() / 'libs' / 'queue',
    meson.project_source_root() / 'libs' / 'slist',
    meson.project_source_root() / 'libs' / 'strlib',
    meson.project_source_root() / 'libs' / 'timer',
    meson.project_source_root() / 'libs' / 'ringbuf',
    meson.project_source_root() / 'libs' / 'cobszpe',
    meson.project_source_root() / 'apps',
    meson.project_source_root() / 'tools',
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

compdb_tidy = custom_target(
    'compdb_tidy',
    output: 'compile_commands.json',
    command: [
        FILTER_COMPDB,
        meson.project_build_root() / 'compile_commands.json',
        '@OUTPUT@',
        '-fno-trapv'
    ]
)

run_target(
    'tidy',
    command: [
        RUN_CLANG_TIDY,
        '-clang-tidy-binary',
        CLANG_TIDY,
        '-j', '@0@'.format(run_command('nproc', check: true).stdout().strip()),
        '-use-color',
        '-quiet',
        '-p',
        meson.project_build_root(),
        files_to_check,
    ],
    depends: [
        compdb_tidy,
    ],
)

run_target(
    'cppcheck',
    command: [
        CPPCHECK,
        '-j',
        '@0@'.format(run_command('nproc', check: true).stdout().strip()),
        '-D__cppcheck__',
        '--error-exitcode=2',
        '--enable=warning,style',
        '--inline-suppr',
        '--check-level=exhaustive',
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
        IWYU,
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

run_target(
    'cspell',
    command: [
        CSPELL,
        '--root',
        meson.project_source_root(),
        '--unique',
        '--color',
        '--cache',
        '--config',
        meson.project_source_root() / 'cspell.json',
    ],
)

doxyfile = configure_file(
    input: 'Doxyfile.in',
    output: 'Doxyfile',
    configuration: {
        'SOURCE_ROOT': meson.project_source_root(),
        'OUTPUT_DIR': meson.project_build_root(),
    }
)

run_target(
    'doxygen',
    command: [
        DOXYGEN,
        doxyfile.full_path()
    ],
)

sphinx_cfg = configuration_data()
sphinx_cfg.set_quoted('DOXYGEN_PATH_XML', meson.project_build_root() / 'xml')
sphinx_cfg.set_quoted('HTML_STATIC_PATH', meson.project_source_root() / 'docs' / '_static')

configure_file(
    input: 'docs' / 'conf.py.in',
    output: 'conf.py',
    configuration: sphinx_cfg
)

run_target(
    'sphinx',
    command: [
        SPHINX,
        '-n',      # nit-picky mode
        '--color', # emit colored output
        '-W',      # turn warnings to errors
        '-T',      # show full traceback on exception
        '-b', 'html',
        '-c', meson.project_build_root(),              # path to conf.py
        meson.project_source_root() / 'docs',          # source dir
        meson.project_build_root() / 'docs' / 'sphinx' # output dir
    ],
)
