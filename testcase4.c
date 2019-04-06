#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "filesys.h"

#define NUM_FILES 8


static int corrupt_file (char *filename)
{
	int fd1;
	int ret;
	char val;
	int offset;

	fd1 = open (filename, O_WRONLY, 0);
	if (fd1 == -1) {
		printf ("Unable to open file descriptor1\n");
		return 0;
	}

	offset = rand () % 1000;
	lseek (fd1, offset, SEEK_SET);
	val = 2;
	ret = write (fd1, &val, 1);
	if (ret != 1) {
		printf ("Unable to write to file\n");
		return 0;
	}
	close (fd1);
	return 1;
}

int main ()
{
  char filename[32];
	int corrupt_idx = rand() % NUM_FILES;
	int ret;

	snprintf (filename, 32, "foo_%d.txt", corrupt_idx);
	ret = corrupt_file (filename);
	if (ret == 0) {
		return 0;
	}

	if (filesys_init() == 0) {
		printf ("init filesys test failed\n");
		return 0;
	}

	printf ("init filesys test passed\n");
	return 0;
}
