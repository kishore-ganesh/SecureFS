#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "filesys.h"

#define NUM_FILES 8

int main_loop (char *filename)
{
	int fd1, fd2;
	char buf[128];
	int size, total_size, ret;

	fd1 = s_open (filename, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
	if (fd1 == -1) {
		printf ("Unable to open file descriptor1\n");
		return 0;
	}

	memset (buf, 1, 128);

	total_size = 128000;
	
	while (total_size) {
		size = (rand() % 127) + 1;
		if (size > total_size) {
			size = total_size;
		}
		ret = s_write (fd1, buf, size);
		if (ret != size) {
			printf ("Unable to write to file\n");
			return 0;
		}
		total_size -= size;
	}

	s_close (fd1);

	fd2 = s_open (filename, O_RDONLY, 0);
	if (fd2 == -1) {
		printf ("Unable to open file descriptor2\n");
		return 0;
	}
	total_size = 128000;
	while (total_size) {
		size = (total_size > 128) ? 128 : total_size;
		ret = s_read (fd2, buf, size);
		if (ret != size) {
			printf ("Unable to read from file %d\n", ret);
			return 0;
		}
		total_size -= size;
	}
	s_close (fd2);
	return 0;
}

int main ()
{
	int i;
	char filename[32];

	system ("rm -rf foo*.txt");

	if (filesys_init() == 1) {
		printf ("Unable to init filesys\n");
		return 0;
	}

	for (i = 0; i < NUM_FILES; i++) {
		snprintf (filename, 32, "foo_%d.txt", i);
		main_loop (filename);
	}

	return 0;
}
