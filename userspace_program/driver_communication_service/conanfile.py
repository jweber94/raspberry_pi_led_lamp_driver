from conans import ConanFile, CMake

class PocoTimerConan(ConanFile):
   settings = "os", "compiler", "build_type", "arch"
   requires = (
      "boost/1.80.0",
      "cpputest/4.0",
      "dbus/1.15.2"
   ) # comma-separated list of requirements
   build_requires = (
      "boost/1.80.0",
      "cpputest/4.0", 
      "dbus/1.15.2"
   )
   generators = "cmake", "gcc", "txt"

   def imports(self):
      self.copy("*.dll", dst="bin", src="bin") # From bin to bin
      self.copy("*.dylib*", dst="bin", src="lib") # From lib to bin

   def build(self):
      cmake = CMake(self)
      cmake.configure()
      cmake.build()