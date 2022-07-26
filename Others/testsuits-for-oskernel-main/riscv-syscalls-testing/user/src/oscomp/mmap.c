#include "unistd.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

/*
 * 测试成功时输出：
 * "  Hello, mmap success"
 * 测试失败时输出：
 * "mmap error."
 */
static struct kstat kst;
void test_mmap(void){
    TEST_START(__func__);
    char *array;
    const char *str = "  Hello, mmap successfully!";
    int fd;

    fd = open("test_mmap.txt", O_RDWR | O_CREATE);
    write(fd, str, strlen(str));
    fstat(fd, &kst);
    printf("file len: %d\n", kst.st_size);
    array = mmap(NULL, kst.st_size, PROT_WRITE | PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
    //printf("return array: %x\n", array);

    if (array == MAP_FAILED) {
	printf("mmap error.\n");
    }else{
	printf("mmap content: %s\n", array);
	//printf("%s\n", str);

	munmap(array, kst.st_size);
    }

    close(fd);

    TEST_END(__func__);
}

int main(void){
    test_mmap();
    return 0;
}
