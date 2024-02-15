# deflate compressed stream utility

The `deflate` is a low level gzip utility to create a DEFLATE streams i.e. gzip file without checksum.
A gzip file consists of such DEFLATE streams/blocks and their SRC32 checksums to detect corrupted data.
Nowadays almost all data is transmitted online and data integrity is guarantied on a channel level (e.g. TLS/HTTPS).
So such deflate files are simpler and smaller than regular gzip files and compressed slightly faster because we skip the checksums calculation.
This is not a big optimization and for a better interoperability use gzip but on a large scale this may matter (battery life, cloud hoisting usage cost, less file sizes).

If you are just want better compression see also:
* pigz which is "gzip v2" and have higher compressions levels
* lz4 for a best compression speed
* brotli for web assets (js, svg, html)
* zstd for regular files with best compression/decompression ratio
* xz for the the best compression but much slower decompression

You can extract such DEFLATE stream from a gzip yourself by cutting header from begin and checksum from trailer: 
```
echo "hello" | pigz -i > hello.txt.gz
cat hello.txt.gz | tail --bytes=+11 | head --bytes=-8 > hello.txt.deflate
```

So you may need this deflate tool only if you want to generate from scratch. 
 
But when the `deflate` comes really helpful is when you want to make concatenation of two gzip web asset files.
Unfortunately browsers doesn't supports yet concatenated gzip files but they do supports concatenated raw DEFLATE streams.
The only one main problem is that such concatenable streams should be properly padded but you can't do that with regular gzip/pigz: 

```sh
echo "first" | pigz -i > f1.gz
echo "second" | pigz -i > f2.gz
cat f1.gz | tail --bytes=+11 | head --bytes=-8 > f1.deflate
cat f2.gz | tail --bytes=+11 | head --bytes=-8 > f2.deflate
echo -n -e "\x1f\x8b\x08\0\0\0\0\0\0\xff" > header.gz
cat header.gz f1.deflate f2.deflate > f12.gz
```

on decompression you'll get an error:
```
$ gzip -dck f12.gz
first

gzip: f12.gz: invalid compressed data--crc error

gzip: f12.gz: invalid compressed data--length error
```

So to do the same with the `deflate`:
```sh
echo "first" | deflate > first.txt.deflate
echo "second" | deflate -E > second.txt.deflate
echo -n -e "\x1f\x8b\x08\0\0\0\0\0\0\xff" > header.gz
cat header.gz first.txt.deflate second.txt.deflate > merged.txt.gz
```

Here the `first.txt.deflate` stream is padded (`00 00 FF FF`) but `second.txt.deflate` is terminated/finished i.e. without padding (see `-E` option).

When decompressing you'll still see an error in stderr but the data will be fully written to stdout:
```
$ gzip -dck merged.txt.gz > merged.txt
gzip: merged.gz: unexpected end of file
$ cat merged.txt
first
second
```

Modern browsers doesn't require the last stream to be finished so you can omit the `-E` option. 

### Debug
You can compress a file without actual compression i.e. with level 0. This will create a `data` stream.
 
```
deflate -v -k -0 f1.txt
deflate -v -k -0 -E -o f2.txt.deflate -f f2.txt
deflate -v -k -d -f f1.txt.deflate -o f1.decomressed.txt
```

Such data streams may be also useful to create some uncompressed place in gzip archive which can be easily altered.
For example you can add a license key into archive with program or change dynamically one part of the compressed html file.  

You can use https://github.com/madler/infgen to debug the resulted deflate file:
```
$ infgen f1.txt.deflate
! infgen 2.4 output
!
stored
data 'first
end
!
stored
end
!
infgen warning: incomplete deflate data
```

Note that in fact there is two blocks but the last one is empty and used for padding. 
The warning `incomplete deflate data` means that this stream is appendable and not finished.

## Install

From Ubuntu PPA:

    sudo add-apt-repository ppa:stokito/utils
    sudo apt update
    sudo apt install deflate

Debian package download and install:

    wget -O /tmp/deflate.deb https://github.com/stokito/deflate/releases/download/v0.3.0/deflate.deb
    sudo dpkg -i /tmp/deflate.deb

Manual download:

    wget -O /tmp/deflate https://github.com/stokito/deflate/releases/download/v0.3.0/deflate
    sudo mv /tmp/deflate /usr/bin/
    sudo chmod +x /usr/bin/deflate

## Build

```
cmake ./
make
make install
```

### OpenWrt
Copy the `Makefile` from `package` to `~/workspace/openwrt/package/utils/deflate`

Then select `Utilities -> deflate` in SDK:
 
```
make menuconfig
make
```

Sources downloaded and unpacked here to `~/workspace/openwrt/build_dir/target-mips_24kc_musl/deflate-0.0.2`

You can recompile the sources:
```bash
~/workspace/openwrt$ make package/utils/deflate/{clean,compile} V=sc
```

the resulted IPK is in `~/workspace/openwrt/bin/packages/mips_24kc/base/` and it's just is regular `tar.gz` file.

Upload to router and install:
```
scp ~/workspace/openwrt/bin/packages/mips_24kc/base/deflate_0.2.0-1_mips_24kc.ipk 192.168.1.1:/tmp/
ssh 192.168.1.1
root@OpenWrt# opkg install /tmp/deflate_0.2.0-1_mips_24kc.ipk 
```

* https://electrosome.com/cross-compile-openwrt-c-program/
* https://openwrt.org/docs/guide-developer/crosscompile
* https://telecnatron.com/articles/Cross-Compiling-For-OpenWRT-On-Linux/index.html

#### Setup sdk locally
create `openwrt.env` file:

```sh
# Set up paths and environment for cross compiling for openwrt
export STAGING_DIR=/home/stokito/workspace/openwrt/staging_dir
export TOOLCHAIN_DIR=$STAGING_DIR/toolchain-mips_24kc_gcc-8.4.0_musl
export LDCFLAGS=$TOOLCHAIN_DIR/usr/lib
export LD_LIBRARY_PATH=$TOOLCHAIN_DIR/usr/lib
export PATH=$TOOLCHAIN_DIR/bin:$PATH

export CC=mips-openwrt-linux-gcc
export CXX=mips-openwrt-linux-g++
export LD=mips-openwrt-linux-ld
```

Then import this env by `source ~/workspace/openwrt/openwrt.env`
