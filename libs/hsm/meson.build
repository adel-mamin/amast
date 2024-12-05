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

hsm_src = [files(
    'hsm.c',
    'tests' / 'regular' / 'regular.c',
    'tests' / 'submachine' / 'calc' / 'calc.c',
    'tests' / 'submachine' / 'complex' / 'submachine.c',
)]
hsm_inc = include_directories(
    'tests' / 'submachine' / 'complex',
    'tests' / 'submachine' / 'calc',
    'tests' / 'regular',
    'tests',
)
src += [hsm_src]

libhsm = library(
    'hsm',
    [hsm_src],
    link_with: libassert,
    include_directories: [inc, hsm_inc]
)
libhsm_dep = declare_dependency(
    link_with : libhsm,
    include_directories : [inc, hsm_inc]
)

if unit_test
    e = executable(
        'regular',
        [
            'tests' / 'regular' / 'test.c'
        ],
        dependencies: [libhsm_dep, libstr_dep],
        include_directories: [include_directories('tests')])
    test('regular', e, suite: 'hsm')

    e = executable(
        'hsm_top_as_nca',
        [
            'tests' / 'hsm_top_as_nca.c'
        ],
        dependencies: [libhsm_dep, libstr_dep],
        include_directories: [include_directories('tests')])
    test('hsm_top_as_nca', e, suite: 'hsm')

    e = executable(
        'redispatch',
        [
            'tests'/ 'redispatch.c'
        ],
        dependencies: [libhsm_dep, libstr_dep],
        include_directories: [include_directories('tests')])
    test('redispatch', e, suite: 'hsm')

    e = executable(
        'dtor',
        [
            'tests'/ 'dtor.c'
        ],
        dependencies: [libhsm_dep, libstr_dep],
        include_directories: [include_directories('tests')])
    test('dtor', e, suite: 'hsm')

    e = executable(
        'history',
        [
            'tests' / 'history.c'
        ],
        dependencies: [libhsm_dep, libstr_dep],
        include_directories: [include_directories('tests')])
    test('history', e, suite: 'hsm')

    e = executable(
        'submachine-basic',
        [
            'tests' / 'submachine' / 'basic' / 'test.c'
        ],
        dependencies: [libhsm_dep, libstr_dep],
        include_directories: [include_directories('tests')])
    test('submachine-basic', e, suite: 'hsm')

    e = executable(
        'submachine-complex',
        [
            'tests' / 'submachine' / 'complex' / 'test.c'
        ],
        dependencies: [libhsm_dep, libstr_dep],
        include_directories: [include_directories('tests')])
    test('submachine-complex', e, suite: 'hsm')

    e = executable(
        'submachine-calc',
        [
            'tests' / 'submachine' / 'calc' / 'test.c'
        ],
        dependencies: [libhsm_dep, libstr_dep, libm_dep],
        include_directories: [include_directories('tests')])
    test('submachine-calc', e, suite: 'hsm')

    e = executable(
        'spy',
        [
            'tests' / 'spy.c',
            hsm_src,
        ],
        c_args: ['-DAM_HSM_SPY'],
        dependencies: [libstr_dep, libassert_dep],
        include_directories: [include_directories('tests')])
    test('spy', e, suite: 'hsm')

    e = executable(
        'reenter',
        [
            'tests' / 'reenter.c',
        ],
        dependencies: [libstr_dep, libhsm_dep],
        include_directories: [include_directories('tests')])
    test('reenter', e, suite: 'hsm')

    e = executable(
        'event_queue',
        [
            'tests' / 'event_queue.c',
        ],
        dependencies: [libstr_dep, libhsm_dep, libevent_dep, libpal_dep],
        include_directories: [include_directories('tests')])
    test('event_queue', e, suite: 'hsm')

    e = executable(
        'defer',
        [
            'tests' / 'defer.c',
        ],
        dependencies: [libstr_dep, libhsm_dep, libevent_dep, libpal_dep],
        include_directories: [include_directories('tests')])
    test('defer', e, suite: 'hsm')
endif
