#ifndef __KMS_H__
#define __KMS_H__

void* kms_init(void);
void kms_frame_copy(void* context, char* dest, int size);

#endif /* __KMS_H__ */