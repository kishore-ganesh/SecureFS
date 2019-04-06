#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "filesys.h"

int main ()
{
	int fd1, fd2;
	int ret;
	char val;

	if (filesys_init() == 1) {
		printf ("Unable to init filesys\n");
		return 0;
	}

	fd1 = open ("foo_0.txt", O_WRONLY, 0);
	if (fd1 == -1) {
		printf ("Unable to open file descriptor1\n");
		return 0;
	}

	ret = lseek (fd1, 0, SEEK_END);
	if (ret != 128000) {
		printf ("invalid file offset1 : %d\n", ret);
	}

	fd2 = s_open ("foo_0.txt", O_WRONLY, 0);
	if (fd2 == -1) {
		printf ("Unable to open file descriptor2\n");
		return 0;
	}

	val = 2;
	ret = write (fd1, &val, 1);
	if (ret != 1) {
		printf ("Unable to write to file\n");
		return 0;
	}

	ret = s_lseek (fd2, 0, SEEK_END);
	if (ret != 128000) {
		printf ("Seek test failed\n");
		return 0;
	}

	printf ("Seek test passed\n");
	close (fd1);
	s_close (fd2);
	return 0;
}
