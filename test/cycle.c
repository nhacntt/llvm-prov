/**
 * @file   cycle.c
 * @brief  Tests the detection of information flow cycles when indirect memory
 *         (store->load) flows exist together with direct operand flows.
 *
 * RUN: %clang %cflags -emit-llvm -S %s -o %t.ll
 * RUN: %opt -disable-output -graph-flows -flow-dir=%t.graphs %t.ll
 * RUN: %filecheck %s -input-file %t.graphs/foo.dot
 */

#include <err.h>
#include <fcntl.h>
#include <unistd.h>

void foo(int fd)
{
	char buffer[1024];

	// CHECK-DAG: [[N:"[0-9a-fx]+"]] [{{.*}}label = "{{ *}}%n = alloca i64
	ssize_t n = 0, total = 0;

	// CHECK-DAG: [[SUB:"[0-9a-fx]+"]] [{{.*}}label = "{{.*}}sub i64 1024, %
	// CHECK-DAG: [[N_SUB_LOAD:"[0-9a-fx]+"]] -> [[SUB]]
	// CHECK-DAG: [[READ:"[0-9a-fx]+"]] [{{.*}}label = "{{.*}}call {{.*}}read{{["]?}}(
	while ((n = read(fd, buffer + n, sizeof(buffer) - n)) > 0)
		// CHECK-DAG: [[ADD:"[0-9a-fx]+"]] [{{.*}}label = "{{.*}}add
		// CHECK-DAG: [[ADD]] -> [[N_SUB_LOAD]]
		total += n;
}
