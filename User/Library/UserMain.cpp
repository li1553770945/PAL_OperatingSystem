#include "Syscalls.hpp"

int main();

extern "C" void _UserMain()
{
	int re=main();
	Sys_Exit(re);
}
