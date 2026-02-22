from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import sys
import setuptools
import pybind11
from pybind11.setup_helpers import Pybind11Extension, build_ext

ext_modules = [
    Pybind11Extension(
        "carlo_pricer",
        ["bindings.cpp"],
        include_dirs=[pybind11.get_include(), "."],
        language='c++',
        extra_compile_args=['-std=c++17'],
    ),
]

setup(
    name="carlo_pricer",
    version="1.0.0",
    author="Your Name",
    description="Monte Carlo Option Pricing with Longstaff-Schwartz Algorithm",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.6",
)
