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

ringbuf_src = [files('ringbuf.c')]

libringbuf = library(
    'ringbuf',
    [ringbuf_src],
    c_args: ['-fno-sanitize=all', '-Os', '-fno-trapv'],
    include_directories : [inc]
)

libringbuf_dep = declare_dependency(
    sources: ringbuf_src,
    include_directories : [inc]
)

libraries += libringbuf

if unit_test
    e = executable(
        'ringbuf',
        [
            'test.c'
        ],
        dependencies: [libringbuf_dep, libassert_dep],
        include_directories: [include_directories('.')])
    test('ringbuf', e, suite: 'ringbuf')
endif
