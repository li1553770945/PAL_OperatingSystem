#include "stdio.h"

/*
 * for execve
 */

int main(int argc, char *argv[]){
    printf("  I am test_echo.\nexecve success.\n");
    printf("  argc:%d\n",argc);
    int i;
	for (i=0;i<argc;++i)
		printf("  argv%d%s",i,argv[i]);
    TEST_END(__func__);
    return 0;
}
