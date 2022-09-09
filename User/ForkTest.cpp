#include <Syscalls.hpp>
#include <EasyFunctions.hpp>

int main()
{
	cout<<"ForkTest: Current PID "<<Sys_GetPID()<<endl
		<<"ForkTest: Perform fork..."<<endl;
	PID pid=SYS_clone();
	cout<<"ForkTest: Fork OK, this is "<<(pid!=0?"parent process":"child process")<<", returned PID "<<pid<<endl
		<<"ForkTest: Current PID "<<Sys_GetPID()<<endl;
	return 0;
}
