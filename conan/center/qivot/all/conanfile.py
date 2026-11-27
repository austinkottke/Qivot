import os

from conan import ConanFile
from conan.tools.build import check_min_cppstd, cross_building
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import copy, get, rmdir

required_conan_version = ">=2.0"


class QivotConan(ConanFile):
    name = "qivot"
    description = ("Qt ORM: declare models in C++, query and migrate SQLite, PostgreSQL, "
                   "MySQL, SQL Server and DuckDB, and map JSON or CBOR onto them.")
    license = "MIT"
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "https://github.com/austinkottke/Qivot"
    topics = ("qt", "orm", "database", "sql", "sqlite", "json", "cbor")
    # Qivot's classes carry no export macros, so it is a static library only.
    package_type = "static-library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "fPIC": [True, False],
        "with_network": [True, False],
    }
    default_options = {
        "fPIC": True,
        "with_network": True,
    }

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self, src_folder="src")

    def requirements(self):
        # Qivot's public headers include Qt's.
        self.requires("qt/[>=6.5 <7]", transitive_headers=True, transitive_libs=True)

    def validate(self):
        check_min_cppstd(self, 17)

    def build_requirements(self):
        # moc has to run on the build machine.
        if cross_building(self):
            self.tool_requires("qt/<host_version>")

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def generate(self):
        VirtualBuildEnv(self).generate()
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
        # Conan generates its own find_package() files.
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "Qivot")
        self.cpp_info.set_property("cmake_target_name", "Qivot::qivot")
        self.cpp_info.libs = ["qivot"]
        # Headers are included as <qivot.h>, <qimodel.h>, ...
        self.cpp_info.includedirs = ["include/qivot"]
        self.cpp_info.requires = ["qt::qtCore", "qt::qtSql"]
        if self.options.with_network:
            self.cpp_info.requires.append("qt::qtNetwork")
        # The field-offset tables use offsetof on non-standard-layout models.
        if self.settings.compiler in ("gcc", "clang", "apple-clang"):
            self.cpp_info.cxxflags = ["-Wno-invalid-offsetof"]
