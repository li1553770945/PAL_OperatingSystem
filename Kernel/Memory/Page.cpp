#include <Memory/Page.hpp>

Page * Page::DismantleNext()
{
    Page * next = this->next;
    if(next==nullptr)
    {
        //panic("DismantleNext Failed");
        return nullptr;
    }
    this->next = next->next;
    if(next->next)
        next->next->pre = this;

    return next;
}
Page * Page::Dismantle()
{
    this->pre->next = this->next;
    if(this->next)
    {
        this->next->pre = this->pre;
    }
    return this;
}
void Page::AddNext(Page * next)
{
    next->next = this->next;
    this->next = next;
    next->pre = this;
    if(next->next)
        next->next->pre = next;
}
