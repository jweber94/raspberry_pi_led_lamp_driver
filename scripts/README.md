# Scripts to generate the debian package out of the project
+ In order to create debian packages, we use the `$ dpkg-deb --build <folder_name>` application. See https://agniva.me/debian/2016/05/30/debian-packaging.html for more details how to use it and which idea is behind it.

## Creating the debian package
+ You just need to use the `Makefile`:
    - `$ make build_pkg`
+ To clean up the repository:
    - `$ make clean`