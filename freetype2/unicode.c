#include <stdlib.h>

struct ustring
{
	int*	code;
	int	length;
};

static int utf8_strlen(const char* utf8_string)
{
	int i = 0;
	int length = 0;

	while(utf8_string[i] != 0)
	{
		if((utf8_string[i] & 0xc0) != 0x80)
			++length;
		++i;
	}

	return length;
}

struct ustring* ustring_new_from_utf8(const char* utf8_string)
{
	struct ustring* ustring = NULL;

	ustring = calloc(1, sizeof(struct ustring));

	ustring->length = utf8_strlen(utf8_string);
	ustring->code = calloc(1, ustring->length * sizeof(int));

	int read_pos = 0;
	int write_pos = 0;

	while(utf8_string[read_pos])
	{
		if(!(utf8_string[read_pos] & 0x80))
		{
			ustring->code[write_pos] = utf8_string[read_pos];
			++read_pos;
			++write_pos;
		}else if((utf8_string[read_pos] & 0xE0) == 0xC0)
		{
			ustring->code[write_pos] += (utf8_string[read_pos] & 0x1F) << 6;
			ustring->code[write_pos] += (utf8_string[read_pos + 1] & 0x3F);
			read_pos += 2;
			++write_pos;
		}else if((utf8_string[read_pos] & 0xF0) == 0xE0)
		{
			ustring->code[write_pos] += (utf8_string[read_pos] & 0x0F) << 12;
			ustring->code[write_pos] += (utf8_string[read_pos + 1] & 0x3F) << 6;
			ustring->code[write_pos] += (utf8_string[read_pos + 2] & 0x3F);
			read_pos += 3;
			++write_pos;
		}else if((utf8_string[read_pos] & 0xF8) == 0xF0)
		{
			ustring->code[write_pos] += (utf8_string[read_pos] & 0x07) << 18;
			ustring->code[write_pos] += (utf8_string[read_pos + 1] & 0x3F) << 12;
			ustring->code[write_pos] += (utf8_string[read_pos + 2] & 0x3F) << 6;
			ustring->code[write_pos] += (utf8_string[read_pos + 3] & 0x3F);
			read_pos += 4;
			++write_pos;
		}else
		{
			++read_pos;
		}
	}

	return ustring;
}

void ustring_destroy(struct ustring* ustring)
{
	free(ustring->code);
	free(ustring);
}

int ustring_len(const struct ustring* ustring)
{
	return ustring->length;
}

int ustring_get_code_at(const struct ustring* ustring, const int position)
{
	return (position >= 0 && position < ustring->length) ? ustring->code[position] : 0;
}

