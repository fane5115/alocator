#include <type_traits>
#include <iostream>
#include <memory>
#include <chrono>
#include <time.h>
#include <vector>
template<class T>
class ObjectPool 
{
public:
	const static std::size_t Size = 100000;

	using value_type = T;
	using pointer = value_type*;

	template <typename U> struct rebind {
		typedef ObjectPool<U> obj;
	};	

	ObjectPool()
	{
		for (auto i = 1; i < Size; ++i)
			mPool[i - 1].mNext = &mPool[i];

		mNextFree = &mPool[0];
	}

	ObjectPool(const ObjectPool&) = delete;

	ObjectPool(ObjectPool&& obj) noexcept
		: mPool{ move(obj.mPool) }
		, mNextFree{ obj.mNextFree }
	{
		obj.mNextFree = nullptr;
	}

	~ObjectPool() = default;

	pointer allocate()
	{
		if (mNextFree == nullptr)
			throw std::bad_alloc{};

		const auto item = mNextFree;
		mNextFree = item->mNext;

		return reinterpret_cast<pointer>(&item->mStorage);
	}

	void deallocate(pointer p) noexcept
	{
		const auto item = reinterpret_cast<Item*>(p);

		item->mNext = mNextFree;
		mNextFree = item;
	}

	template<class ...Args>
    pointer construct(Args&& ...args)
	{
		return new (allocate()) value_type(std::forward<Args>(args)...);
	}

	void destroy(pointer p) noexcept
	{
		if (p == nullptr)
			return;

		p->~value_type();
		deallocate(p);
	}

	ObjectPool& operator =(const ObjectPool&) = delete;

	ObjectPool& operator =(ObjectPool&& obj) noexcept
	{
		if (this == &obj)
			return *this;

		mPool = std::move(obj.mPool);
		mNextFree = obj.mNextFree;

		obj.mNextFree = nullptr;

		return *this;
	}

private:
	union Item
	{
		std::aligned_storage_t<sizeof(value_type), alignof(value_type)> mStorage;
		Item* mNext;
	};

	std::unique_ptr<Item[]> mPool = std::make_unique<Item[]>(Size);
	Item* mNextFree = nullptr;
};


int main()
{
	ObjectPool<int> pool;
	clock_t start;
	start = clock();

	for (auto i = 0; i < 100000; ++i)
		delete new int;
	std::cout << "Default Allocator Time: ";
	std::cout << (((double)clock() - start) / CLOCKS_PER_SEC) << "\n\n";
		
	start = clock();

	for (auto i = 0; i < 100000; ++i)
		pool.destroy(pool.construct());
	std::cout << "Personal Allocator Time: ";
	std::cout << (((double)clock() - start) / CLOCKS_PER_SEC) << "\n\n";

	return 0;
}