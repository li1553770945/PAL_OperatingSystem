#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"

struct tms              
{                     
	long tms_utime;  
	long tms_stime;  
	long tms_cutime; 
	long tms_cstime; 
};

struct tms mytimes;

void test_times() {
	TEST_START(__func__);

	int test_ret = times(&mytimes);
	assert(test_ret >= 0);

	printf("mytimes success\n{tms_utime:%d, tms_stime:%d, tms_cutime:%d, tms_cstime:%d}\n",
		mytimes.tms_utime, mytimes.tms_stime, mytimes.tms_cutime, mytimes.tms_cstime);
	TEST_END(__func__);
}

int main(void) {
	test_times();
	return 0;
}
