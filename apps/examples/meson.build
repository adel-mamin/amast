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

e = executable(
    'hahi-example',
    ['hsm' / 'hahi' / 'hahi.c'],
    dependencies: [libhsm_dep, libstr_dep, libassert_dep])

e = executable(
    'regular-example',
    ['hsm' / 'regular' / 'example.c'],
    dependencies: [libhsm_dep, libhsm_test_dep, libstr_dep, libassert_dep])

e = executable(
    'submachine-example',
    ['hsm' / 'submachine' / 'complex' / 'example.c'],
    dependencies: [libhsm_dep, libhsm_test_dep, libstr_dep, libassert_dep])

e = executable(
    'calc-example',
    ['hsm' / 'submachine' / 'calc' / 'example.c'],
    dependencies: [libhsm_dep, libhsm_test_dep, libstr_dep, libassert_dep])

libdpp = library(
    'dpp',
    [
        'dpp' / 'main.c',
        'dpp' / 'philo.c',
        'dpp' / 'table.c'
    ],
    c_args : ['-DAM_ASSERT_FAILURE_ENABLED=0'],
    dependencies: [
        libhsm_dep,
        libstr_dep,
        libpal_dep,
        libevent_dep,
        libtimer_dep,
        libbit_dep])

dpp_preemptive = executable(
    'dpp_preemptive',
    dependencies: libao_preemptive_dep,
    link_with: libdpp)

dpp_cooperative = executable(
    'dpp_cooperative',
    dependencies: libao_cooperative_dep,
    link_with: libdpp)

if pal != 'stubs'
    test('threaded_preemptive', dpp_preemptive, suite: 'dpp')
    test('threaded_cooperative', dpp_cooperative, suite: 'dpp')
endif

libringbuf_example = library(
    'ringbuf_preemptive_example',
    [
        'ringbuf' / 'reader.c',
        'ringbuf' / 'writer.c'
    ],
    c_args : ['-DAM_ASSERT_FAILURE_ENABLED=0'],
    dependencies: [
        libpal_dep,
        libringbuf_dep,
        libassert_dep,
        libbit_dep,
    ])

ringbuf_preemptive = executable(
    'ringbuf_preemptive',
    ['ringbuf' / 'main.c'],
    link_with: libringbuf_example,
    dependencies: libao_preemptive_dep)

ringbuf_cooperative = executable(
    'ringbuf_cooperative',
    ['ringbuf' / 'main.c'],
    link_with: libringbuf_example,
    dependencies: libao_cooperative_dep)

if pal != 'stubs'
    test('threaded_preemptive', ringbuf_preemptive, suite: 'ringbuf')
    test('threaded_cooperative', ringbuf_cooperative, suite: 'ringbuf')
endif

async = executable(
    'async',
    ['async' / 'main.c'],
    dependencies: [
        libhsm_dep,
        libbit_dep,
        libasync_dep,
        libao_preemptive_dep,
        libpal_dep,
        libassert_dep,
    ]
)

fork = executable(
    'fork',
    ['fork' / 'main.c'],
    dependencies: [
        libhsm_dep,
        libasync_dep,
        libao_preemptive_dep,
        libpal_dep,
        libassert_dep,
        libbit_dep
    ]
)

workers = executable(
    'workers',
    ['workers' / 'main.c'],
    c_args : ['-DAM_ASSERT_FAILURE_ENABLED=0'],
    dependencies: [
        libhsm_dep,
        libao_preemptive_dep,
        libpal_dep,
        libassert_dep,
        libbit_dep
    ]
)

if pal != 'stubs'
    test('workers', workers, suite: 'workers')
endif
