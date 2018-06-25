CFLAGS = -Wall

all: cp mkdir ln rm restore checker
	
cp: utils
	gcc ${CFLAGS} ext2_cp.c utils.o -o ext2_cp -g

mkdir: utils
	gcc ${CFLAGS} ext2_mkdir.c utils.o -o ext2_mkdir -g

ln: utils
	gcc ${CFLAGS} ext2_ln.c utils.o -o ext2_ln -g

rm: utils
	gcc ${CFLAGS} ext2_rm.c utils.o -o ext2_rm -g

restore: utils
	gcc ${CFLAGS} ext2_restore.c utils.o -o ext2_restore -g

checker: utils
	gcc ${CFLAGS} ext2_checker.c utils.o -o ext2_checker -g

utils: utils.c utils.h
	gcc ${CFLAGS} utils.c -c -g

clean:
	rm *.o 