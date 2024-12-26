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

ao_src = [files('ao.c')]
ao_c_args = []

ao_src_cooperative = [files('cooperative' / 'port.c')]
ao_src_preemptive = [files('preemptive' / 'port.c')]

libao_cooperative = library(
    'ao_cooperative',
    [ao_src, ao_src_cooperative],
    c_args: ao_c_args,
    include_directories: [
        inc,
    ]
)

libao_preemptive = library(
    'ao_preemptive',
    [ao_src, ao_src_preemptive],
    c_args: ao_c_args,
    include_directories: [
        inc,
    ]
)

libao_cooperative_dep = declare_dependency(
    link_with: libao_cooperative,
    dependencies: [libevent_dep, libhsm_dep, libtimer_dep],
    include_directories: [inc]
)

libao_preemptive_dep = declare_dependency(
    link_with: libao_preemptive,
    dependencies: [libevent_dep, libhsm_dep, libtimer_dep],
    include_directories: [inc]
)

if unit_test
    e = executable(
        'minimal_preemptive',
        [
            'tests' / 'minimal.c'
        ],
        dependencies: [libao_preemptive_dep, libassert_dep, libpal_dep],
        include_directories: [include_directories('tests')])
    test('minimal_preemptive', e, suite: 'ao')

    e = executable(
        'minimal_cooperative',
        [
            'tests' / 'minimal.c'
        ],
        dependencies: [
            libao_cooperative_dep, libassert_dep, libpal_dep, libbit_dep
        ],
        include_directories: [include_directories('tests')])
    test('minimal_cooperative', e, suite: 'ao')
endif
