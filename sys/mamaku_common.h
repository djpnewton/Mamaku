#if !defined(_MAMAKU_COMMON_H_)
#define _MAMAKU_COMMON_H_

typedef struct
{
    int x;
    int y;
    int major;
    int minor;
    int size;
    int id;
    int orientation;
    int state;
} MamakuTrackpadData, *PMamakuTrackpadData;

#endif
