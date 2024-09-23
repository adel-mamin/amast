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

bt_src = [files('bt.c')]
src += [bt_src]

libbt = library(
    'bt',
    [
        bt_src,
        dlist_src,
        hsm_src,
        timer_src,
    ],
    include_directories: inc)
libbt_dep = declare_dependency(link_with : libbt, include_directories : inc)

if unit_test
    e = executable(
        'invert',
        [
            'tests' / 'invert.c',
            'tests' / 'test_log.c',
            'tests' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('tests')])
    test('invert', e, suite: 'bt')

    e = executable(
        'force_success',
        [
            'tests' / 'force_success.c',
            'tests' / 'test_log.c',
            'tests' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('tests')])
    test('force_success', e, suite: 'bt')

    e = executable(
        'force_failure',
        [
            'tests' / 'force_failure.c',
            'tests' / 'test_log.c',
            'tests' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('tests')])
    test('force_failure', e, suite: 'bt')

    e = executable(
        'repeat',
        [
            'tests' / 'repeat.c',
            'tests' / 'test_log.c',
            'tests' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('tests')])
    test('repeat', e, suite: 'bt')

    e = executable(
        'retry_until_success',
        [
            'tests' / 'retry_until_success.c',
            'tests' / 'test_log.c',
            'tests' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('tests')])
    test('retry_until_success', e, suite: 'bt')

    e = executable(
        'run_until_failure',
        [
            'tests' / 'run_until_failure.c',
            'tests' / 'test_log.c',
            'tests' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('tests')])
    test('run_until_failure', e, suite: 'bt')

    e = executable(
        'delay',
        [
            'tests' / 'delay.c',
            'tests' / 'test_log.c',
            'tests' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep, libtimer_dep],
        include_directories: [include_directories('tests')])
    test('delay', e, suite: 'bt')

    e = executable(
        'count',
        [
            'tests' / 'count.c',
            'tests' / 'test_log.c',
            'tests' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep, libtimer_dep],
        include_directories: [include_directories('tests')])
    test('count', e, suite: 'bt')

    e = executable(
        'fallback',
        [
            'tests' / 'fallback.c',
            'tests' / 'test_log.c',
            'tests' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('tests')])
    test('fallback', e, suite: 'bt')

    e = executable(
        'sequence',
        [
            'tests' / 'sequence.c',
            'tests' / 'test_log.c',
            'tests' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('tests')])
    test('sequence', e, suite: 'bt')

    e = executable(
        'parallel',
        [
            'tests' / 'parallel.c',
            'tests' / 'test_log.c',
            'tests' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('tests')])
    test('parallel', e, suite: 'bt')
endif
