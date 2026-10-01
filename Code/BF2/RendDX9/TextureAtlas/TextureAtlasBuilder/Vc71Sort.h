// [CHOSEN, not recovered] local home for the VC7.1 sorting permutation used by atlas packing.
// Equal-key order affects rectangle placement and output bytes; preserve it when changing sorting.
// The by-size comparator is non-strict. Partition bounds and the insertion sentinel keep its
// equal-element case bounded. Copy/move traffic may differ while preserving the permutation.

#ifndef TEXTUREATLASBUILDER_VC71SORT_H
#define TEXTUREATLASBUILDER_VC71SORT_H

#include <algorithm>
#include <iterator>
#include <utility>

namespace textureatlas {
namespace vc71sort {

// _Med3 (0x100032A0): three compare-and-swaps, (mid, first), (last, mid), (mid, first).
template <class RanIt, class Pr>
void med3(RanIt first, RanIt mid, RanIt last, Pr pred)
{
    if (pred(*mid, *first))
        std::iter_swap(mid, first);
    if (pred(*last, *mid))
        std::iter_swap(last, mid);
    if (pred(*mid, *first))
        std::iter_swap(mid, first);
}

// _Median (0x10003B10): `last` is inclusive here (the caller passes _Last - 1). Above 40 elements,
// the median of three medians of three, with step = (last - first + 1) / 8.
template <class RanIt, class Pr>
void median(RanIt first, RanIt mid, RanIt last, Pr pred)
{
    if (40 < last - first)
    {
        const typename std::iterator_traits<RanIt>::difference_type step = (last - first + 1) / 8;
        med3(first, first + step, first + 2 * step, pred);
        med3(mid - step, mid, mid + step, pred);
        med3(last - 2 * step, last - step, last, pred);
        med3(first + step, mid, last - step, pred);
    }
    else
    {
        med3(first, mid, last, pred);
    }
}

// _Unguarded_partition (0x10004240): a three-way partition around the median; returns the range of
// elements equivalent to the pivot.
template <class RanIt, class Pr>
std::pair<RanIt, RanIt> unguardedPartition(RanIt first, RanIt last, Pr pred)
{
    RanIt mid = first + (last - first) / 2;
    median(first, mid, last - 1, pred);
    RanIt pfirst = mid;
    RanIt plast = pfirst + 1;

    while (first < pfirst && !pred(*(pfirst - 1), *pfirst) && !pred(*pfirst, *(pfirst - 1)))
        --pfirst;
    while (plast < last && !pred(*plast, *pfirst) && !pred(*pfirst, *plast))
        ++plast;

    RanIt gfirst = plast;
    RanIt glast = pfirst;

    for (;;)
    {
        for (; gfirst < last; ++gfirst)
        {
            if (pred(*pfirst, *gfirst))
                ;
            else if (pred(*gfirst, *pfirst))
                break;
            else
                std::iter_swap(plast++, gfirst);
        }
        for (; first < glast; --glast)
        {
            if (pred(*(glast - 1), *pfirst))
                ;
            else if (pred(*pfirst, *(glast - 1)))
                break;
            else
                std::iter_swap(--pfirst, glast - 1);
        }
        if (glast == first && gfirst == last)
            return std::pair<RanIt, RanIt>(pfirst, plast);

        if (glast == first)
        {   // no room at the bottom: rotate the pivot run upward
            if (plast != gfirst)
                std::iter_swap(pfirst, plast);
            ++plast;
            std::iter_swap(pfirst++, gfirst++);
        }
        else if (gfirst == last)
        {   // no room at the top: rotate the pivot run downward
            if (--glast != --pfirst)
                std::iter_swap(glast, pfirst);
            std::iter_swap(pfirst, --plast);
        }
        else
        {
            std::iter_swap(gfirst++, --glast);
        }
    }
}

// _Insertion_sort (0x10005990): a new earliest element is rotated to the front; otherwise the element
// is rotated down to just after the last position whose predecessor it is not "less" than.
template <class RanIt, class Pr>
void insertionSort(RanIt first, RanIt last, Pr pred)
{
    if (first != last)
    {
        for (RanIt next = first; ++next != last; )
        {
            if (pred(*next, *first))
            {
                RanIt next1 = next;
                std::rotate(first, next, ++next1);
            }
            else
            {
                RanIt dest = next;
                for (RanIt dest0 = dest; pred(*next, *--dest0); )
                    dest = dest0;
                if (dest != next)
                {
                    RanIt next1 = next;
                    std::rotate(dest, next, ++next1);
                }
            }
        }
    }
}

// _Push_heap (0x100032F0).
template <class RanIt, class Diff, class Ty, class Pr>
void pushHeap(RanIt first, Diff hole, Diff top, Ty val, Pr pred)
{
    for (Diff idx = (hole - 1) / 2; top < hole && pred(*(first + idx), val); idx = (hole - 1) / 2)
    {
        *(first + hole) = *(first + idx);
        hole = idx;
    }
    *(first + hole) = val;
}

// _Adjust_heap (0x10003BE0).
template <class RanIt, class Diff, class Ty, class Pr>
void adjustHeap(RanIt first, Diff hole, Diff bottom, Ty val, Pr pred)
{
    const Diff top = hole;
    Diff idx = 2 * hole + 2;

    for (; idx < bottom; idx = 2 * idx + 2)
    {
        if (pred(*(first + idx), *(first + (idx - 1))))
            --idx;
        *(first + hole) = *(first + idx);
        hole = idx;
    }

    if (idx == bottom)
    {
        *(first + hole) = *(first + (bottom - 1));
        hole = bottom - 1;
    }
    pushHeap(first, hole, top, val, pred);
}

// make_heap -> _Make_heap (0x10004DB0).
template <class RanIt, class Pr>
void makeHeap(RanIt first, RanIt last, Pr pred)
{
    typedef typename std::iterator_traits<RanIt>::difference_type Diff;
    typedef typename std::iterator_traits<RanIt>::value_type Ty;
    const Diff bottom = last - first;
    for (Diff hole = bottom / 2; 0 < hole; )
    {
        --hole;
        adjustHeap(first, hole, bottom, Ty(*(first + hole)), pred);
    }
}

// sort_heap (0x10006320), pop_heap inlined: the last element is saved, the root moved into its slot,
// and the saved element pushed back through _Adjust_heap from the root.
template <class RanIt, class Pr>
void sortHeap(RanIt first, RanIt last, Pr pred)
{
    typedef typename std::iterator_traits<RanIt>::difference_type Diff;
    typedef typename std::iterator_traits<RanIt>::value_type Ty;
    for (; 1 < last - first; --last)
    {
        Ty val = *(last - 1);
        *(last - 1) = *first;
        adjustHeap(first, Diff(0), Diff((last - 1) - first), val, pred);
    }
}

// _Sort (0x10006830).
template <class RanIt, class Diff, class Pr>
void sortRange(RanIt first, RanIt last, Diff ideal, Pr pred)
{
    Diff count;
    for (; 32 < (count = last - first) && 0 < ideal; )
    {   // divide and conquer by quicksort
        std::pair<RanIt, RanIt> mid = unguardedPartition(first, last, pred);
        ideal /= 2, ideal += ideal / 2;   // allow 1.5 log2(N) divisions

        if (mid.first - first < last - mid.second)
            sortRange(first, mid.first, ideal, pred), first = mid.second;
        else
            sortRange(mid.second, last, ideal, pred), last = mid.first;
    }

    if (32 < count)
    {   // heap sort if too many divisions
        makeHeap(first, last, pred);
        sortHeap(first, last, pred);
    }
    else if (1 < count)
    {
        insertionSort(first, last, pred);
    }
}

// std::sort(_First, _Last, _Pred) -> _Sort(_First, _Last, _Last - _First, _Pred).
template <class RanIt, class Pr>
void sort(RanIt first, RanIt last, Pr pred)
{
    sortRange(first, last, last - first, pred);
}

} // namespace vc71sort
} // namespace textureatlas

#endif // TEXTUREATLASBUILDER_VC71SORT_H
