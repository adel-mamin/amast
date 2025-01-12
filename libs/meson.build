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

inc = [include_directories('.')]

libraries = []

subdir('common')
subdir('pal')
subdir('blk')
subdir('crc')
subdir('slist')
subdir('dlist')
subdir('bit')
subdir('cobszpe')
subdir('queue')
subdir('strlib')
subdir('onesize')
subdir('event')
subdir('timer')
subdir('async')
subdir('ringbuf')
subdir('fsm')
subdir('hsm')
subdir('ao')

library_paths = []

foreach library : libraries
    library_paths += library.full_path()
endforeach

library_sizes_rst = custom_target(
    'library_sizes_rst',
    output: 'library_sizes.rst',
    command: [
        GENERATE_LIB_SIZES,
        '@OUTPUT@'
    ] + library_paths,
    depends: libraries,
    build_by_default: true
)

