#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <wait.h>

int main(int argc, char** argv)
{
	int result = 0;
	int pid = 0;
	struct stat rootfs_stat;
	char* rootfs = argv[1];
	char* init = argv[2];

	if(argc < 2)
	{
		fprintf(stderr, "%s path_to_rootfs [program_to_run]\n", argv[0]);
		return EXIT_FAILURE;
	}

	if(argc == 2)
		init = "/bin/sh";

	result = stat(rootfs, &rootfs_stat);

	if(!S_ISDIR(rootfs_stat.st_mode))
	{
		fputs("Invalid path\n", stderr);
		return EXIT_FAILURE;
	}

	unshare(CLONE_NEWNS|CLONE_NEWUTS|CLONE_NEWNET|CLONE_NEWIPC|CLONE_NEWCGROUP);

	/* Make all mounts private first */
	mount("none", "/", NULL, MS_REC|MS_PRIVATE, NULL);

	result = mount(rootfs, rootfs, NULL, MS_BIND|MS_PRIVATE, NULL);

	fprintf(stderr, "mount == %d\n", result);

	system("ip l a eth0 type veth peer name veth0 netns /proc/1/ns/net");
	chdir(rootfs);
	syscall(SYS_pivot_root, ".", ".");
	umount2("/", MNT_DETACH);
	chdir("/");

	unshare(CLONE_NEWPID);

	pid = fork();

	if(pid > 0)
	{
		waitpid(pid, NULL, 0);
	}else if(pid == 0)
	{
		result = mount("proc", "/proc", "proc", 0, NULL);
		execl(init, init, NULL);
		perror("execl");
	}else
	{
		perror("fork");
	}

	return EXIT_SUCCESS;
}