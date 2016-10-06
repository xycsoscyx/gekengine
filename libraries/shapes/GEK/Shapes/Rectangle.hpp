#pragma once

namespace Gek
{
    namespace Shapes
    {
        template <typename TYPE>
        struct Rectangle
        {
            TYPE left;
            TYPE top;
            TYPE right;
            TYPE bottom;

            Rectangle(TYPE left = 0, TYPE top = 0, TYPE right = 0, TYPE bottom = 0)
                : left(left)
                , top(top)
                , right(right)
                , bottom(bottom)
            {
            }
        };
    }; // namespace Shapes
}; // namespace Gek
