import os
import re

from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy, load

required_conan_version = ">=2.0"


class QivotConan(ConanFile):
    """Builds Qivot from this checkout.

    From the repository root:
        conan create conan --build=missing
    The version comes from project(VERSION ...) in CMakeLists.txt.
    """

    name = "qivot"
    license = "MIT"
    url = "https://github.com/austinkottke/Qivot"
    homepage = "https://github.com/austinkottke/Qivot"
    description = ("Qt ORM: declare models in C++, query and migrate SQLite, PostgreSQL, "
                   "MySQL, SQL Server and DuckDB, and map JSON (optionally over HTTP) onto them.")
    topics = ("qt", "orm", "database", "sql", "sqlite", "json")

    # Qivot's classes carry no export macros, so it is a static library only.
    package_type = "static-library"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "fPIC": [True, False],
        "with_network": [True, False],
    }
    default_options = {
        "fPIC": True,
        "with_network": True,
    }

    @property
    def _root(self):
        # The recipe lives in conan/; the sources are one level up.
        return os.path.join(self.recipe_folder, os.pardir)

    def set_version(self):
        cmakelists = load(self, os.path.join(self._root, "CMakeLists.txt"))
        match = re.search(r"project\(\s*Qivot\s+VERSION\s+([0-9.]+)", cmakelists)
        self.version = match.group(1)

    def export_sources(self):
        for pattern in ("CMakeLists.txt", "LICENSE.txt", "cmake/*", "src/*"):
            copy(self, pattern, self._root, self.export_sources_folder)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        # Qivot's headers include Qt's, so Qt is a transitive header dependency.
        self.requires("qt/[>=6.5 <7]", transitive_headers=True, transitive_libs=True)

    def validate(self):
        check_min_cppstd(self, 17)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["QIVOT_WITH_NETWORK"] = bool(self.options.with_network)
        tc.cache_variables["QIVOT_BUILD_EXAMPLES"] = False
        tc.cache_variables["QIVOT_WITH_QML"] = False
        tc.generate()
        CMakeDeps(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "LICENSE.txt", self.source_folder, os.path.join(self.package_folder, "licenses"))
        CMake(self).install()

    def package_info(self):
        # Consumers write find_package(Qivot) and link Qivot::qivot, as with a
        # plain CMake install. The headers are included as <qivot.h>.
        self.cpp_info.set_property("cmake_file_name", "Qivot")
        self.cpp_info.set_property("cmake_target_name", "Qivot::qivot")
        self.cpp_info.libs = ["qivot"]
        self.cpp_info.includedirs = ["include/qivot"]
        self.cpp_info.requires = ["qt::qtCore", "qt::qtSql"]
        if self.options.with_network:
            self.cpp_info.requires.append("qt::qtNetwork")
        if self.settings.compiler in ("gcc", "clang", "apple-clang"):
            self.cpp_info.cxxflags = ["-Wno-invalid-offsetof"]
