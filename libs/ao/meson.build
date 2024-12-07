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

ao_src = [files('ao.c')]
ao_c_args = []
if ao_type == 'cooperative'
    ao_src += [files('cooperative' / 'port.c')]
    ao_c_args += ['-DAM_AO_COOPERATIVE']
elif ao_type == 'preemptive'
    ao_src += [files('preemptive' / 'port.c')]
    ao_c_args += ['-DAM_AO_PREEMPTIVE']
endif
src += ao_src

libao = library(
    'ao',
    [ao_src],
    c_args: ao_c_args,
    include_directories: [
        inc,
    ]
)
libao_dep = declare_dependency(link_with: libao, include_directories: [inc])
