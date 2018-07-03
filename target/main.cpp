#include <cstdio>
#include<stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
	int i = 0;
	while (1) {
		printf("Hello,ptrace! [pid:%d]! num is %d\n", getpid(), i++);
		sleep(2);
	}
	return 0;
}