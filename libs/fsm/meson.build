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

fsm_src = [files('fsm.c')]

libfsm = library(
    'fsm',
    [fsm_src],
    c_args: ['-fno-sanitize=all', '-Os', '-fno-trapv'],
    include_directories : [inc]
)

libfsm_dep = declare_dependency(
    sources: fsm_src,
    include_directories : [inc]
)

libraries += libfsm

if unit_test
    e = executable(
        'history',
        [
            'tests' / 'history.c'
        ],
        dependencies: [libfsm_dep, libassert_dep],
        include_directories: [include_directories('tests')])
    test('history', e, suite: 'fsm')

    e = executable(
        'redispatch',
        [
            'tests'/ 'redispatch.c'
        ],
        dependencies: [libfsm_dep, libassert_dep],
        include_directories: [include_directories('tests')])
    test('redispatch', e, suite: 'fsm')

    e = executable(
        'dtor',
        [
            'tests'/ 'dtor.c'
        ],
        dependencies: [libfsm_dep, libassert_dep],
        include_directories: [include_directories('tests')])
    test('dtor', e, suite: 'fsm')

    e = executable(
        'reenter',
        [
            'tests' / 'reenter.c',
        ],
        dependencies: [libfsm_dep, libstr_dep, libassert_dep],
        include_directories: [include_directories('tests')])
    test('reenter', e, suite: 'fsm')

    e = executable(
        'event_queue',
        [
            'tests' / 'event_queue.c',
        ],
        dependencies: [libstr_dep, libfsm_dep, libevent_dep, libpal_dep, libassert_dep],
        include_directories: [include_directories('tests')])
    test('event_queue', e, suite: 'fsm')
endif
