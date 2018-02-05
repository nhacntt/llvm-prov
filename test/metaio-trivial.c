/**
 * @file   metaio-trivial.c
 * @brief  metaio propagation with trivial data flow analysis
 *
 * RUN: %clang %cflags -S %s -D SOURCE="\"%s\"" -D DEST="\"%t.c\"" -emit-llvm -o %t.ll
 * RUN: clang-declaration-tool %s -o posix.yaml -- clang -D SOURCE="\"\"" -D DEST="\"\"" -I "../include/"
 * RUN: %prov -S %t.ll -o %t.prov.ll
 * RUN: %filecheck %s -input-file %t.ll -check-prefix LLCHECK
 * RUN: %filecheck %s -input-file %t.prov.ll -check-prefix PROVCHECK
 */

#include <unistd.h>

void foo()
{
	// LLCHECK: [[X:%[a-z0-9]+]] = alloca [[INT:i[0-9]+]]
	// LLCHECK: [[Y:%[a-z0-9]+]] = alloca [[INT]]
	// LLCHECK: [[Z:%[a-z0-9]+]] = alloca [[INT]]
	// PROVCHECK-DAG: [[METAIO:%[a-z0-9]+]] = alloca %struct.metaio
	// PROVCHECK: [[X:%[a-z0-9]+]] = alloca [[INT:i[0-9]+]]
	// PROVCHECK: [[Y:%[a-z0-9]+]] = alloca [[INT]]
	// PROVCHECK: [[Z:%[a-z0-9]+]] = alloca [[INT]]
	int x;

	// LLCHECK: [[X_AS_BUFFER:%[0-9]+]] = bitcast [[INT]]* [[X]] to i8*
	// LLCHECK: call [[SSIZE:i[0-9]+]] {{.*}}read{{["]?}}([[INT]] 0, i8* [[X_AS_BUFFER]]
	// PROVCHECK: [[X_AS_BUFFER:%[0-9]+]] = bitcast [[INT]]* [[X]] to i8*
	// PROVCHECK: call [[SSIZE:i[0-9]+]] @{{"*}}metaio_{{.*}}read{{"*}}([[INT]] 0, i8* [[X_AS_BUFFER]], [[SSIZE]] {{[0-9]+}}, %struct.metaio* [[METAIO]])
	read(0, &x, sizeof(x));

	// LLCHECK: [[XVAL:%[0-9]+]] = load [[INT]], [[INT]]* [[X]]
	// LLCHECK: store [[INT]] [[XVAL]], [[INT]]* [[Y]]
	int y = x;

	// LLCHECK: [[YVAL:%[0-9]+]] = load [[INT]], [[INT]]* [[Y]]
	// LLCHECK: store [[INT]] [[YVAL]], [[INT]]* [[Z]]
	int z = y;

	// LLCHECK: [[Z_AS_BUFFER:%[0-9]+]] = bitcast [[INT]]* [[Z]] to i8*
	// LLCHECK: call [[SSIZE]] {{.*}}write{{["]?}}([[INT]] 1, i8* [[Z_AS_BUFFER]]
	// PROVCHECK: [[Z_AS_BUFFER:%[0-9]+]] = bitcast [[INT]]* [[Z]] to i8*
	// PROVCHECK: call [[SSIZE]] @{{"*}}metaio_{{.*}}write{{"*}}([[INT]] 1, i8* [[Z_AS_BUFFER]], [[SSIZE]] 4, %struct.metaio* [[METAIO]])
	write(1, &z, sizeof(z));
}
