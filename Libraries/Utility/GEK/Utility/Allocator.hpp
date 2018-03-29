/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
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

        AlignedAllocator(AlignedAllocator const &)
        {
        }

        template <typename NEWTYPE> AlignedAllocator(AlignedAllocator<NEWTYPE, ALIGNMENT> const &)
        {
        }

        ~AlignedAllocator()
        {
        }

        TYPE *address(TYPE &value) const
        {
            return &value;
        }

        const TYPE *address(TYPE const &value) const
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

        bool operator != (AlignedAllocator const &other) const
        {
            return !((*this) == other);
        }

        void construct(TYPE * const value, TYPE const &data) const
        {
            void * const nebulous = static_cast<void *>(value);
            new (nebulous) TYPE(data);
        }

        void destroy(TYPE *const value) const
        {
            value->~TYPE();
        }

        bool operator == (AlignedAllocator const &other) const
        {
            return true;
        }

        TYPE *allocate(std::size_t const size) const
        {
            if (size == 0)
            {
                return nullptr;
            }

            if (size > max_size())
            {
                throw std::length_error("AlignedAllocator<TYPE>::allocate() - integer overflow.");
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
        TYPE *allocate(std::size_t const size, NEWTYPE const *) const
        {
            return allocate(size);
        }

    private:
        AlignedAllocator &operator=(AlignedAllocator const &);
    };
}; // namespace Gek
