from conan import ConanFile
from conan.tools.cmake import CMakeDeps
from conan.tools.files import copy
from conan.errors import ConanInvalidConfiguration
import os

required_conan_version = ">=2.0.14"


class xSDK(ConanFile):
    settings = "os", "arch", "compiler", "build_type"

    def requirements(self):
        self.requires("gtest/1.14.0")

    def generate(self):
        tc = CMakeDeps(self)
        tc.generate()

