#pragma once

namespace Gek
{
    template <typename TYPE, std::size_t ALIGNMENT = sizeof(TYPE)>
    class AlignedAllocator
    {
    public:
        using pointer = TYPE *;
        using const_pointer = const TYPE *;
        using reference = TYPE &;
        using const_reference = const TYPE;
        using value_type = TYPE;
        using size_type = std::size_t;
        using difference_type = ptrdiff_t;

        AlignedAllocator()
        {
        }

        AlignedAllocator(const AlignedAllocator &)
        {
        }

        template <typename NEWTYPE> AlignedAllocator(const AlignedAllocator<NEWTYPE, ALIGNMENT> &)
        {
        }

        ~AlignedAllocator()
        {
        }

        TYPE *address(TYPE &value) const
        {
            return &value;
        }

        const TYPE *address(const TYPE &value) const
        {
            return &value;
        }

        std::size_t max_size() const
        {
            return (static_cast<std::size_t>(0) - static_cast<std::size_t>(1)) / sizeof(TYPE);
        }

        template <typename NEWTYPE>
        struct rebind
        {
            using other = AlignedAllocator<NEWTYPE, ALIGNMENT>;
        };

        bool operator != (const AlignedAllocator &other) const
        {
            return !(*this == other);
        }

        void construct(TYPE *const value, const TYPE &data) const
        {
            void * const nebulous = static_cast<void *>(value);
            new (nebulous) TYPE(data);
        }

        void destroy(TYPE *const value) const
        {
            value->~TYPE();
        }

        bool operator == (const AlignedAllocator &other) const
        {
            return true;
        }

        TYPE *allocate(const std::size_t size) const
        {
            if (size == 0)
            {
                return nullptr;
            }

            if (size > max_size())
            {
                throw std::length_error("AlignedAllocator<TYPE>::allocate() - Integer overflow.");
            }

            void *const nebulous = _mm_malloc(size * sizeof(TYPE), ALIGNMENT);
            if (nebulous == nullptr)
            {
                throw std::bad_alloc();
            }

            return static_cast<TYPE *>(nebulous);
        }

        void deallocate(TYPE *const value, const std::size_t size) const
        {
            _mm_free(value);
        }

        template <typename NEWTYPE>
        TYPE *allocate(const std::size_t size, const NEWTYPE *) const
        {
            return allocate(size);
        }

    private:
        AlignedAllocator &operator=(const AlignedAllocator &);
    };
}; // namespace Gek
