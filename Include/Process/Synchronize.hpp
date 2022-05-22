#ifndef POS_SYNCHRONIZE_HPP
#define POS_SYNCHRONIZE_HPP

class SpinLock
{
	protected:
		volatile bool lock;
		
	public:
		inline bool TryLock()
		{
			
			return 0;
		}
		
		inline void Unlock()
		{
			
		}
		
		inline void Lock()
		{
			
		}
		
		inline void Init()
		{lock=0;}
		
//	typedef volatile bool lock_t;
//
//	static inline void
//	lock_init(lock_t *lock) {
//	    *lock = 0;
//	}
//	
//	static inline bool
//	try_lock(lock_t *lock) {
//	    return !test_and_set_bit(0, lock);
//	}
//	
//	static inline void
//	lock(lock_t *lock) {
//	    while (!try_lock(lock)) {
//	        schedule();
//	    }
//	}
//	
//	static inline void
//	unlock(lock_t *lock) {
//	    if (!test_and_clear_bit(0, lock)) {
//	        panic("Unlock failed.\n");
//	    }
//	}
};

class Mutex
{
	protected:
		
		
	public:
		inline void Lock()
		{
			
		}
		
		inline void Unlock()
		{
			
		}
		
		inline bool TryLock()
		{
			
			return 0;
		}
		
		void Init()
		{
			
		}
};

#endif
