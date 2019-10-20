from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

ext_modules = [
    Extension(
        name='xfast_tests',
        sources=['tests.pyx'],
        include_dirs=['../src'],
        extra_compile_args=['-std=c++11', '-stdlib=libc++', '-mmacosx-version-min=10.9'],
        extra_link_args=['-std=c++11', '-mmacosx-version-min=10.9'],
    ),
]

setup(ext_modules=cythonize(ext_modules))
