// ==========================================================================
//                              radix_inplace.h
// ==========================================================================
// Copyright (c) 2006-2013, Knut Reinert, FU Berlin
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Knut Reinert or the FU Berlin nor the names of
//       its contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL KNUT REINERT OR THE FU BERLIN BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
// ==========================================================================
// Author: Sascha Meiers <meiers@inf.fu-berlin.de>
// ==========================================================================

#include <mutex>
#include <thread>

#ifndef CORE_INCLUDE_SEQAN_INDEX_RADIX_INPLACE_H_
#define CORE_INCLUDE_SEQAN_INDEX_RADIX_INPLACE_H_

namespace SEQAN_NAMESPACE_MAIN
{


// turn on debug output for radix sort (will spam the screen)
#define CORE_INCLUDE_SEQAN_INDEX_RADIX_INPLACE_H_DEBUG_RADIX_SORT

// ============================================================================
// struct RadixTextAccessor
// ============================================================================

template <
    typename TSAValue,          // input
    typename TString,           // string object that is referenced
    typename TSpec = void,      // Suffix modifier
    typename TSize = unsigned>  // return type (ordValue)
struct RadixTextAccessor;
/*
 * NOTE:
 * These accessors cannot resolve the correct order of out-of-bound-positions,
 * i.e. when suffixes are equal up to their last character.
 * All these cases get collected in a 0 bucket.
 * The InplaceRadixSorter takes care of that by calling a special
 * sort function on the 0 buckets.
 */

// ----------------------------------------------------------------------------
// struct RadixTextAccessor                                            [String]
// ----------------------------------------------------------------------------

template <typename TSAValue, typename TString, typename TSize>
struct RadixTextAccessor<TSAValue, TString, void, TSize> :
    public std::unary_function<TSAValue, TSize>
{
    TString const & text;
    typename Size<TString>::Type const L;

    RadixTextAccessor(TString const &str) : text(str), L(length(str))
    {}

    template <typename TSize2>
    inline TSize operator()(TSAValue const &x, TSize2 depth) const
    {
        typename Size<TString>::Type pos = x + depth;
        if (pos >= L)   return 0;
        TSize ret = ordValue(text[pos]);
        return ret+1;
    }
};

// ----------------------------------------------------------------------------
// struct RadixTextAccessor                                         [StringSet]
// ----------------------------------------------------------------------------

template <typename TSAValue, typename TString, typename TSetSpec, typename TSize>
struct RadixTextAccessor<TSAValue, StringSet<TString, TSetSpec>, void, TSize> :
    public std::unary_function<TSAValue, TSize>

{
    StringSet<TString, TSetSpec> const & text;
    String<typename Size<TString>::Type> L;

    RadixTextAccessor(StringSet<TString, TSetSpec> const &str) : text(str)
    {
        resize(L, length(text), Exact());
        for(typename Size<TString>::Type i = 0; i < length(text); ++i)
        L[i] = length(text[i]);
    }

    template <typename TSize2>
    inline TSize operator()(TSAValue const &x, TSize2 depth) const
    {
        typename Size<TString>::Type pos = getSeqOffset(x) + depth;
        typename Size<TString>::Type seq = getSeqNo(x);
        if (pos >= L[seq])   return 0;
        TSize ret = ordValue(text[seq][pos]);
        return ret+1;
    }
};

// ----------------------------------------------------------------------------
// struct RadixTextAccessor                               [String, CyclicShape]
// ----------------------------------------------------------------------------

template <typename TSAValue, typename TString, typename TShape, typename TSize>
struct RadixTextAccessor<TSAValue, TString, ModCyclicShape<CyclicShape<TShape> >, TSize> :
    public std::unary_function<TSAValue, TSize>
{
    TString const & text;
    typename Size<TString>::Type const L,w,s;
    String<typename Size<TString>::Type> positions;

    RadixTextAccessor(TString const &str, CyclicShape<TShape> const & shape) :
        text(str), L(length(str)), w(weight(shape)), s(shape.span)
    {
        carePositions(positions, shape);
        // discard shape
    }

    template <typename TSize2>
    inline TSize operator()(TSAValue const &x, TSize2 depth) const
    {
        // TODO(meiers): Why not use modulo ???
        typename Size<TString>::Type shapePos = depth - depth/w * w; // modulo
        typename Size<TString>::Type pos = x + depth/w * s + positions[ shapePos ];
        if (pos >= L) return 0;
        TSize ret = ordValue(text[pos]);
        return ret+1;
    }
};

// ----------------------------------------------------------------------------
// struct RadixTextAccessor                            [StringSet, CyclicShape]
// ----------------------------------------------------------------------------

// TODO: Maybe specialised version of hardwired shape that accesses the text even faster??

template <typename TSAValue, typename TString, typename TSetSpec, typename TShape, typename TSize>
struct RadixTextAccessor<TSAValue,
                        StringSet<TString, TSetSpec>,
                        ModCyclicShape<CyclicShape<TShape> >,
                        TSize> :
    public std::unary_function<TSAValue, TSize>

{
    StringSet<TString, TSetSpec> const & text;
    String<typename Size<TString>::Type> L;
    String<typename Size<TString>::Type> positions;
    const typename Size<TString>::Type w, s;

    RadixTextAccessor(StringSet<TString, TSetSpec> const &str, CyclicShape<TShape> const & shape) :
        text(str), w(weight(shape)), s(shape.span)
    {
        carePositions(positions, shape);
        resize(L, length(text), Exact());
        for(typename Size<TString>::Type i = 0; i < length(text); ++i)
            L[i] = length(text[i]);
    }

    template <typename TSize2>
    inline TSize operator()(TSAValue const &x, TSize2 depth) const
    {
        typename Size<TString>::Type shapePos = depth - depth/w * w; // = depth % w
        typename Size<TString>::Type pos = getSeqOffset(x) +
                                           depth/w * s + positions[ shapePos ];
        typename Size<TString>::Type seq = getSeqNo(x);
        if (pos >= L[seq])   return 0;
        TSize ret = ordValue(text[seq][pos]);
        return ret+1;
    }
};


// ============================================================================
// functor InplaceRadixSorter and accessories
//
// The following Radix Sort functions are adapted from Martin Frith's "last"
// tool (last.cbrc.jp), but he himself adapted the code from McIlroy, Bostic:
// "Engineering radix sort" as well as Kärkkäinen, Rantala: "Engineering radix
// sort for strings". Thanks to Martin for showing this to me.
// ============================================================================

// ----------------------------------------------------------------------------
// RecursionStack.
// ----------------------------------------------------------------------------

template <typename TSAValue, typename TSmallSize>
struct _RadixRecursionStackEntry
{
    TSAValue * from;
    TSAValue * to;
    TSmallSize depth;
    _RadixRecursionStackEntry()
    {}
    _RadixRecursionStackEntry(TSAValue *a, TSAValue *b, TSmallSize d) :
    from(a), to(b), depth(d)
    {}
};

template <typename TSAValue, typename TSmallSize=unsigned>
struct RadixRecursionStack
{
    typedef _RadixRecursionStackEntry<TSAValue, TSmallSize> TEntry;
    std::vector<TEntry> stack;

    RadixRecursionStack()
    {
        reserve(stack, 20); // todo (change back to reasonable number e.g. 20,000
    }

    inline bool empty() { return length(stack) <=0; }

    inline void push(TSAValue *beg, TSAValue *end, TSmallSize depth)
    {
        stack.push_back(TEntry(beg, end, depth));
    }
    inline void pop(TSAValue *& beg, TSAValue *& end, TSmallSize &depth)
    {
        TEntry & top = stack.back();
        beg = top.from;
        end = top.to;
        depth = top.depth;
        stack.pop_back();
    }
};


// TODO: this stack is not very generic, but it MIGHT be faster.
/*
template <typename TSAValue, typename TSmallSize=unsigned>
struct RadixRecursionStack
{
    typedef _RadixRecursionStackEntry<TSAValue, TSmallSize> TEntry;
    static const unsigned STACKSIZE = 256*256*4;
    TEntry stack[STACKSIZE]; // enough for depth 256 on char
    TEntry *top;

    RadixRecursionStack() : top(stack) {}

    inline bool empty() { return top <= stack; }

    inline void push(TSAValue *beg, TSAValue *end, TSmallSize depth)
    {
        top->from = beg;
        top->to = end;
        top->depth = depth;
        ++top;
        if (top == stack + STACKSIZE -1)
            std::cerr << "FATAL ERROR: Radix Sort Stack limit reached." << std::endl;
    }
    inline void pop(TSAValue *& beg, TSAValue *& end, TSmallSize &depth)
    {
        --top;
        beg = top->from;
        end = top->to;
        depth = top->depth;
    }
};
*/

// ----------------------------------------------------------------------------
// InplaceRadixSorter                                   general alphabet <= 256
// ----------------------------------------------------------------------------

template <
    unsigned Q,                             // alph size = ValueSize + 1
    typename TAccessFunctor,                // text accessor
    typename TOrderFunctor,                 // For seperate sort of the 0 bucket.
    typename TSize = unsigned,              // type of depth and bucketCount a.s.o
    typename TBucketValue = unsigned>       // type the alphabet gets translated to
struct InplaceRadixSorter
{
    typedef typename TAccessFunctor::argument_type      TSAValue;
    typedef typename TAccessFunctor::result_type        TOrdValue; // unsigned

    static const unsigned ORACLESIZE = 256;
    TAccessFunctor 	   textAccess; // IMPORTANT: functors as copies
    TOrderFunctor 	   comp;

    InplaceRadixSorter(TAccessFunctor const & f, TOrderFunctor const & c) :
        textAccess(f), comp(c)
    {}

    inline void operator()(TSAValue * beg,
                           TSAValue * end,
                           TSize depth,
                           RadixRecursionStack<TSAValue, TSize> & stack)
    {
        // Note(meiers): Watch out here when you want to parallelize
        static TSize bucketSize[Q];  // initialized to zero at startup
        TSAValue* bucketEnd[Q];  // "static" makes little difference to speed




        // EXPERIMENTAL ////////////////////////////////////////////////////////
        RadixRecursionStack<TSAValue, TSize> tempStack;
        // END EX //////////////////////////////////////////////////////////////



        // get bucket ends, and put buckets on the stack to sort within them later:
        // EDIT: 0 bucket is not sorted here, but later.
        TSize zeroBucketSize = bucketSize[0];
        TSAValue* pos     = beg + bucketSize[0];
        bucketEnd[0] = pos;

        for( unsigned i = 1; i < Q; ++i )
        {
            TSAValue* nextPos = pos + bucketSize[i];
            if (nextPos - pos > 1)

            //#pragma omp critical(stacklock)
            {
                //stack.push(pos, nextPos, depth+1);
                tempStack.push(pos, nextPos, depth+1);
            }
            pos = nextPos;
            bucketEnd[i] = pos;
        }

        // permute items into the correct buckets:
        for( TSAValue* i = beg; i < end; ) {
            TOrdValue subset;  // unsigned is faster than uchar!
            TSAValue holdOut = *i;
            while( --bucketEnd[ subset = textAccess(holdOut, depth) ] > i )
                std::swap( *bucketEnd[subset], holdOut );
            *i = holdOut;
            i += bucketSize[subset];
            bucketSize[subset] = 0;  // reset it so we can reuse it
        }

        // sort the 0 bucket using std::sort
        if(zeroBucketSize > 1)
            std::sort(beg, beg+zeroBucketSize, comp);

        #pragma omp critical(stacklock)
        {
            while (!tempStack.stack.empty())
            {
                stack.stack.push_back( tempStack.stack.back() );
                tempStack.stack.pop_back();
            }
        }

    }
};

// ----------------------------------------------------------------------------
// InplaceRadixSorter                                                 Dna (4+1)
// ----------------------------------------------------------------------------
/*
 template <typename TValue, typename TFunctor, typename TSize>
 struct Radix<TValue, 5, TFunctor, TSize> {

 TFunctor const & textAccess;

 Radix(TFunctor const & f) : textAccess(f)
 {}

 inline void operator()(
 TValue *beg,
 TValue *end,
 TSize depth,
 RadixRecursionStack<TValue, TSize> & stack)
 {
 TValue *end0 = beg,
 *end1 = beg,
 *end2 = beg,
 *end3 = beg,
 *beg4 = end;
 while(end3 < beg4)
 {
 TValue x = *end3;
 switch(textAccess(x, depth) ) {
 case 0:
 *end3++ = *end2;
 *end2++ = *end1;
 *end1++ = *end0;
 *end0++ = x;
 break;
 case 1: // A
 *end3++ = *end2;
 *end2++ = *end1;
 *end1++ = x;
 break;
 case 2: // C
 *end3++ = *end2;
 *end2++ = x;
 break;
 case 3: // G
 ++end3;
 break;
 default: // T
 *end3 = *--beg4;
 *beg4 = x;
 break;
 }
 }
 stack.push(beg, end0, depth+1); // which order?
 stack.push(end3, end, depth+1);
 stack.push(end2, end3, depth+1);
 stack.push(end1, end2, depth+1);
 stack.push(end0, end1, depth+1);
 }
 }; */
template <typename TStr, typename TSA, typename TPos>
void __outputSA(TStr const & str, TSA const & sa, TPos from, TPos to)
{
    for(TPos x = from; x < to; ++x)
        if (length(suffix(str, sa[x])) > 20)
            std::cout << x << ": " << sa[x] << "  \t" << prefix(suffix(str, sa[x]),20) << "..." << std::endl;
        else
            std::cout << x << ": " << sa[x] << "  \t" << suffix(str, sa[x]) << std::endl;
}

// ----------------------------------------------------------------------------
// Functors to compare suffixes from 0 bucket (suffixes that are lex. equal)
// ----------------------------------------------------------------------------

template <typename TSAValue, typename TLimitsString=Nothing const>
struct _ZeroBucketComparator
{
    TLimitsString const & limits;
    _ZeroBucketComparator(TLimitsString const & lim) : limits(lim)  { /*std::cout << "limits: " << limits << std::endl;*/   }

    inline bool operator()(TSAValue const & a, TSAValue const & b) const
    {
        typename Size<TLimitsString>::Type lena = limits[getSeqNo(a)+1]-limits[getSeqNo(a)] - getSeqOffset(a);
        typename Size<TLimitsString>::Type lenb = limits[getSeqNo(b)+1]-limits[getSeqNo(b)] - getSeqOffset(b);	
        if (lena == lenb)
            return getSeqNo(a) > getSeqNo(b);
        else
            return lena < lenb;
    }
};

// String
template <typename TSAValue>
struct _ZeroBucketComparator<TSAValue, Nothing const>
{
    _ZeroBucketComparator(Nothing const &) {}
    _ZeroBucketComparator(Nothing &) {}


    inline bool operator()(TSAValue const & a, TSAValue const & b) const
    {
        return a > b;
    }
};

// ----------------------------------------------------------------------------
// Function inplaceRadixSort()                                        [default]
// ----------------------------------------------------------------------------

template <typename TSA, typename TString>
void inplaceRadixSort(
                      TSA & sa,
                      TString const & str,
                      typename Size<TString>::Type maxDepth)
{
    typedef typename Value<typename Concatenator<TString>::Type>::Type TAlphabet;
    typedef typename Value<TSA>::Type                               TSAValue;
    typedef typename Size<TString>::Type                            TSize;
    typedef typename StringSetLimits<TString const>::Type           TLimitsString; // "Nothing" for Strings

    typedef RadixTextAccessor<TSAValue, TString>                    TAccessor;
    typedef _ZeroBucketComparator<TSAValue, TLimitsString>          TZeroComp;

    static const unsigned SIGMA = static_cast<unsigned>(ValueSize<TAlphabet>::VALUE) + 1;
    SEQAN_ASSERT_LT_MSG(SIGMA, 1000u, "Attention: inplace radix sort is not suited for large alphabets");

    typedef InplaceRadixSorter<SIGMA, TAccessor, TZeroComp, TSize>    TSorter;

    if (empty(sa)) return; // otherwise access sa[0] fails

    TAccessor 	textAccess(str);
    TSorter 	radixSort(textAccess, TZeroComp(stringSetLimits(str)));

    RadixRecursionStack<TSAValue, TSize> stack;
    stack.push(&sa[0], &sa[0]+length(sa), 0);

    while(!stack.empty())
    {
        TSAValue *from;
        TSAValue *to;
        TSize currDepth;
        stack.pop(from, to, currDepth);

        if(currDepth >= maxDepth)
        continue;

        // Temporarily disabled. Working on mutex
        //radixSort(from, to, currDepth, stack);
    }
}


// ----------------------------------------------------------------------------
// Function inplaceRadixSort()                              [modified Suffixes]
// ----------------------------------------------------------------------------
// NOTE: General for all cyclic suffix modifiers, as long as a corresponding
//      text accessor exists.

template <typename TSA, typename TString, typename TMod>
void inplaceRadixSort(
                      TSA & sa,
                      TString const & str,
                      typename Size<TString>::Type maxDepth,
                      typename Cargo<ModifiedString<TString, TMod> >::Type const & modiferCargo,
                      TMod const &)
{
    typedef typename Value<typename Concatenator<TString>::Type>::Type TAlphabet;
    typedef typename Value<TSA>::Type                               TSAValue;
    typedef typename Size<TString>::Type                            TSize;
    typedef typename StringSetLimits<TString const>::Type           TLimitsString; // "Nothing" for Strings

    typedef RadixTextAccessor<TSAValue, TString, TMod>              TAccessor;
    typedef _ZeroBucketComparator<TSAValue, TLimitsString>          TZeroComp;

    static const unsigned SIGMA = static_cast<unsigned>(ValueSize<TAlphabet>::VALUE) + 1;
    SEQAN_ASSERT_LT_MSG(SIGMA, 1000u, "Attention: inplace radix sort is not suited for large alphabets");

    typedef InplaceRadixSorter<SIGMA, TAccessor, TZeroComp, TSize>    TSorter;

    if (empty(sa)) return; // otherwise access sa[0] fails

    TAccessor 	textAccess(str, modiferCargo);
    TSorter 	radixSort(textAccess, TZeroComp(stringSetLimits(str)));


    RadixRecursionStack<TSAValue, TSize> stack;
    stack.push(&sa[0], &sa[0]+length(sa), 0);

    while(!stack.empty())
    {
        TSAValue *from;
        TSAValue *to;
        TSize currDepth;
        stack.pop(from, to, currDepth);

        if(currDepth >= maxDepth)
            continue;

        // Temporarily disabled. Working on mutex
        //radixSort(from, to, currDepth, stack);
    }
}


// ----------------------------------------------------------------------------
// Function inplaceFullRadixSort()                                    [default]
// ----------------------------------------------------------------------------


template <typename TSA, typename TString>
void inplaceFullRadixSort( TSA & sa, TString const & str)
{
    typedef typename Value<typename Concatenator<TString>::Type>::Type TAlphabet;
    typedef typename Value<TSA>::Type                               TSAValue;
    typedef typename Size<TString>::Type                            TSize;
    typedef typename StringSetLimits<TString const>::Type           TLimitsString; // "Nothing" for Strings

    typedef RadixTextAccessor<TSAValue, TString>                    TAccessor;
    typedef _ZeroBucketComparator<TSAValue,TLimitsString>           TZeroComp;

    static const unsigned SIGMA = static_cast<unsigned>(ValueSize<TAlphabet>::VALUE) + 1;
    SEQAN_ASSERT_LT_MSG(SIGMA, 1000u, "Attention: inplace radix sort is not suited for large alphabets");

    typedef InplaceRadixSorter<SIGMA, TAccessor, TZeroComp, TSize>    TSorter;

    if (empty(sa)) return; // otherwise access sa[0] fails

    TAccessor 	textAccess(str);
    TSorter 	radixSort(textAccess, TZeroComp(stringSetLimits(str)));

    RadixRecursionStack<TSAValue, TSize> stack;
    stack.push(&sa[0], &sa[0]+length(sa), 0);


    // experimental ////////////////////////////////////////////////////////////


    // sort by the first character
    {
        TSAValue *from;
        TSAValue *to;
        TSize currDepth;
        stack.pop(from, to, currDepth);
        radixSort(from, to, currDepth, stack);
    }
    std::cout << "Finished sorting by the first charracter" << std::endl;


    #pragma omp parallel
    {
        // flag per thread
        bool done = false;
        TSorter 	radixSort_thread(textAccess, TZeroComp(stringSetLimits(str))); // input is const &

        while (!done)
        {
            TSAValue *from;
            TSAValue *to;
            TSize currDepth;

            #pragma omp critical(stacklock)
            {
                if (stack.empty())
                    done = true;
                else
                    stack.pop(from, to, currDepth);

                std::cout << "Thread " << omp_get_thread_num() << " is done? " << done << std::endl;
            }
            if (!done)
                radixSort_thread(from, to, currDepth, stack);
        }
    }


    // end experimental ////////////////////////////////////////////////////////


    /*

    while(!stack.empty())
    {
        TSAValue *from;
        TSAValue *to;
        TSize currDepth;
        stack.pop(from, to, currDepth);

        if(to - from < 2)
        continue;

        // other sort algorithm for small buckets:
//        if(to - from < 20)
//        {
//            ::std::sort( from, to, SuffixLess_<TSAValue, TString const, void>(str, currDepth));
//            continue;
//        }

        radixSort(from, to, currDepth, stack);
    }

    */
}


// ----------------------------------------------------------------------------
// Function inplaceFullRadixSort()                          [modified Suffixes]
// ----------------------------------------------------------------------------
// NOTE: General for all cyclic suffix modifiers, as long as a corresponding
//      text accessor in radixSort exists.
template <typename TSA, typename TString, typename TMod>
void inplaceFullRadixSort(TSA & sa,
                          TString const & str,
                          typename Cargo<ModifiedString<TString, TMod> >::Type const & modiferCargo,
                          TMod const &)
{
    typedef typename Value<typename Concatenator<TString>::Type>::Type TAlphabet;
    typedef typename Value<TSA>::Type                               TSAValue;
    typedef typename Size<TString>::Type                            TSize;
    typedef typename StringSetLimits<TString const>::Type           TLimitsString; // "Nothing" for Strings

    typedef RadixTextAccessor<TSAValue, TString, TMod>              TAccessor;
    typedef _ZeroBucketComparator<TSAValue,TLimitsString>           TZeroComp;
 
    static const unsigned SIGMA = static_cast<unsigned>(ValueSize<TAlphabet>::VALUE) + 1;
    SEQAN_ASSERT_LT_MSG(SIGMA, 1000u, "Attention: inplace radix sort is not suited for large alphabets");

    typedef InplaceRadixSorter<SIGMA, TAccessor, TZeroComp, TSize>    TSorter;
    
    if (empty(sa)) return; // otherwise access sa[0] fails

    TAccessor 	textAccess(str, modiferCargo);
    TSorter 	radixSort(textAccess, TZeroComp(stringSetLimits(str)));
    
    RadixRecursionStack<TSAValue, TSize> stack;
    stack.push(&sa[0], &sa[0]+length(sa), 0);
    
    while(!stack.empty())
    {
        TSAValue *from;
        TSAValue *to;
        TSize currDepth;
        stack.pop(from, to, currDepth);
        
        if(to - from < 2)
        continue;

        // other sort algorithm for small buckets:
/*        if(to - from < 20)
        {
            ::std::sort( from, to, SuffixLess_<TSAValue, TString const, TMod>(str, modiferCargo, currDepth));
            continue;
        }
*/
        // Temporarily disabled. Working on mutex
        //radixSort(from, to, currDepth, stack);
    }
}
    
    
    
}


#endif  // #ifndef CORE_INCLUDE_SEQAN_INDEX_RADIX_INPLACE_H_
