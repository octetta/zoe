all: diditwork.zip

clean:
	rm -f bart DO-NOT-DELETE.zip diditwork.zip libzoe.a *.o

DO-NOT-DELETE.zip:
	zip DO-NOT-DELETE.zip DO-NOT-DELETE

diditwork.zip: bart DO-NOT-DELETE.zip
	cat bart DO-NOT-DELETE.zip > diditwork.zip
	chmod +x diditwork.zip

add: diditwork.zip
	zip -A diditwork.zip LICENSE
	zip -A diditwork.zip README.md
	zip -A diditwork.zip bart

del: diditwork.zip
	zip -d diditwork.zip LICENSE
	zip -d diditwork.zip README.md
	zip -d diditwork.zip bart

libzoe.a: zoe.c zoe.h miniz.c miniz.h
	gcc \
        -DMINIZ_NO_TIME \
        -DMINIZ_NO_ARCHIVE_WRITING_APIS \
        -DMINIZ_NO_ZLIB_APIS \
        -c miniz.c
	gcc -c zoe.c
	ar -cvq libzoe.a miniz.o zoe.o

bart: bart.c libzoe.a zoe.h
	gcc -I. -L. bart.c -o bart -lzoe
