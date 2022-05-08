#include <Memory/PhysicalMemory.hpp>
#include <Test.hpp>
#include <Library/Kout.hpp>
void TestPhysicalMemory()
{
    using namespace POS;
    kout<<"TestPhysicalMemory()"<<endl;
    int * p = (int*)POS_PMM.AllocPage(1)->Addr();
    kout <<"alloc success"<<endl;
    *p = 1;
    p[128] = 2;
    kout<<p<<" "<<DataWithSize(p,4096);
}