#ifndef __TRANSFORMATION__
#define __TRANSFORMATION__

typedef struct transformation {
    char name[64];
    int max;                    //max number of transformations of this type
    int running;                //number of transformations of this type currently in use
} Transformation;

#endif