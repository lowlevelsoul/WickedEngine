#pragma once
#include <atomic>
#include <thread>

#if !PLATFORM_APPLE
#	include <emmintrin.h> // _mm_pause()
#endif

namespace wi
{
	class SpinLock
	{
	private:
		std::atomic_flag lck = ATOMIC_FLAG_INIT;
	public:
		inline void lock()
		{
			int spin = 0;
			while (!try_lock())
			{
				if (spin < 10)
				{
#if PLATFORM_APPLE
					// (JS) Could not find the intrinsic for this.
					asm volatile("yield");
#else
					_mm_pause(); // SMT thread swap can occur here
#endif
				}
				else
				{
					std::this_thread::yield(); // OS thread swap can occur here. It is important to keep it as fallback, to avoid any chance of lockup by busy wait
				}
				spin++;
			}
		}
		inline bool try_lock()
		{
			return !lck.test_and_set(std::memory_order_acquire);
		}

		inline void unlock()
		{
			lck.clear(std::memory_order_release);
		}
	};
}
