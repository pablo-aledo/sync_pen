all:
	g++ -g sync_via_pen.cpp -o sync_via_pen

install:
	sudo cp sync_via_pen /bin/
