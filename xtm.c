/*
 * Copyright (c) 2011, Julien Laffaye <jlaffaye@FreeBSD.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

#define OFFSET_NAMELEN 40
#define OFFSET_NAME 41
#define OFFSET_NUM 92
#define OFFSET_SIZE 96
#define OFFSET_DATA 104

struct xtm_header {
	uint8_t name_len;
	char name[51];
	uint8_t has_md5;
	uint32_t num_files;
	uint64_t size;
};

static ssize_t
append_xtm(int from, int to)
{
	char buf[1024 * 10];
	ssize_t sz;
	ssize_t total = 0;

	for (;;) {
		sz = read(from, buf, sizeof(buf));
		if (sz < 0)
			err(1, "read()");
		else if (sz == 0)
			return total;

		total += sz;
		
		if (write(to, buf, sz) != sz)
			err(1, "write()");
	}
}

static int
get_info(struct xtm_header *h, const char *file)
{
	int fd;

	bzero(h, sizeof(struct xtm_header));

	fd = open(file, O_RDONLY);
	if (fd < 0)
		err(1, "open(%s)", file);

	lseek(fd, OFFSET_NAMELEN, SEEK_SET);
	read(fd, &h->name_len, 1);

	read(fd, &h->name, 50);
	h->name[h->name_len] = '\0';

	read(fd, &h->has_md5, 1);

	read(fd, &h->num_files, 4);

	read(fd, &h->size, 8);

	return fd;
}

static void
print_info(struct xtm_header *h)
{
	printf("Name: %s\n", h->name);
	printf("Has md5: %d\n", h->has_md5);
	printf("Number of files: %d\n", h->num_files);
	printf("Size: %ld\n", h->size);
	printf("\n\n");
}

int
main(int argc, char **argv)
{
	struct xtm_header h;
	uint8_t num_files;
	ssize_t size = 0;
	mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;
	int from, to;
	int i;
	
	if (argc == 1)
		return 1;

	from = get_info(&h, argv[1]);

	if (0)
		print_info(&h);

	num_files = argc - 1;
	if (h.num_files != num_files)
		errx(1, "got %d files, expecting %d", num_files, h.num_files);

	to = open(h.name, O_WRONLY|O_CREAT|O_TRUNC, mode);
	if (to < 0)
		err(1, "open(%s)", h.name);

	size += append_xtm(from, to);
	close(from);

	for (i = 2; i < argc; i++) {
		printf("%d/%d\n", i, num_files);

		from = open(argv[i], O_RDONLY);
		if (from < 0)
			err(1, "open(%s)", argv[i]);

		size += append_xtm(from, to);

		close(from);

	}
	printf("\n");

	close(to);

	if ((uint64_t)size != h.size)
		errx(1, "wrong total size %ld, expected %ld", (long)size,
(long)h.size);

	return 0;
}
