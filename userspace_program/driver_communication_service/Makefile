BUILD_TYPE ?= "Release"

pre_build:
	conan install . -if build --build=missing -s build_type=${BUILD_TYPE}

build: pre_build
	cmake . -Bbuild
	make -C build -j12

clean:
	rm -rf build

test_build: pre_build
	cmake . -Bbuild -DBUILD_TEST=1

test: test_build
	make -C build -j12
	./build/bin/unit_tests