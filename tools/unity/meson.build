#
# The MIT License (MIT)
#
# Copyright (c) 2024 Adel Mamin
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

files_txt = configure_file(
    input: 'files.txt.in',
    output: 'files.txt',
    configuration: {
        'SRC_ROOT': meson.project_source_root(),
    }
)

configure_file(
    input: 'amast_config.h',
    output: 'amast_config.h',
    copy: true
)

amast_exe = executable(
    'amast-unity',
    ['unity.c', str_src],
    dependencies : [libassert_dep],
    c_args: ['-std=gnu99', '-ggdb', '-fno-lto'],
    include_directories: libs_inc
)

amast_makefile = custom_target(
    'amast_makefile',
    input : 'Makefile',
    output : 'Makefile',
    command : ['cp', '@INPUT@', '@OUTDIR@']
)

amast_src = custom_target(
    'amast_src',
    input : [files_txt],
    output : [
        'amast.c',              # 0
        'amast_test.c',         # 1
        'amast_freertos.c',     # 2
        'amast_posix.c',        # 3
        'amast_stubs.c',        # 4
        'amast_cooperative.c',  # 5
        'amast_preemptive.c',   # 6
        'amast.h',              # 7
        'amast_test.h'          # 8
    ],
    command : [
        amast_exe, '-f', '@INPUT@', '-o', '@OUTDIR@' , '&&',
        CLANG_FORMAT, '--style=file', '-i', '@OUTPUT@'
    ],
    build_always_stale: true,
    depends: [amast_exe, amast_makefile]
)

if unit_test
    e = executable(
        'unity',
        [
            amast_src[0], # 'amast.c'
            amast_src[1], # 'amast_test.c'
            amast_src[4], # 'amast_stubs.c'
        ],
        include_directories: libs_inc,
        c_args: [
            '-DAM_HSM_SPY',
            '-DAM_FSM_SPY',
            '-DAM_PAL_STUBS',
            '-ffunction-sections', '-fdata-sections'
        ],
        link_args: ['-Wl,--gc-sections'],
        dependencies: [libm_dep, libassert_dep]
    )
    test('unity', e)
endif
