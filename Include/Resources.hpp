#ifndef POS_RESOURCES_HPP
#define POS_RESOURCES_HPP

#define ResourceDeclare(name)						\
	extern "C"										\
	{												\
		extern char _binary_Build_##name##_size[];			\
		extern char _binary_Build_##name##_start[];		\
		extern char _binary_Build_##name##_end[];			\
	};

ResourceDeclare(Hello_img);
ResourceDeclare(Count1_100_img);
ResourceDeclare(ForkTest_img);

#define GetResourceBegin(name)	(_binary_Build_##name##_start)
#define GetResourceEnd(name)	(_binary_Build_##name##_end)
#define GetResourceSize(name)	(_binary_Build_##name##_size)

#endif
