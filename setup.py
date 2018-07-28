#from distutils.core import setup
#from Cython.Build import cythonize

#setup(
#    name = "backtesting",
#    ext_modules = cythonize("Tools/backtesting.pyx")
#)
from setuptools import setup, find_packages, Extension
from Cython.Distutils import build_ext

ext_modules=[
    Extension("Tools.backtesting",    # location of the resulting .so
             ["Tools/backtesting.pyx"], extra_compile_args = ["-ffast-math"]) ]

setup(name='backtesting',
      packages=find_packages(),
      cmdclass = {'build_ext': build_ext},
      ext_modules = ext_modules,
     )

#annotate=True, # to see highlighted code in html
