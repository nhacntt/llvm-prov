/**
 * @file   metaio-ops.c
 * @brief  metaio propagation with some arithmetic operations thrown in
 *
 * REQUIRES: freebsd
 *
 * RUN: %clang %cflags -S %s -D SOURCE="\"%s\"" -D DEST="\"%t.c\"" -emit-llvm -o %t.ll
 * RUN: %prov -S %t.ll -o %t.prov.ll
 * RUN: %filecheck %s -input-file %t.prov.ll
 */

#include <unistd.h>

int my_global = 7;

void foo()
{
	// CHECK-DAG: [[METAIO:%[a-z0-9]+]] = alloca %struct.metaio
	// CHECK: [[X:%[a-z0-9]+]] = alloca [[INT:i[0-9]+]]
	// CHECK: [[Y:%[a-z0-9]+]] = alloca [[INT]]
	// CHECK: [[Z:%[a-z0-9]+]] = alloca [[INT]]
	int x;

	// CHECK: [[X_AS_BUFFER:%[0-9]+]] = bitcast [[INT]]* [[X]] to i8*
	// CHECK: call [[SSIZE:i[0-9]+]] @metaio_read([[INT]] 0, i8* [[X_AS_BUFFER]], [[SSIZE]] {{[0-9]+}}, %struct.metaio* [[METAIO]])
	read(0, &x, sizeof(x));

	int y = (x + my_global) % 5;

	int z = y;

	// CHECK: [[Z_AS_BUFFER:%[0-9]+]] = bitcast [[INT]]* [[Z]] to i8*
	// CHECK: call [[SSIZE]] @metaio_write([[INT]] 1, i8* [[Z_AS_BUFFER]], [[SSIZE]] 4, %struct.metaio* [[METAIO]])
	write(1, &z, sizeof(z));
}
