#!/bin/sh
clear

if [ $# -ne 1 ];then 
	echo "./build.sh 1/0(all/part)"
	exit
fi

#0
mk_build=$1

if [ ${mk_build} -eq 1 ];then

	mk_cmake=1
	mk_nasm=1
	mk_x264=1
	mk_x265=1
	mk_ffmpeg=1
	mk_test=1
	rm -rfv  cmake nasm x264 x265 ffmpeg
else
	mk_cmake=0
	mk_nasm=0
	mk_x264=0
	mk_x265=0
	mk_ffmpeg=0
	mk_test=1
fi


#1
if [ ${mk_build} -eq 1 ];then
	#wget https://github.com/Kitware/CMake/releases/download/v3.25.0-rc2/cmake-3.25.0-rc2.tar.gz --no-check-certificate
	#wget https://www.nasm.us/pub/nasm/releasebuilds/2.15/nasm-2.15.tar.gz --no-check-certificate
	#wget https://download.videolan.org/videolan/x264/snapshots/x264-snapshot-20191217-2245.tar.bz2 --no-check-certificate
	#wget https://download.videolan.org/videolan/x265/x265_3.2.1.tar.gz --no-check-certificate
	#wget https://ffmpeg.org/releases/ffmpeg-snapshot.tar.bz2 --no-check-certificate

	tar xvf tar/cmake-3.25.0-rc2.tar.gz
	tar xvf tar/nasm-2.15.tar.gz
	tar xvf tar/x264-snapshot-20191217-2245.tar.bz2
	tar xvf tar/x265_3.2.1.tar.gz
	tar xvf tar/ffmpeg-4.4.3.tar.gz

	mv cmake-3.25.0-rc2 cmake
	mv nasm-2.15 nasm
	mv x264-snapshot-20191217-2245 x264
	mv x265_3.2.1 x265
	mv ffmpeg-4.4.3 ffmpeg

	mkdir -p $(pwd)/cmake/_install
	mkdir -p $(pwd)/nasm/_install
	mkdir -p $(pwd)/x264/_install
	mkdir -p $(pwd)/x265/_install
	mkdir -p $(pwd)/ffmpeg/_install
fi

install_cmake=$(pwd)/cmake/_install
install_nasm=$(pwd)/nasm/_install
install_x264=$(pwd)/x264/_install
install_x265=$(pwd)/x265/_install
install_ffmpeg=$(pwd)/ffmpeg/_install

#2
PATH_CMAKE=${install_cmake}/bin
PATH_NASM=${install_nasm}/bin
PATH_X264=${install_x264}/bin
PATH_X265=${install_x265}/bin
PATH_FFMPEG=${install_ffmpeg}/bin

PATH_PKG_X264=${install_x264}/lib/pkgconfig
PATH_INC_X264=${install_x264}/include/
PATH_LIB_X264=${install_x264}/lib/

PATH_PKG_X265=${install_x265}/lib/pkgconfig
PATH_INC_X265=${install_x265}/include/
PATH_LIB_X265=${install_x265}/lib/

PATH_PKG_FFMPEG=${install_ffmpeg}/lib/pkgconfig
PATH_INC_FFMPEG=${install_ffmpeg}/include/
PATH_LIB_FFMPEG=${install_ffmpeg}/lib/

test_lib=$(pwd)/test/lib/
test_inc=$(pwd)/test/inc/


#3
cur="$(pwd)"

#4
do_make()
{
	make clean
	wait
	make -j8
	wait
	if [ $? -ne 0 ];then
		echo "make err"
		exit
	fi
	make install
	wait
	cd $cur
}

#5
if [ ${mk_cmake} -eq 1 ];then
	cd cmake
	./bootstrap --prefix=${install_cmake}
	do_make
fi

if [ ${mk_nasm} -eq 1 ];then

	cd nasm
	./configure --prefix=${install_nasm}
	do_make
fi

if [ ${mk_x264} -eq 1 ];then

	cd x264
	export PATH=$PATH:${PATH_NASM}
	./configure --enable-static --prefix=${install_x264}
	do_make
	cp -rfv ${PATH_INC_X264}* $test_inc
	cp -rfv ${PATH_LIB_X264}* $test_lib
fi

if [ ${mk_x265} -eq 1 ];then

	cd x265/build/linux
	export PATH=$PATH:${PATH_CMAKE}
	export PATH=$PATH:${PATH_NASM}
	cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX="$install_x265" -DENABLE_SHARED:bool=off ../../source
	do_make
	cp -rfv ${PATH_INC_X265}* $test_inc
	cp -rfv ${PATH_LIB_X265}* $test_lib
fi

if [ ${mk_ffmpeg} -eq 1 ];then

	cd ffmpeg
	export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:${PATH_PKG_X264}
	export PATH=$PATH:${PATH_NASM}
	./configure --prefix=${install_ffmpeg} \
		--disable-shared \
		--enable-static \
		--enable-gpl \
		--enable-libx264 \
		--disable-asm \
		--extra-cflags=-I${PATH_INC_X264} \
		--extra-ldflags=-L${PATH_LIB_X264}
	do_make
	cp -rfv ${PATH_INC_FFMPEG}* $test_inc
	cp -rfv ${PATH_LIB_FFMPEG}* $test_lib
fi

if [ ${mk_test} -eq 1 ];then
	cd test
	make clean
	make
	./venctest
	cd $cur
fi
