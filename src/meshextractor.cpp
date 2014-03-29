#include <bw/meshextractor.h>

namespace detail {

/**
 * return the axis (0,1,2) which is perpendicular
 * to the surface of face with topological coordinate tc 
 * find the dimension with an odd entry
 */
short normalOrientation(Coor tc) {
    for(int i=0; i<3; ++i) {
        if(tc[i] %2 != 0) {
            return i;
        }
    }
    return -1;
}

/** 
 *  return the four corners of the voxel face as defined 
 *  by the topological coordinate tc
 */
void cartesianCorners(Coor tc, std::vector<Coor>& corners) {
    const short n = normalOrientation(tc);
    short a = -1; //first axis of the 2-cell
    short b = -1; //second axis of the 2-cell
    
    for(short i=0; i<3; ++i) {
        if(i != n) { 
            if(a == -1) {
                a = i;
            }
            else {
                b = i;
            }
        }
    }

    short k, l;
    Coor c;
    if(n == 0 || n==2) { 
        //normal vector in x-direction or in z-direction
        for(int i=1; i<=4; ++i) {
            k = int(i/2)%2;     // 0, 1, 1, 0
            l = (i<=2) ? 0 : 1; // 0, 0, 1, 1
            c[n] = (tc[n]+1)/2;
            c[a] =  tc[a]/2 + k;
            c[b] =  tc[b]/2 + l;
            corners[i-1] = c;
        }
    }
    else if(n==1) {
        //normal vector in y-direction
        for(int i=1; i<=4; ++i) {
            k = (i<=2) ? 0 : 1; // 0, 0, 1, 1
            l = int(i/2)%2;     // 0, 1, 1, 0
            c[n] = (tc[n]+1)/2;
            c[a] =  tc[a]/2 + k;
            c[b] =  tc[b]/2 + l;
            corners[i-1] = c;
        }
    }
}

} /* namespace detail */
