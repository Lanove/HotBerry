
#include "pico/stdlib.h"

class ili9486_drivers{
    public:
    ili9486_drivers(uint8_t*dataPins);
    private:
    uint8_t _d[8];
    uint8_t _cs,_rs,_rst,_wrx,_rdx;
};