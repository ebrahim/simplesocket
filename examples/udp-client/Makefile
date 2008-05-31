APP=udp-client
CPPFLAGS=-I../..

all: $(APP)

$(APP): $(APP).cpp
	$(LINK.cpp) $< -o $@

clean:
	rm -f *.o $(APP)

.PHONY: all clean
