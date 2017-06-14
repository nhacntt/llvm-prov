/**
 * @file   callgraph.c
 * @brief  tests callgraph output
 *
 * RUN: %clang %cflags %s -emit-llvm -S -o %t.ll
 * RUN: %opt -callgraph -cg-format dot %t.ll -o /dev/null
 * RUN: %filecheck %s -input-file %t.ll.callgraph.dot
 */

#include <unistd.h>

int foo(int);
int bar(int);
void baz(int);
void wibble(void);

typedef int (*fooptr)(int);

int foo(int x)
{
	// CHECK-DAG: "foo" -> "printf"
	printf("x: %d\n", x);
	return x;
}

int bar(int y)
{
	// CHECK-DAG: "bar" -> "foo"
	return foo(y - 1) + foo(y + 1);
}

void baz(int z)
{
	// CHECK-DAG: "baz" -> "foo"
	foo(z - 2);
	// CHECK-DAG: "baz" -> "bar"
	bar(z);
	foo(z + 2);
}

void wibble()
{
	fooptr f = &foo;
	fooptr b = bar;

	// TODO: indirect calls
	f(10);
	b(20);
	f(30);
}
