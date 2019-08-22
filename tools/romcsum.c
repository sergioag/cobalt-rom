#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	unsigned int sum = 0;
	int fd;
	unsigned char c;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <file>\n", argv[0]);
		exit(1);
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "open(%s): %s\n", argv[1], strerror(errno));
		exit(2);
	}

	while (read(fd, &c, sizeof(c)) == sizeof(c)) {
		sum += c;
	}

	printf("0x%08x\n", sum);

	return 0;
}
	
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
