import os

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMakeDeps
from conan.tools.files import copy

required_conan_version = ">=2.8.1"


class xSDK(ConanFile):
    settings = "os", "arch", "compiler", "build_type"

    def _get_opt(self, optname, defval):
        return os.environ.get(optname, defval) == "ON"

    def requirements(self):
        self.requires("gtest/1.14.0")

    def generate(self):
        tc = CMakeDeps(self)
        tc.generate()
