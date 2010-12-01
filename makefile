CXX=g++
CXX_FLAGS=-Wall
FILES=psftp.cpp arguments.cpp cmdmode.cpp controlconn.cpp dataconn.cpp messages.cpp operations.cpp
FILES_H= arguments.h cmdmode.h controlconn.h dataconn.h messages.h operations.h

all: $(FILES) $(FILES_H)
	$(CXX) -pthread -o psftp $(CXX_FLAGS) $(FILES)
	
.PHONY:
clean:
	rm -F psftp
	rm -F *.o
