#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#define ENTER_CA_MODE	"\x1b[?47h"
#define EXIT_CA_MODE	"\x1b[?47l"



struct tty_device
{
	int		fd;
	struct termios	original_terminos;
};



struct tty_device* tty_device_open(int fd)
{
	int r = 0;
	struct termios raw;
	struct tty_device* device = NULL;

	device = calloc(sizeof(struct tty_device), 1);
	device->fd = fd;

	r = tcgetattr(device->fd, &device->original_terminos);

	if(r < 0) goto failure;

	raw = device->original_terminos;
	raw.c_iflag &= ~(BRKINT|ICRNL|INPCK|ISTRIP|IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO|ICANON|IEXTEN|ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	r = tcsetattr(device->fd, TCSAFLUSH, &raw);
	if(r < 0) goto failure;

	return device;
failure:
	free(device);
	return NULL;
}


void tty_device_close(struct tty_device* tty_device)
{
	tcsetattr(tty_device->fd, TCSAFLUSH, &tty_device->original_terminos);
	free(tty_device);
}


int main(int argc, char** argv)
{
	struct tty_device* tty = NULL;

	tty = tty_device_open(0);
	write(0, "\x1b[?25l\x1b[?47h\x1b[2J\x1b[H", 19);
	sleep(5);
	tty_device_close(tty);
	write(0, "\x1b[?25h\x1b[?47l", 12);

	return EXIT_SUCCESS;
}