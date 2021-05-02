#ifndef UTIL_TABLE_HPP
#define UTIL_TABLE_HPP

#include <vector>

namespace Util {

    template<typename T> class Table
    {
    public:
        Table()
        : mWidth(0), mHeight(0)
        {
        }

        Table(size_t width, size_t height, T defaultValue = 0)
        : mWidth(width), mHeight(height), mData(width * height, defaultValue)
        {
        }

        void resize(size_t width, size_t height, T defaultValue = 0)
        {
            mWidth = width;
            mHeight = height;
            mData.resize(width * height, defaultValue);
        }

        const T &at(unsigned int x, unsigned int y) const
        {
            return mData[y*mWidth + x];
        }

        T &at(unsigned int x, unsigned int y)
        {
            return mData[y*mWidth + x];
        }
    
    private:
        size_t mWidth;
        size_t mHeight;
        std::vector<T> mData;
    };
}
#endif