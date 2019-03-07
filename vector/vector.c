#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vector.h"

struct node
{
	int	prev;
	int	next;
};

struct vector
{
	size_t		capacity;
	size_t		size;
	size_t		element_size;
	void*		data;
	struct node*	node;
	int		front;
	int		back;
	int		free;
};

static int vector_get_free_node(vector* obj)
{
	int free_node = -1;

	if(obj->free > -1)
	{
		free_node = obj->free;
		obj->free = obj->node[obj->free].next;
		obj->node[obj->free].prev = -1;
	}else if(obj->size < obj->capacity)
	{
		free_node = obj->size;
	}

	return free_node;
}

vector* vector_new(int element_size, int capacity)
{
	vector* obj = NULL;
	obj = calloc(1, sizeof(vector));
	obj->data = calloc(capacity, element_size);
	obj->node = calloc(capacity, sizeof(struct node));
	obj->front = -1;
	obj->back = -1;
	obj->free = -1;
	obj->capacity = capacity;
	obj->element_size = element_size;


	return obj;
}

void vector_delete(vector* obj)
{
	free(obj->node);
	free(obj->data);
	free(obj);
}

void vector_pushback(vector* obj, void* data)
{
	int node;
	node = vector_get_free_node(obj);

	if(node < 0)
		return;

	if(obj->back < 0)
	{
		obj->front = node;
		obj->back = node;
		obj->node[node].prev = -1;
		obj->node[node].next = -1;
	}else
	{
		obj->node[obj->back].next = node;
		obj->node[node].prev = obj->back;
		obj->node[node].next = -1;
		obj->back = node;
	}

	memcpy(&((char*)obj->data)[node * obj->element_size], data, obj->element_size);

	++obj->size;
}


void vector_erase(vector* obj, int position)
{
	int i = obj->front;
	int n = 0;
	int prev, next;

	if(position >= obj->size)
		return;

	while(i > -1)
	{
		if(n == position)
			break;
		++n;
		i = obj->node[i].next;
	}

	prev = obj->node[i].prev;
	next = obj->node[i].next;

	if(prev == -1)
	{
		obj->front = obj->node[i].next;
	}else
	{
		obj->node[prev].next = next;	
	}

	if(next == -1)
	{
		obj->back = obj->node[i].prev;
	}else
	{
		obj->node[next].prev = prev;		
	}

	obj->node[i].prev = -1;
	obj->node[i].next = obj->free;
	obj->free = i;
}

void vector_iterate(vector* obj, PFNVECTORITERATOR iterator)
{
	int i = obj->front;
	int n = 0;
	while(i > -1)
	{
		iterator(n, &((char*)obj->data)[i * obj->element_size]);
		++n;
		i = obj->node[i].next;
	}
}

