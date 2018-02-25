//
// Copyright 2006 and onwards, Lukasz Lew
//

#ifndef FAST_STACK_H_
#define FAST_STACK_H_

#include <vector>

#include "fast_random.hpp"
#include "utils.hpp"

template <typename Elt, uint max_size>
class FastStack {
public:
    FastStack() : size(0) {}

    void Clear() { size = 0; }

    bool IsEmpty() const { return size == 0; }

    bool IsFull() const { return size == max_size; }

    uint Size() const { return size; }

    uint Capacity() const { return max_size - size; }

    Elt* Data() { return tab; }

    const Elt* Data() const { return tab; }

    vector<Elt> AsVector() {
        vector<Elt> ret(size);
        rep(ii, size) ret[ii] = tab[ii];
        return ret;
    }

    void Push(const Elt& elt) {
        tab[size++] = elt;
        Check();
    }

    Elt& Top() {
        ASSERT(size > 0);
        return tab[size - 1];
    }

    Elt& NewTop() {
        size += 1;
        Check();
        return Top();
    }

    void Pop() {
        size--;
        Check();
    }

    Elt Remove(uint idx) {
        Elt elt = tab[idx];
        size--;
        tab[idx] = tab[size];
        return elt;
    }

    Elt& PopTop() {
        size--;
        Check();
        return tab[size];
    }

    Elt& operator[](uint i) {
        ASSERT(i < size);
        return tab[i];
    }

    const Elt& operator[](uint i) const {
        ASSERT(i < size);
        return tab[i];
    }

private:
    void Check() const { ASSERT(size <= max_size); }

private:
    Elt tab[max_size];
    uint size;
};

// TODO add iterators

#endif
