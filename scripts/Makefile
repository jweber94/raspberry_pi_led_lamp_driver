create_deb:
	$(shell pwd)/create_deb.sh
	
build_pkg: create_deb
	dpkg-deb --build printer_lamp
	
clean:
	rm -rf printer_lamp
	rm printer_lamp.deb