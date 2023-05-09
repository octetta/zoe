all: diditwork.zip

clean:
	rm -f bart DO-NOT-DELETE.zip diditwork.zip

DO-NOT-DELETE.zip:
	zip DO-NOT-DELETE.zip DO-NOT-DELETE

diditwork.zip: bart DO-NOT-DELETE.zip
	cat bart DO-NOT-DELETE.zip > diditwork.zip
	chmod +x diditwork.zip

add: diditwork.zip
	zip -A diditwork.zip LICENSE
	zip -A diditwork.zip readme.md
	zip -A diditwork.zip ChangeLog.md
	zip -A diditwork.zip ../CRYSTAL17A.wav

del: diditwork.zip
	zip -d diditwork.zip LICENSE

bart: bart.c zoe.c zoe.h miniz.c miniz.h
	gcc \
        -DMINIZ_NO_TIME \
        -DMINIZ_NO_ARCHIVE_WRITING_APIS \
        -DMINIZ_NO_ZLIB_APIS \
        -I. \
        miniz.c \
        zoe.c \
        bart.c \
        -o bart
