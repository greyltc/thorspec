#!/bin/bash
#
# Zenwalk build file for thorspec
# Author: Mark Colclough
# 4 Oct 2009  -- first proper build
# 19 Oct 2010 -- Added thorspec GUI prog, modernised build

# Source http://www.cm.ph.bham.ac.uk/software/source/

srcname='thorspec'
# the following line is rewritten when a tarball is generated
srcversion="1.1"
srcextn='tar.gz'

tarballname=$srcname-${srcversion}.$srcextn
srcdirname=$srcname-${srcversion}
pkgname=$srcname
pkgversion='66.1'
arch='i486'
cpu='i686'
knowndeps='python,numpy,fltk,pyfltk,cartesian,multiplot,gnuplot,fxload'

#       |-----handy-ruler------------------------------------------------------|
descfile="\
$pkgname: $pkgname (Driver for Thorlabs SP1-USB SP2-USB and CCS spectrometers)
$pkgname:
$pkgname: Contains a C and python library for driving the Thorlabs
$pkgname: SP1-USB SP2-USB and CCS fibre-input optical spectrometers, along
$pkgname: with a test program and a GUI data acquisition program
$pkgname:
$pkgname:
$pkgname:
$pkgname:
$pkgname:
$pkgname:"

# Compute useful variables
builddir=$(pwd)
package="$pkgname-$srcversion-$arch-$pkgversion"
destdir="$builddir/$package"
tarball="$builddir/$tarballname"
srcdir="$builddir/$srcdirname"
buildlog="$builddir/$pkgname-buildlog"
INS755="install -m 755"

# Prevent any rm -rf accidents from happening too close to the root
if [ ${#builddir} -lt 9 ] ; then
	echo "You don't want to build in $builddir"
	exit 1
fi

(  # subshell to redirect all output to build log
echo "== Preparing the source tree =="
cd $builddir
rm -rf $srcdir
tar xf $tarball

echo "== preparing dest tree at $destdir =="
cd $builddir
rm -rf $destdir
mkdir -p $destdir/install
mkdir -p $destdir/usr/src/$pkgname-$srcversion
mkdir -p $destdir/usr/doc/$pkgname-$srcversion
echo "$descfile" > $destdir/install/slack-desc
$INS755 $builddir/build-$pkgname.sh $destdir/usr/src/$pkgname-$srcversion/

echo "== make =="
cd $srcdir
make

echo "== make install =="
cd $srcdir
make install DESTDIR=$destdir

echo "install docs =="
cd $srcdir
cp README TODO SPX_Drv.txt spxtest.c ILX526A.PDF $destdir/usr/doc/$pkgname-$srcversion/

echo "== strip and compress =="
cd $destdir
find . | xargs file | grep "executable" | grep ELF | cut -f 1 -d : | \
  xargs strip --strip-unneeded 2> /dev/null
find . | xargs file | grep "shared object" | grep ELF | cut -f 1 -d : | \
  xargs strip --strip-unneeded 2> /dev/null

echo "== making package =="
cd $destdir
/sbin/makepkg -l y -c n $destdir.txz
cd $builddir
md5sum $package.txz >$package.md5

echo "== verifying package =="
cd $builddir
zen-pkgcheck.pl $package.txz 2>&1

echo "== writing dependencies =="
echo $knowndeps > $package.dep

echo "== all done =="
) 2>&1 | tee $buildlog
