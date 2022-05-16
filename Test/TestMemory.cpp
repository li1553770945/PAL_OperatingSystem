#include <Memory/PhysicalMemory.hpp>
#include <Test.hpp>
#include <Library/Kout.hpp>
void TestPhysicalMemory()
{
    using namespace POS;
    kout<<"TestPhysicalMemory()"<<endl;
    int * p = (int*)POS_PMM.Alloc(256*sizeof(int));
    if(p)
    {
         kout <<"alloc success"<<endl;
        *p = 1;
        p[128] = 2;
        kout<<"p:"<<p;
    }
    else
    {
        kout<<"alloc failed"<<endl;
    }
   

      int * p1 = (int*)POS_PMM.Alloc(256*sizeof(int));
    if(p1)
    {
         kout <<"alloc success"<<endl;
        *p1 = 1;
        p1[128] = 2;
        kout<<"p1:"<<p1;
    }
    else
    {
        kout<<"alloc failed"<<endl;
    }
    



}