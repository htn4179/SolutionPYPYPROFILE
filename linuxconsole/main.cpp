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
	/*int 80(ϵͳ����) int 3���ϵ㣩*/
	unsigned char code[] = { 0xcd,0x80,0xcc,0x00,0,0,0,0 }; //�˸��ֽڣ�����long �͵ĳ���
	char backup[8]; //���ݶ�ȡ��ָ��
	ptrace(PTRACE_ATTACH, tracee, NULL, NULL);
	long inst;  //���ڱ���ָ��Ĵ�����ָ�����һ����Ҫִ�е�ָ����ڴ��ַ 

	wait(NULL);
	ptrace(PTRACE_GETREGS, tracee, NULL, &regs);
	inst = ptrace(PTRACE_PEEKTEXT, tracee, regs.rip, NULL);
	printf("tracee:RIP:0x%llx INST: 0x%lx\n", regs.rip, inst);
	//��ȡ�ӽ��̽�Ҫִ�е� 7 bytesָ�����
	getdata(tracee, regs.rip, backup, 7);
	//���öϵ�
	putdata(tracee, regs.rip, (char*)code, 7);
	//���ӽ��̼���ִ�в�ִ�С�int 3���ϵ�ָ��ֹͣ
	ptrace(PTRACE_CONT, tracee, NULL, NULL);

	wait(NULL);
	long rip = ptrace(PTRACE_PEEKUSER, tracee, 8 * RIP, NULL);//��ȡ�ӽ���ֹͣʱ��rip��ֵ
	long inst2 = ptrace(PTRACE_PEEKTEXT, tracee, rip, NULL);
	printf("tracee:RIP:0x%lx INST: 0x%lx\n", rip, inst2);


	printf("Press Enter to continue  tracee process\n");
	getchar();
	putdata(tracee, regs.rip, backup, 7); //���½����ݵ�ָ��д�ؼĴ���
	ptrace(PTRACE_SETREGS, tracee, NULL, &regs);//���û�ԭ���ļĴ���ֵ
	ptrace(PTRACE_CONT, tracee, NULL, NULL);
	ptrace(PTRACE_DETACH, tracee, NULL, NULL);
	return 0;


}