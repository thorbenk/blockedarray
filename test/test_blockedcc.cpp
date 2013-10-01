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

#define VIGRA_CHECK_BOUNDS

#include <bw/connectedcomponents.h>

#include <iostream>

#include <bw/extern_templates.h>

using namespace BW;

typedef vigra::TinyVector<int, 2> d;

void pprint(d p1, d q1, d p2, d q2) {
    {
        //d r1, r2;
        //bool s = intersect(p1, q1, p2, q2, r1, r2);
        Roi<2> roi(p1, q1);
        Roi<2> result;
        bool s = roi.intersect(Roi<2>(p2, q2), result);
        std::cout << result.p << " -- " << result.q;
        if(!s) { std::cout << " no intersect"; }
        std::cout << std::endl;
    }
    {
        //d r1, r2;
        //bool s = intersect(p2, q2, p1, q1, r1, r2);
        Roi<2> roi(p2, q2);
        Roi<2> result;
        bool s = roi.intersect(Roi<2>(p1, q1), result);
        std::cout << result.p << " -- " << result.q;
        if(!s) { std::cout << " no intersect"; }
        std::cout << std::endl;
    }
    std::cout << "---" << std::endl;
}

int main() {
    pprint(d(1,1), d(10,20), d(5,6), d(30,40));
    pprint(d(10,20), d(1,1), d(30,40), d(20,30));

    typedef vigra::MultiArray<2, int> A;
    typedef Roi<2> R;
    typedef R::V V;

    A a(vigra::Shape2(20,20));
    A b(vigra::Shape2(20,20));

    UnionFindArray<int> ufd;

    //blockMerge<2,int>(ufd, R(V(1,1), V(10,20)), a,
    //                       R(V(5,6), V(30,40)), b);

    Blocking<2> bb( R(V(0,0), V(50,50)), V(10,20), V(1,1) );
    bb.pprint();

    //BlockwiseConnectedComponents<2, int> bcc(a, V(10,10));

    return 0;
}
