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

static int open_file (char *filename)
{
	int fd2;

	fd2 = s_open (filename, O_RDONLY, 0);
	if (fd2 == -1) {
		return 0;
	}
	return 1;
}

int main ()
{
  char filename[32];
  int i, ret;
	int corrupt_idx = rand() % NUM_FILES;

	if (filesys_init() == 1) {
		printf ("Unable to init filesys\n");
		return 0;
	}
	snprintf (filename, 32, "foo_%d.txt", corrupt_idx);
	ret = corrupt_file (filename);
	if (ret == 0) {
		return 0;
	}

	for (i = 0; i < NUM_FILES; i++) {
		snprintf (filename, 32, "foo_%d.txt", i);
		ret = open_file (filename);
		if ((ret == 0 && i != corrupt_idx) || (ret == 1 && i == corrupt_idx)) {
			printf ("open test failed\n");
			return 0;
		}
	}

	printf ("open test passed\n");
	return 0;
}
