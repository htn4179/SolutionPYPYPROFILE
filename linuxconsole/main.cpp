#include<sys/ptrace.h>
#include<sys/reg.h>
#include<sys/wait.h>
#include<sys/user.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<stdio.h>

#define long_size sizeof(long)


void getdata(pid_t child, long addr, char * str, int len) {
	char * laddr = str;
	int i, j;
	union u {
		long  val;
		char   chars[long_size];
	} data;
	i = 0;
	j = len / long_size;

	while (i<j) {
		data.val = ptrace(PTRACE_PEEKDATA, child, addr + long_size*i, NULL);
		if (data.val == -1) {
			if (errno) {
				printf("READ error: %s\n", strerror(errno));
			}
		}
		memcpy(laddr, data.chars, long_size);
		++i;
		laddr = laddr + long_size;
	}

	j = len %long_size;
	if (j != 0) {
		data.val = ptrace(PTRACE_PEEKDATA, child, addr + long_size*i, NULL);
		if (data.val == -1) {
			if (errno) {
				printf("READ error: %s\n", strerror(errno));
			}
		}
		memcpy(laddr, data.chars, j);
	}
	str[len] = '\0';
}

void putdata(pid_t child, long addr, char * str, int len) {
	char * laddr = str;
	int i, j;
	j = len / long_size;
	i = 0;
	union u {
		long val;
		char chars[long_size];
	} data;
	while (i<j) {
		memcpy(data.chars, laddr, long_size);
		ptrace(PTRACE_POKEDATA, child, addr + long_size*i, data.val);
		++i;
		laddr = laddr + long_size;
	}
	j = len%long_size;
	if (j != 0) {
		data.val = ptrace(PTRACE_PEEKDATA, child, addr + long_size*i, NULL);
		if (data.val == -1) {
			if (errno) {
				printf("READ error: %s\n", strerror(errno));
			}
		}

		memcpy(data.chars, laddr, j);
		ptrace(PTRACE_POKEDATA, child, addr + long_size*i, data.val);
	}
}

int main(int argc, char * argv[]) {
	if (argc != 2) {
		printf("Usage: %s pid\n", argv[0]);
	}
	pid_t tracee = atoi(argv[1]);
	struct user_regs_struct regs;
	/*int 80(系统调用) int 3（断点）*/
	unsigned char code[] = { 0xcd,0x80,0xcc,0x00,0,0,0,0 }; //八个字节，等于long 型的长度
	char backup[8]; //备份读取的指令
	ptrace(PTRACE_ATTACH, tracee, NULL, NULL);
	long inst;  //用于保存指令寄存器所指向的下一条将要执行的指令的内存地址 

	wait(NULL);
	ptrace(PTRACE_GETREGS, tracee, NULL, &regs);
	inst = ptrace(PTRACE_PEEKTEXT, tracee, regs.rip, NULL);
	printf("tracee:RIP:0x%llx INST: 0x%lx\n", regs.rip, inst);
	//读取子进程将要执行的 7 bytes指令并备份
	getdata(tracee, regs.rip, backup, 7);
	//设置断点
	putdata(tracee, regs.rip, (char*)code, 7);
	//让子进程继续执行并执行“int 3”断点指令停止
	ptrace(PTRACE_CONT, tracee, NULL, NULL);

	wait(NULL);
	long rip = ptrace(PTRACE_PEEKUSER, tracee, 8 * RIP, NULL);//获取子进程停止时，rip的值
	long inst2 = ptrace(PTRACE_PEEKTEXT, tracee, rip, NULL);
	printf("tracee:RIP:0x%lx INST: 0x%lx\n", rip, inst2);


	printf("Press Enter to continue  tracee process\n");
	getchar();
	putdata(tracee, regs.rip, backup, 7); //重新将备份的指令写回寄存器
	ptrace(PTRACE_SETREGS, tracee, NULL, &regs);//设置会原来的寄存器值
	ptrace(PTRACE_CONT, tracee, NULL, NULL);
	ptrace(PTRACE_DETACH, tracee, NULL, NULL);
	return 0;


}