#ifndef __UNICODE_H__
#define __UNICODE_H__

struct ustring;

struct ustring* ustring_new_from_utf8(const char* utf8_string);
void ustring_destroy(struct ustring* ustring);
int ustring_len(const struct ustring* ustring);
int ustring_get_code_at(const struct ustring* ustring, const int position);

#endif /* __UNICODE_H__ */