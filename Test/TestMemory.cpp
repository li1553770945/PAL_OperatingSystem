#include <Memory/PhysicalMemory.hpp>
#include <Test.hpp>
#include <Library/Kout.hpp>
void TestPhysicalMemory()
{
    using namespace POS;
    kout<<"TestPhysicalMemory()"<<endl;
    int * p = (int*)kmalloc(sizeof(int)*2000);
    kout <<"alloc success"<<endl;
    *p = 1;
    p[128] = 2;
    kout<<"p:"<<p;

     int * p2 = (int*)kmalloc(sizeof(int)*2000);
    kout <<"alloc success"<<endl;
    kout<<"p2:"<<p2;

    kfree(p);
    kfree(p2);

    p = (int*)kmalloc(sizeof(int)*4000);
    kout <<"alloc success"<<endl;
    *p = 1;
    p[100] = 2;
    kout<<"p:"<<p;
    kfree(p);
}