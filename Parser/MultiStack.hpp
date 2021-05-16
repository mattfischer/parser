#ifndef PARSER_MULTI_STACK_HPP
#define PARSER_MULTI_STACK_HPP

#include <vector>
#include <functional>
#include <memory>
#include <stdexcept>

namespace Parser
{
    template<typename T> class MultiStack
    {
    private:
        struct Segment;

    public:
        struct Path {
            std::vector<std::shared_ptr<Segment>> segments;
        };

        class PathIterator {
        public:
            PathIterator();
            PathIterator(std::shared_ptr<Path> path, size_t segment, size_t index);

            T &operator*();
            const T &operator*() const;

            T *operator->();
            const T*operator->() const;

            PathIterator &operator++();
            bool operator==(const PathIterator &other) const;
            bool operator!=(const PathIterator &other) const;

        private:           
            std::shared_ptr<Path> mPath;
            size_t mSegment;
            size_t mIndex;

            friend class MultiStack;
        };

        typedef PathIterator iterator;

        MultiStack();
        size_t size() const;

        void push_back(size_t stack, T data);
        const T &back(size_t stack) const;

        size_t add(iterator before, T data);
        void replace(size_t stack, iterator before, T data);
        std::vector<size_t> split(size_t stack, size_t splits);
        void erase(size_t stack);
        size_t join(const std::vector<size_t> &stacks);

        void enumerate(size_t stack, size_t size, std::vector<iterator> &firsts, iterator &last);

    private:
        struct Segment {
            std::vector<std::shared_ptr<Segment>> prev;
            std::vector<std::weak_ptr<Segment>> next;
            std::vector<T> data;
        };

        std::vector<std::shared_ptr<Path>> expandPath(std::shared_ptr<Path> path, size_t size, size_t currentSize);
   
        std::shared_ptr<Segment> splitSegment(std::shared_ptr<Segment> segment, size_t size);
        void mergeSegment(std::shared_ptr<Segment> segment);
        void unlinkSegment(std::shared_ptr<Segment> segment);

        std::vector<std::shared_ptr<Segment>> mStacks;
    };

    template<typename T> MultiStack<T>::PathIterator::PathIterator()
    {
        mSegment = 0;
        mIndex = 0;
    }

    template<typename T> T &MultiStack<T>::PathIterator::operator*()
    {
        return mPath->segments[mSegment]->data[mIndex];
    }

    template<typename T> const T &MultiStack<T>::PathIterator::operator*() const
    {
        return mPath->segments[mSegment]->data[mIndex];
    }

    template<typename T> T *MultiStack<T>::PathIterator::operator->()
    {
        return &mPath->segments[mSegment]->data[mIndex];
    }

    template<typename T> const T *MultiStack<T>::PathIterator::operator->() const
    {
        return &mPath->segments[mSegment]->data[mIndex];
    }

    template<typename T> typename MultiStack<T>::PathIterator &MultiStack<T>::PathIterator::operator++()
    {
        if(mIndex < mPath->segments[mSegment]->data.size() - 1) {
            mIndex++;
        } else if(mSegment < mPath->segments.size() - 1) {
            mSegment++;
            mIndex = 0;
        } else {
            mIndex = mPath->segments.back()->data.size();
        }
        return *this;
    }

    template<typename T> bool MultiStack<T>::PathIterator::operator==(const PathIterator &other) const
    {
        return (mPath->segments[mSegment] == other.mPath->segments[other.mSegment]) && (mIndex == other.mIndex);
    }

    template<typename T> bool MultiStack<T>::PathIterator::operator!=(const PathIterator &other) const
    {
        return !(*this == other);
    }

    template<typename T> MultiStack<T>::PathIterator::PathIterator(std::shared_ptr<Path> path, size_t segment, size_t index)
    : mPath(std::move(path)), mSegment(segment), mIndex(index)
    {
    }

    template<typename T> MultiStack<T>::MultiStack()
    {
        mStacks.push_back(std::make_shared<Segment>());
    }

    template<typename T> size_t MultiStack<T>::size() const
    {
        return mStacks.size();
    }

    template<typename T> void MultiStack<T>::push_back(size_t stack, T data)
    {
        if(stack >= mStacks.size()) {
            throw std::out_of_range("stack");
        }

        mStacks[stack]->data.push_back(std::move(data));
    }

    template<typename T> const T& MultiStack<T>::back(size_t stack) const
    {
        if(stack >= mStacks.size()) {
            throw std::out_of_range("stack");
        }

        if(mStacks[stack]->data.size() > 0) {
            return mStacks[stack]->data.back();
        } else if(mStacks[stack]->prev.size() == 1) {
            return mStacks[stack]->prev[0]->data.back();
        } else {
            throw std::out_of_range("size");
        }
    }

    template<typename T> size_t MultiStack<T>::add(iterator before, T data)
    {
        std::shared_ptr<Segment> newSegment = std::make_shared<Segment>();
        newSegment->data.push_back(data);

        std::shared_ptr<Segment> segment = before.mPath->segments[before.mSegment];
        if(before.mIndex != 0) {
            segment = splitSegment(segment, before.mIndex);
        }
    
        for(auto &prev : segment->prev) {
            newSegment->prev.push_back(prev);
            prev->next.push_back(newSegment);
        }
    
        size_t result = mStacks.size();
        mStacks.push_back(newSegment);
        return result;
    }

    template <typename T> void MultiStack<T>::replace(size_t stack, iterator before, T data)
    {
        std::shared_ptr<Segment> segment = before.mPath->segments[before.mSegment];
        if(segment == mStacks[stack]) {
            segment->data.erase(segment->data.begin() + before.mIndex, segment->data.end());
            segment->data.push_back(data);
        } else {
            std::shared_ptr<Segment> newSegment = std::make_shared<Segment>();
            newSegment->data.push_back(data);

            std::shared_ptr<Segment> segment = before.mPath->segments[before.mSegment];
            if(before.mIndex != 0) {
                segment = splitSegment(segment, before.mIndex);
            }
        
            for(auto &prev : segment->prev) {
                newSegment->prev.push_back(prev);
                prev->next.push_back(newSegment);
            }
        
            unlinkSegment(mStacks[stack]);
            mStacks[stack] = newSegment;
        }
    }

    template <typename T> void MultiStack<T>::erase(size_t stack)
    {
        if(stack >= mStacks.size()) {
            throw std::out_of_range("stack");
        }

        unlinkSegment(mStacks[stack]);
        mStacks.erase(mStacks.begin() + stack);
    }

    template <typename T> void MultiStack<T>::unlinkSegment(std::shared_ptr<Segment> segment)
    {
        for(auto &prev : segment->prev) {
            for(size_t i=0; i<prev->next.size(); i++) {
                if(std::shared_ptr<Segment>(prev->next[i]) == segment) {
                    prev->next.erase(prev->next.begin() + i);
                    break;
                }
            }
            if(prev->next.size() == 1) {
                mergeSegment(prev);
            }
        }
    }

    template <typename T> size_t MultiStack<T>::join(const std::vector<size_t> &stacks)
    {
        std::vector<std::shared_ptr<Segment>> segments;
        for(size_t stack : stacks) {
            if(stack >= mStacks.size()) {
                throw std::out_of_range("stack");
            }

            segments.push_back(mStacks[stack]);
        }

        auto pred = [&](std::shared_ptr<Segment> segment) {
            for(const auto &s : segments) {
                if(s == segment) {
                    return true;
                }
            }
            return false;
        };
        mStacks.erase(std::remove_if(mStacks.begin(), mStacks.end(), pred), mStacks.end());
        
        mStacks.push_back(std::make_shared<Segment>());
        for(auto &segment : segments) {
            mStacks.back()->prev.push_back(segment);
            segment->next.push_back(mStacks.back());
        }

        return mStacks.size() - 1;
    }

    template<typename T> void MultiStack<T>::enumerate(size_t stack, size_t size, std::vector<iterator> &firsts, iterator &last)
    {
        std::shared_ptr<Path> startPath = std::make_shared<Path>();
        startPath->segments.push_back(mStacks[stack]);
        std::vector<std::shared_ptr<Path>> paths = expandPath(startPath, size, startPath->segments[0]->data.size());
        
        for(auto &path : paths) {
            size_t segmentSize = size;
            for(size_t i=0; i<path->segments.size() - 1; i++) {
                segmentSize -= path->segments[i]->data.size();
            }
            size_t index = path->segments.back()->data.size() - segmentSize;
            std::reverse(path->segments.begin(), path->segments.end());
            firsts.push_back(PathIterator(path, 0, index));
        }
        last = PathIterator(startPath, 0, startPath->segments[0]->data.size());
    }

    template<typename T> std::vector<std::shared_ptr<typename MultiStack<T>::Path>> MultiStack<T>::expandPath(std::shared_ptr<Path> path, size_t size, size_t currentSize)
    {
        std::vector<std::shared_ptr<Path>> results;
        if(currentSize >= size) {
            results.push_back(path);
        } else {
            for(auto &prev : path->segments.back()->prev) {
                std::shared_ptr<Path> newPath = std::make_shared<Path>();
                newPath->segments.insert(newPath->segments.end(), path->segments.begin(), path->segments.end());
                newPath->segments.push_back(prev);
                std::vector<std::shared_ptr<Path>> newPaths = expandPath(newPath, size, currentSize + prev->data.size());
                results.insert(results.end(), newPaths.begin(), newPaths.end());
            }
        }
        return results;
    }

    template<typename T> std::shared_ptr<typename MultiStack<T>::Segment> MultiStack<T>::splitSegment(std::shared_ptr<Segment> segment, size_t size)
    {
        if(size == segment->data.size()) {
            return std::shared_ptr<Segment>();
        }

        std::shared_ptr<Segment> back = std::make_shared<Segment>();
        back->data.insert(back->data.end(), segment->data.begin() + size, segment->data.end());
        segment->data.erase(segment->data.begin() + size, segment->data.end());

        back->prev.push_back(segment);
        back->next.insert(back->next.end(), segment->next.begin(), segment->next.end());
        for(auto &n : back->next) {
            std::shared_ptr<Segment> next(n);
            for(unsigned int i=0; i<next->prev.size(); i++) {
                if(next->prev[i] == segment) {
                    next->prev[i] = back;
                    break;
                }
            }
        }
        segment->next.clear();
        segment->next.push_back(back);

        for(unsigned int i=0; i<mStacks.size(); i++) {
            if(mStacks[i] == segment) {
                mStacks[i] = back;
                break;
            }
        }

        return back;
    }

    template<typename T> void MultiStack<T>::mergeSegment(std::shared_ptr<Segment> segment)
    {
        if(segment->next.size() != 1) {
            return;
        }

        std::shared_ptr<Segment> next(segment->next[0]);
        segment->data.insert(segment->data.end(), next->data.begin(), next->data.end());
        
        segment->next.clear();
        segment->next.insert(segment->next.end(), next->next.begin(), next->next.end());
        for(auto &nn : segment->next) {
            std::shared_ptr<Segment> n(nn);
            for(size_t i=0; i<n->prev.size(); i++) {
                if(n->prev[i] == next) {
                    n->prev[i] = segment;
                    break;
                }
            }
        }

        for(size_t i=0; i<mStacks.size(); i++) {
            if(mStacks[i] == next) {
                mStacks[i] = segment;
            }
        }
    }
}
#endif