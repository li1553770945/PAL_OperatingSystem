#include <Syscalls.hpp>
#include <EasyFunctions.hpp>

int main()
{
	for (int i=1;i<=100;Loop(3e7),++i)
		cout<<"Count1-100: "<<i<<endl;
	return 0;
}
