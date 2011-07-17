SUBDIRS=src
DISTCLEANFILES=fortune.txt

all: fortune.txt
fortune.txt: fortunizer.pl
	if [ ! -x ./$< ]; then chmod +x ./$<; fi
	cat fortunes/*.fortune | ./$< >$@

