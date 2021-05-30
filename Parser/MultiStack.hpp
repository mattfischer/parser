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
        class PathIterator;
        class Locator {
        public:
            Locator(const PathIterator &iterator);
            Locator(std::shared_ptr<Segment> segment, size_t index);

            T &operator*();
            const T &operator*() const;

        private:
            std::shared_ptr<Segment> mSegment;
            size_t mIndex;

            friend class MultiStack;
        };

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
            bool operator==(const Locator &other) const;
            bool operator!=(const PathIterator &other) const;
            bool operator!=(const Locator &other) const;

        private:           
            std::shared_ptr<Path> mPath;
            size_t mSegment;
            size_t mIndex;

            friend class Locator;
            friend class MultiStack;
        };

        typedef PathIterator iterator;

        MultiStack();
        size_t size() const;

        void push_back(size_t stack, T data);
        void pop_back(size_t stack);
        const T &back(size_t stack) const;
        Locator end(size_t stack);

        size_t add(Locator &before);
        size_t add(iterator &before) { return add(Locator(before)); }

        void relocate(size_t stack, Locator &before);
        void relocate(size_t stack, iterator &before) { return relocate(stack, Locator(before)); }

        void erase(size_t stack);

        void join(size_t stack, Locator &before);
        void join(size_t stack, iterator &before) { return join(stack, Locator(before)); }

        std::vector<iterator> backtrack(Locator &end, size_t size);

    private:
        struct Segment {
            std::vector<std::shared_ptr<Segment>> prev;
            std::vector<std::shared_ptr<Segment>> next;
            std::vector<T> data;
        };

        std::vector<std::shared_ptr<Path>> expandPath(std::shared_ptr<Path> path, size_t size, size_t currentSize);
   
        void splitSegment(Locator &before);
        void mergeSegment(std::shared_ptr<Segment> segment);
        void unlinkSegment(std::shared_ptr<Segment> segment);

        std::vector<std::shared_ptr<Segment>> mStacks;
    };

    template<typename T> MultiStack<T>::Locator::Locator(const PathIterator &iterator)
    {
        mSegment = iterator.mPath->segments[iterator.mSegment];
        mIndex = iterator.mIndex;
    }

    template<typename T> MultiStack<T>::Locator::Locator(std::shared_ptr<Segment> segment, size_t index)
    {
        mSegment = segment;
        mIndex = index;
    }

    template<typename T> T &MultiStack<T>::Locator::operator*()
    {
        return mSegment->data[mIndex];
    }

    template<typename T> const T &MultiStack<T>::Locator::operator*() const
    {
        return mSegment->data[mIndex];
    }
    
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

    template<typename T> bool MultiStack<T>::PathIterator::operator==(const Locator &other) const
    {
        return (mPath->segments[mSegment] == other.mSegment) && (mIndex == other.mIndex);
    }

    template<typename T> bool MultiStack<T>::PathIterator::operator!=(const PathIterator &other) const
    {
        return !(*this == other);
    }

    template<typename T> bool MultiStack<T>::PathIterator::operator!=(const Locator &other) const
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

    template<typename T> void MultiStack<T>::pop_back(size_t stack)
    {
        mStacks[stack]->data.pop_back();
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

    template<typename T> typename MultiStack<T>::Locator MultiStack<T>::end(size_t stack)
    {
        return Locator(mStacks[stack], mStacks[stack]->data.size());
    }

    template<typename T> size_t MultiStack<T>::add(Locator &before)
    {
        std::shared_ptr<Segment> newSegment = std::make_shared<Segment>();

        splitSegment(before);
        std::shared_ptr<Segment> segment = before.mSegment;
        
        for(auto &prev : segment->prev) {
            newSegment->prev.push_back(prev);
            prev->next.push_back(newSegment);
        }
    
        size_t result = mStacks.size();
        mStacks.push_back(newSegment);
        return result;
    }

    template <typename T> void MultiStack<T>::relocate(size_t stack, Locator &before)
    {
        std::shared_ptr<Segment> segment = before.mSegment;
        if(segment == mStacks[stack]) {
            segment->data.erase(segment->data.begin() + before.mIndex, segment->data.end());
        } else {
            std::shared_ptr<Segment> newSegment = std::make_shared<Segment>();

            splitSegment(before);
            std::shared_ptr<Segment> segment = before.mSegment;
            
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

    template <typename T> void MultiStack<T>::join(size_t stack, Locator &before)
    {
        std::shared_ptr<Segment> segment = mStacks[stack];
        
        splitSegment(before);
        std::shared_ptr<Segment> targetSegment = before.mSegment;
        segment->next.push_back(targetSegment);
        targetSegment->prev.push_back(segment);
        mStacks.erase(mStacks.begin() + stack);
    }

    template<typename T> std::vector<typename MultiStack<T>::iterator> MultiStack<T>::backtrack(Locator &end, size_t size)
    {
        std::shared_ptr<Path> startPath = std::make_shared<Path>();
        startPath->segments.push_back(end.mSegment);
        std::vector<std::shared_ptr<Path>> paths = expandPath(startPath, size, end.mIndex);
        std::vector<iterator> results;
        for(auto &path : paths) {
            size_t segmentSize = size;
            for(size_t i=1; i<path->segments.size(); i++) {
                segmentSize -= path->segments[i]->data.size();
            }
            size_t index = path->segments.front()->data.size() - segmentSize;
            results.push_back(PathIterator(path, 0, index));
        }
        return results;
    }
    
    template<typename T> std::vector<std::shared_ptr<typename MultiStack<T>::Path>> MultiStack<T>::expandPath(std::shared_ptr<Path> path, size_t size, size_t currentSize)
    {
        std::vector<std::shared_ptr<Path>> results;
        if(currentSize >= size) {
            results.push_back(path);
        } else {
            for(auto &prev : path->segments.front()->prev) {
                std::shared_ptr<Path> newPath = std::make_shared<Path>();
                newPath->segments.push_back(prev);
                newPath->segments.insert(newPath->segments.end(), path->segments.begin(), path->segments.end());
                std::vector<std::shared_ptr<Path>> newPaths = expandPath(newPath, size, currentSize + prev->data.size());
                results.insert(results.end(), newPaths.begin(), newPaths.end());
            }
        }
        return results;
    }

    template<typename T> void MultiStack<T>::splitSegment(Locator &before)
    {
        if(before.mIndex == 0) {
            return;
        }

        std::shared_ptr<Segment> segment = before.mSegment;
        std::shared_ptr<Segment> back = std::make_shared<Segment>();
        back->data.insert(back->data.end(), segment->data.begin() + before.mIndex, segment->data.end());
        segment->data.erase(segment->data.begin() + before.mIndex, segment->data.end());

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

        before.mSegment = back;
        before.mIndex = 0;
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
        for(auto &n : segment->next) {
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
            } else if(prev->next.size() == 0) {
                unlinkSegment(prev);
            }
        }
    }
}
#endif