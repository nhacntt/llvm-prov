/**
 * @file   cycle.c
 * @brief  Tests information flow tracking involving possible cycles
 *
 * RUN: %clang %cflags -emit-llvm -S %s -o %t.ll
 * RUN: %opt -disable-output -graph-flows %t.ll
 *
 * XFAIL: *
 */

#include <err.h>
#include <fcntl.h>
#include <unistd.h>

void foo(int fd)
{
	char buffer[1024];
	ssize_t n = 0, total = 0;
	while ((n = read(fd, buffer + n, sizeof(buffer) - n)) > 0)
		total += n;
}
