#
# The MIT License (MIT)
#
# Copyright (c) 2024 Adel Mamin
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

test_lib = library('test_lib', [hsm_src, 'common.c'], include_directories: inc)

if unit_test
    e = executable(
        'regular',
        [
            'common.c',
            'regular' / 'regular.c',
            'regular' / 'test.c'
        ],
        link_with: test_lib,
        include_directories: inc)
    test('regular', e, suite: 'hsm')

    e = executable(
        'regular-example',
        [
            'common.c',
            'regular' / 'regular.c',
            'regular' / 'example.c'
        ],
        link_with: test_lib,
        include_directories: inc)

    e = executable(
        'hsm_top_as_nca',
        ['hsm_top_as_nca.c'],
        link_with: test_lib,
        include_directories: inc)
    test('hsm_top_as_nca', e, suite: 'hsm')

    e = executable(
        'submachine',
        [
            'common.c',
            'submachine' / 'submachine.c',
            'submachine' / 'test.c'
        ],
        link_with: test_lib,
        include_directories: inc)
    test('submachine', e, suite: 'hsm')

    e = executable(
        'submachine-example',
        [
            'common.c',
            'submachine' / 'submachine.c',
            'submachine' / 'example.c'
        ],
        link_with: test_lib,
        include_directories: inc)
endif
