/************************************************************************/
/*                                                                      */
/*    Copyright 2013 by Thorben Kroeger                                 */
/*    thorben.kroeger@iwr.uni-heidelberg.de                             */
/*                                                                      */
/*    Permission is hereby granted, free of charge, to any person       */
/*    obtaining a copy of this software and associated documentation    */
/*    files (the "Software"), to deal in the Software without           */
/*    restriction, including without limitation the rights to use,      */
/*    copy, modify, merge, publish, distribute, sublicense, and/or      */
/*    sell copies of the Software, and to permit persons to whom the    */
/*    Software is furnished to do so, subject to the following          */
/*    conditions:                                                       */
/*                                                                      */
/*    The above copyright notice and this permission notice shall be    */
/*    included in all copies or substantial portions of the             */
/*    Software.                                                         */
/*                                                                      */
/*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND    */
/*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES   */
/*    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND          */
/*    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT       */
/*    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,      */
/*    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR     */
/*    OTHER DEALINGS IN THE SOFTWARE.                                   */
/*                                                                      */
/************************************************************************/

#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <vigra/random.hxx>

template<class Array>
bool arraysEqual(const Array& a, const Array& b) {
    if(a.shape() != b.shape()) {
        std::stringstream err;
        err << "shape " << a.shape() << " vs. " << b.shape();
        throw std::runtime_error(err.str());
    }
    for(size_t x=0; x<a.size(); ++x) {
        if(a[x] != b[x]) {
            std::stringstream err;
            err << "a[" << x << "]=" << a[x] << " vs b[" << x << "]=" << b[x] << std::endl;
            std::cout << "error: arrays not equal: " << err.str() << std::endl;
            return false;
        }
    }
    return true;
}

template<class T, class Iter>
struct FillRandom {
    static void fillRandom(Iter a, Iter b) {
        throw std::runtime_error("not specialized");
    }
};

//
// uint
//

template<class Iter>
struct FillRandom<vigra::UInt8, Iter> {
    static void fillRandom(Iter a, Iter b) {
        vigra::RandomMT19937 random;
        for(; a!=b; ++a) {
            *a = random.uniformInt(256);
        }
    }
};

template<class Iter>
struct FillRandom<vigra::UInt16, Iter> {
    static void fillRandom(Iter a, Iter b) {
        vigra::RandomMT19937 random;
        for(; a!=b; ++a) {
            *a = random.uniformInt(std::numeric_limits<vigra::UInt16>::max());
        }
    }
};

template<class Iter>
struct FillRandom<vigra::UInt32, Iter> {
    static void fillRandom(Iter a, Iter b) {
        vigra::RandomMT19937 random;
        for(; a!=b; ++a) {
            *a = random.uniformInt(std::numeric_limits<vigra::UInt32>::max());
        }
    }
};

template<class Iter>
struct FillRandom<vigra::UInt64, Iter> {
    static void fillRandom(Iter a, Iter b) {
        vigra::RandomMT19937 random;
        for(; a!=b; ++a) {
            *a = random.uniformInt(std::numeric_limits<vigra::UInt64>::max());
        }
    }
};

//
// int
//

template<class Iter>
struct FillRandom<vigra::Int8, Iter> {
    static void fillRandom(Iter a, Iter b) {
        vigra::RandomMT19937 random;
        for(; a!=b; ++a) {
            *a = random.uniformInt(256);
        }
    }
};

template<class Iter>
struct FillRandom<vigra::Int16, Iter> {
    static void fillRandom(Iter a, Iter b) {
        vigra::RandomMT19937 random;
        for(; a!=b; ++a) {
            *a = random.uniformInt(std::numeric_limits<vigra::Int16>::max());
        }
    }
};

template<class Iter>
struct FillRandom<vigra::Int32, Iter> {
    static void fillRandom(Iter a, Iter b) {
        vigra::RandomMT19937 random;
        for(; a!=b; ++a) {
            *a = random.uniformInt(std::numeric_limits<vigra::Int32>::max());
        }
    }
};

template<class Iter>
struct FillRandom<vigra::Int64, Iter> {
    static void fillRandom(Iter a, Iter b) {
        vigra::RandomMT19937 random;
        for(; a!=b; ++a) {
            *a = random.uniformInt(std::numeric_limits<vigra::Int64>::max());
        }
    }
};

//
// float
//

template<class Iter>
struct FillRandom<float, Iter> {
    static void fillRandom(Iter a, Iter b) {
        vigra::RandomMT19937 random;
        for(; a!=b; ++a) {
            *a = random.uniform();
        }
    }
};

template<class Iter>
struct FillRandom<double, Iter> {
    static void fillRandom(Iter a, Iter b) {
        vigra::RandomMT19937 random;
        for(; a!=b; ++a) {
            *a = random.uniform();
        }
    }
};


#endif /* TEST_UTILS_H */
