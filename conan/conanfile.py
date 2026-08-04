from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import copy, save
from conan.tools.scm import Version


class QivotConan(ConanFile):
    name = "qivot"
    version = "1.0.0"
    license = "MIT"
    author = "Austin Kottke <austinkottke@gmail.com>"
    url = "https://github.com/austinkottke/Qivot"
    homepage = "https://github.com/austinkottke/Qivot"
    description = "Qt ORM with JSON-over-HTTP — declare models in C++, query them, and pull REST APIs"
    topics = ("qt", "orm", "database", "sql")

    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_network": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_network": True,
    }

    exports_sources = "src/*", "cmake/*", "CMakeLists.txt", "LICENSE.txt"
    generators = "cmake_find_package"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self, src_folder=".")

    def requirements(self):
        qt_version = "6.5.0"
        self.requires(f"qt/{qt_version}", force=True)

        if self.options.with_network:
            self.requires(f"qt/{qt_version}")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["QIVOT_WITH_NETWORK"] = "ON" if self.options.with_network else "OFF"
        tc.variables["QIVOT_BUILD_EXAMPLES"] = "OFF"
        tc.variables["QIVOT_BUILD_TESTS"] = "OFF"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

        # Copy license
        copy(self, "LICENSE.txt", self.source_folder,
             dst=self.package_folder, keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["qivot"]

        if self.options.with_network:
            self.cpp_info.defines = ["QIVOT_WITH_NETWORK"]

        # Qt dependencies
        self.cpp_info.requires = [
            "qt::qtcore",
            "qt::qtsql",
        ]

        if self.options.with_network:
            self.cpp_info.requires.append("qt::qtnetwork")
