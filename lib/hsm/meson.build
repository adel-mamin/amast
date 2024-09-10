#
# The MIT License (MIT)
#
# Copyright (c) Adel Mamin
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

hsm_src = [files('hsm.c')]
src += [hsm_src]

libhsm = library('libhsm', [hsm_src], include_directories: inc)
libhsm_dep = declare_dependency(link_with : libhsm, include_directories : inc)

if unit_test
    e = executable(
        'regular',
        [
            'test' / 'regular' / 'regular.c',
            'test' / 'regular' / 'test.c'
        ],
        dependencies: [libhsm_dep, libstr_dep],
        include_directories: [include_directories('test')])
    test('regular', e, suite: 'hsm')

    e = executable(
        'hsm_top_as_nca',
        [
            'test' / 'hsm_top_as_nca.c'
        ],
        dependencies: [libhsm_dep, libstr_dep],
        include_directories: [include_directories('test')])
    test('hsm_top_as_nca', e, suite: 'hsm')

    e = executable(
        'redispatch',
        [
            'test'/ 'redispatch.c'
        ],
        dependencies: [libhsm_dep, libstr_dep],
        include_directories: [include_directories('test')])
    test('redispatch', e, suite: 'hsm')

    e = executable(
        'history',
        [
            'test' / 'history.c'
        ],
        dependencies: [libhsm_dep, libstr_dep],
        include_directories: [include_directories('test')])
    test('history', e, suite: 'hsm')

    e = executable(
        'submachine-basic',
        [
            'test' / 'submachine' / 'basic' / 'test.c'
        ],
        dependencies: [libhsm_dep, libstr_dep],
        include_directories: [include_directories('test')])
    test('submachine-basic', e, suite: 'hsm')

    e = executable(
        'submachine-complex',
        [
            'test' / 'submachine' / 'complex' / 'submachine.c',
            'test' / 'submachine' / 'complex' / 'test.c'
        ],
        dependencies: [libhsm_dep, libstr_dep],
        include_directories: [include_directories('test')])
    test('submachine-complex', e, suite: 'hsm')

endif

e = executable(
    'regular-example',
    [
        'test' / 'regular' / 'regular.c',
        'example' / 'regular' / 'example.c'
    ],
    dependencies: [libhsm_dep, libstr_dep],
    include_directories: [include_directories('test' / 'regular', 'test')])

e = executable(
    'submachine-example',
    [
        'test' / 'submachine' / 'complex' / 'submachine.c',
        'example' / 'submachine' / 'example.c'
    ],
    dependencies: [libhsm_dep, libstr_dep],
    include_directories: [
        include_directories(
            'test' / 'submachine' / 'complex',
            'test',
        )
])
