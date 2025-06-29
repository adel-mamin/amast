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

event_src = [files('event.c')]

libevent = library(
    'event',
    [event_src],
    c_args: ['-fno-sanitize=all', '-Os', '-fno-trapv'],
    include_directories: [inc]
)

libevent_dep = declare_dependency(
    sources: [event_src],
    dependencies: [libonesize_dep, libqueue_dep],
    include_directories: [inc]
)

libraries += libevent

if unit_test
    e = executable(
        'event',
        ['event.c', 'test.c', slist_src],
        c_args: ['-DAM_EVENT_POOLS_NUM_MAX=32'],
        dependencies : [libonesize_dep, libqueue_dep, libassert_dep],
        include_directories: inc)
    test('event', e)
endif
