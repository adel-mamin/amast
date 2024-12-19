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

e = executable(
    'regular-example',
    [
        'hsm' / 'regular' / 'example.c'
    ],
    dependencies: [libhsm_dep, libstr_dep, libassert_dep])

e = executable(
    'submachine-example',
    [
        'hsm' / 'submachine' / 'complex' / 'example.c'
    ],
    dependencies: [libhsm_dep, libstr_dep, libassert_dep])

e = executable(
    'calc-example',
    [
        'hsm' / 'submachine' / 'calc' / 'example.c'
    ],
    dependencies: [libhsm_dep, libstr_dep, libassert_dep])

e = executable(
    'dpp-example',
    [
        'dpp' / 'main.c',
        'dpp' / 'philo.c',
        'dpp' / 'table.c'
    ],
    dependencies: [
        libhsm_dep,
        libstr_dep,
        libpal_dep,
        libao_preemptive_dep,
        libevent_dep,
        libtimer_dep,
        libbit_dep])

ringbuf_example = executable(
    'ringbuf-example',
    [
        'ringbuf' / 'main.c',
        'ringbuf' / 'reader.c',
        'ringbuf' / 'writer.c'
    ],
    dependencies: [
        libao_preemptive_dep,
        libpal_dep,
        libringbuf_dep,
        libassert_dep
    ])

if unit_test
    test('threaded', ringbuf_example, suite: 'ringbuf')
endif
