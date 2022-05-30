#ifndef POS_SPINLOCK_HPP
#define POS_SPINLOCK_HPP

class SpinLock
{
	protected:
		volatile bool lock=0;
		
	public:
		inline bool TryLock()
		{
			bool old=__sync_lock_test_and_set(&lock,1);
			__sync_synchronize();
			return !old;
		}
		
		inline void Unlock()
		{
			__sync_synchronize();
			__sync_lock_release(&lock);
		}
		
		inline void Lock()
		{
			while(__sync_lock_test_and_set(&lock,1)!=0);
			__sync_synchronize();
		}
		
		inline void Init()//Call this is not created(Specified from memory address).
		{lock=0;}
};

#endif
