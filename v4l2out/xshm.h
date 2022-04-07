#ifndef __XSHM_H__
#define __XSHM_H__

void* xshm_init(void);
void xshm_frame_copy(void* context, char* dest, int size);

#endif /* __XSHM_H__ */