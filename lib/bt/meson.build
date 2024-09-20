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
            'test' / 'invert.c',
            'test' / 'test_log.c',
            'test' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('test')])
    test('invert', e, suite: 'bt')

    e = executable(
        'force_success',
        [
            'test' / 'force_success.c',
            'test' / 'test_log.c',
            'test' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('test')])
    test('force_success', e, suite: 'bt')

    e = executable(
        'force_failure',
        [
            'test' / 'force_failure.c',
            'test' / 'test_log.c',
            'test' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('test')])
    test('force_failure', e, suite: 'bt')

    e = executable(
        'repeat_success',
        [
            'test' / 'repeat' / 'success.c',
            'test' / 'test_log.c',
            'test' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('test')])
    test('repeat_success', e, suite: 'bt')

    e = executable(
        'repeat_failure',
        [
            'test' / 'repeat' / 'failure.c',
            'test' / 'test_log.c',
            'test' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('test')])
    test('repeat_failure', e, suite: 'bt')

    e = executable(
        'retry_until_success_limited',
        [
            'test' / 'retry_until_success' / 'limited.c',
            'test' / 'test_log.c',
            'test' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('test')])
    test('retry_until_success_limited', e, suite: 'bt')

    e = executable(
        'retry_until_success_infinite',
        [
            'test' / 'retry_until_success' / 'infinite.c',
            'test' / 'test_log.c',
            'test' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('test')])
    test('retry_until_success_infinite', e, suite: 'bt')

    e = executable(
        'run_until_failure_first',
        [
            'test' / 'run_until_failure' / 'fail_first.c',
            'test' / 'test_log.c',
            'test' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('test')])
    test('run_until_failure_first', e, suite: 'bt')

    e = executable(
        'run_until_failure_second',
        [
            'test' / 'run_until_failure' / 'fail_second.c',
            'test' / 'test_log.c',
            'test' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep],
        include_directories: [include_directories('test')])
    test('run_until_failure_second', e, suite: 'bt')

    e = executable(
        'delay_success',
        [
            'test' / 'delay' / 'success.c',
            'test' / 'test_log.c',
            'test' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep, libtimer_dep],
        include_directories: [include_directories('test')])
    test('delay_success', e, suite: 'bt')

    e = executable(
        'delay_failure',
        [
            'test' / 'delay' / 'failure.c',
            'test' / 'test_log.c',
            'test' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep, libtimer_dep],
        include_directories: [include_directories('test')])
    test('delay_failure', e, suite: 'bt')

    e = executable(
        'fallback_success_first',
        [
            'test' / 'fallback' / 'success_first.c',
            'test' / 'test_log.c',
            'test' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep, libtimer_dep],
        include_directories: [include_directories('test')])
    test('fallback_success_first', e, suite: 'bt')

    e = executable(
        'fallback_success_second',
        [
            'test' / 'fallback' / 'success_second.c',
            'test' / 'test_log.c',
            'test' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep, libtimer_dep],
        include_directories: [include_directories('test')])
    test('fallback_success_second', e, suite: 'bt')

    e = executable(
        'fallback_failure',
        [
            'test' / 'fallback' / 'failure.c',
            'test' / 'test_log.c',
            'test' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep, libtimer_dep],
        include_directories: [include_directories('test')])
    test('fallback_failure', e, suite: 'bt')

    e = executable(
        'fallback_jump_second',
        [
            'test' / 'fallback' / 'jump_second.c',
            'test' / 'test_log.c',
            'test' / 'test_event.c',
        ],
        dependencies: [libhsm_dep, libbt_dep, libstr_dep, libtimer_dep],
        include_directories: [include_directories('test')])
    test('fallback_jump_second', e, suite: 'bt')
endif
