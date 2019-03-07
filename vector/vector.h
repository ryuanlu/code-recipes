#ifndef __VECTOR_H__
#define __VECTOR_H__

typedef struct vector vector;
typedef void (*PFNVECTORITERATOR)(int index, void* data);

vector* vector_new(int element_size, int capacity);
void vector_delete(vector* obj);
void vector_pushback(vector* obj, void* data);
void vector_erase(vector* obj, int position);
void vector_iterate(vector* obj, PFNVECTORITERATOR iterator);

#endif /* __VECTOR_H__ */