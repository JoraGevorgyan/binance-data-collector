from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout


class BinanceDataCollectorConan(ConanFile):
    name = "binance-data-collector"
    version = "1.0.0"
    description = "Binance market data collection and aggregation service (WebSocket)."
    settings = "os", "compiler", "build_type", "arch"

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("boost/1.83.0")
        self.requires("openssl/3.2.1")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

