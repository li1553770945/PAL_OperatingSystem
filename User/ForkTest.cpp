#include <Syscalls.hpp>
#include <EasyFunctions.hpp>

int main()
{
	cout<<"ForkTest: Current PID "<<Sys_GetPID()<<endl
		<<"ForkTest: Perform fork..."<<endl;
	PID pid=Sys_Fork();
	cout<<"ForkTest: Fork OK, this is "<<(pid!=0?"parent process":"child process")<<", returned PID "<<pid<<endl
		<<"ForkTest: Current PID "<<Sys_GetPID()<<endl;
	
	Loop(1e8);
	
	cout<<"ForkTest: Current PID "<<Sys_GetPID()<<endl
		<<"ForkTest: Perform fork again..."<<endl;
	pid=Sys_Fork();
	cout<<"ForkTest: Fork OK, this is "<<(pid!=0?"parent process":"child process")<<", returned PID "<<pid<<endl
		<<"ForkTest: Current PID "<<Sys_GetPID()<<endl;
	return 0;
}
