#pragma once

#include <array>
/**
* To add new tiles, follow the instruction mentioned in:
* https://github.students.cs.ubc.ca/CPSC427-2021W-T1/team09/pull/36
*/


// TODO: Remove this when procedural generation is implemented
static constexpr std::array<std::array<uint8_t, 10>, 10> map_layout_1 = {
    2,4,1,2,3,4,0,2,1,3,
    1,2,4,3,3,3,3,2,2,0,
    1,2,2,1,4,3,2,2,2,2,
    1,2,3,2,2,2,2,2,0,3,
    2,5,1,2,0,0,0,2,0,4,
    3,2,2,2,3,4,0,1,0,0,
    4,2,2,2,3,0,0,2,0,0,
    0,0,4,3,2,1,4,4,4,3,
    1,1,1,0,0,0,0,1,2,3,
    1,2,2,2,2,2,2,2,2,5,
};