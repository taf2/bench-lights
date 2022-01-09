ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: bench

upload: bench
	pio run -j 2 --target upload && sleep 1;  pio device monitor

bench: src/main.cpp
	pio run

monitor:
	pio device monitor

configure:
	pio lib install "TinyPICO Helper Library"

clean:
	pio run --target clean
