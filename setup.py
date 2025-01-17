from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

extensions = [Extension("Tools.backtestEngine",
                        ["Tools/backtestEngine.pyx"],
                        extra_compile_args = ["-ffast-math"])]

extensions = cythonize(extensions, annotate=True)

setup(
    ext_modules = extensions
)
