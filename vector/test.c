#include <stdio.h>
#include <stdlib.h>
#include "vector.h"

void print_node(int index, void* data)
{
	int *ptr = data;
	fprintf(stderr, "[%d] = %d\n", index, *ptr);
}


int main(int argc, char** argv)
{
	int data;
	vector *v = NULL;

	v = vector_new(sizeof(void*), 100);

	for(data = 0;data < 10;++data)
	{
		vector_pushback(v, (void*)&data);
		vector_iterate(v, print_node);
		printf("\n");
	}

	for(data = 0;data < 10;++data)
	{
		vector_erase(v, 0);
		vector_iterate(v, print_node);
		printf("\n");
	}

	for(data = 0;data < 10;++data)
	{
		vector_pushback(v, (void*)&data);
		vector_iterate(v, print_node);
		printf("\n");
	}

	vector_delete(v);

	return EXIT_SUCCESS;
}