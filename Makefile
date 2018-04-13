PKGNAME := thorspec

all: spxmodule.so ccsmodule.so libspx.a libccs.a spxtest thorspecUI.py

# dunno how to build just the one extension module at a time, so must have both
# static libs as deps for each, else the bulid can fail
spxmodule.so: spxmodule.c spxdrv.h vitypes.h libspx.a libccs.a
	python setup.py -v  build -f

ccsmodule.so: ccsmodule.c CCS_Series_Drv.h vitypes.h libspx.a libccs.a
	python setup.py -v  build -f

spxtest: spxtest.c spxdrv.h vitypes.h libspx.a
	gcc -g -o spxtest spxtest.c -L. -lspx -lusb

libspx.a: spxdrv.o spxusb.o
	ar rcs libspx.a spxdrv.o spxusb.o

libccs.a: ccsdrv.o spxusb.o
	ar rcs libccs.a ccsdrv.o spxusb.o

spxusb.o: spxusb.c spxusb.h vitypes.h
	gcc -g -c -Wall -o spxusb.o spxusb.c

spxdrv.o: spxdrv.c spxdrv.h spxusb.h vitypes.h
	gcc -g -c -Wall -o spxdrv.o spxdrv.c

ccsdrv.o: CCS_Series_Drv.c CCS_Series_Drv.h
	gcc -g -c -Wall -Wfatal-errors -o $@ $<

ccstest: ccstest.c libccs.a
	gcc -g -o ccstest ccstest.c ccsdrv.o spxusb.o -lm -lusb

thorspecUI.py: thorspecUI.fl
	flconvert -o $@ $^
	patch < thorspecUI.patch
	
clean:
	rm -f *.o *.pyc *.a
	
veryclean: clean
	rm -rf build
	rm -f spxtest

install: all
	install -D -m644 spxdrv.h $(DESTDIR)/usr/include/spxdrv.h
	install -D -m644 CCS_Series_Drv.h $(DESTDIR)/usr/include/CCS_Series_Drv.h
	install -D -m644 vitypes.h $(DESTDIR)/usr/include/vitypes.h
	install -D -m644 libspx.a $(DESTDIR)/usr/lib/libspx.a
	install -D -m644 libspx.a $(DESTDIR)/usr/lib/libccs.a
	install -D -m644 80-spectrometer.rules $(DESTDIR)/lib/udev/rules.d/80-spectrometer.rules
	install -D -m644 firmware/CCS100_1.ihx $(DESTDIR)/lib/firmware/CCS100_1.ihx
	install -D -m755 thorspec-bin $(DESTDIR)/usr/bin/thorspec
	install -D -m644 thorspec.desktop $(DESTDIR)/usr/share/applications/thorspec.desktop
	install -D -m644 thorspec.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/apps/thorspec.svg
	# put python package in a directory to suit stupid setuptools
	rm -rf thorspec/*
	cp __init__.py thorspec.py thorspecUI.py thorspecversion.py zoomcanvas.py thorspec/
	python setup.py install --root=$(DESTDIR)


# Boilerplate to make a distribution tarball from a Mercurial archive.
# First, check that we are in a Mercurial archive
ifeq "$(shell test -d .hg && echo 'hg')" "hg"
# We force into the tarball a version.h file, and set the srcversion line
# in the Zenwalk build script: both with the version of the tarball being made.
# We also force in a version.py file
#
# If invoked as  'make dist tag=1.1'  the revision labelled by a preexisting
# tag is packaged with the name $(PKGNAME)-$(tag).tar.gz .
#
# If no tag is specified, the tip is used, and the tarball version is the revno,
# with optional prefix specified in DEV.
#
# Requires PKGNAME and optionally tag to be set elsewhere.
# The code ought to make use of the SPX_DRIVER_REVISION literal from the version.h header.
# The Zenwalk build file should have a srcversion= line.
#
# The false clause of DEV is used to add a prefix to the revno for non-tagged tarballs
DEV := $(if $(tag),,dev)
REV := $(if $(tag),$(tag),$(shell hg tip --template '{rev}\n'))
PKGVER := $(DEV)$(REV)
dist:
	rm -rf $(PKGNAME)-$(PKGVER)
	hg archive -t files -r $(REV) $(PKGNAME)-$(PKGVER)
	echo "#define SPX_DRIVER_REVISION \"$(PKGVER)\"" > $(PKGNAME)-$(PKGVER)/version.h
	echo "VERSION = \"$(PKGVER)\"" > $(PKGNAME)-$(PKGVER)/$(PKGNAME)version.py
	sed -i 's/^srcversion=.*/srcversion="$(PKGVER)"/' $(PKGNAME)-$(PKGVER)/build-$(PKGNAME).sh 
	tar cfz $(PKGNAME)-$(PKGVER).tar.gz $(PKGNAME)-$(PKGVER)
	rm -r $(PKGNAME)-$(PKGVER)

endif
# hg

# Make a tarball and immediately build a Zenwalk package from it
distbuild: dist
	rm -rf tmp
	mkdir tmp
	mv $(PKGNAME)-$(PKGVER).tar.gz tmp
	(cd tmp; tar xf $(PKGNAME)-$(PKGVER).tar.gz)
	cp tmp/$(PKGNAME)-$(PKGVER)/build-$(PKGNAME).sh tmp
	rm -r tmp/$(PKGNAME)-$(PKGVER)
	(cd tmp; fakeroot build-$(PKGNAME).sh)

.PHONY: dist distbuild install
