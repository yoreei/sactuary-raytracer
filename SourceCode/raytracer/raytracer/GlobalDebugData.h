#pragma once
#include <string>
#include "CRTTypes.h"

namespace GlobalDebug
{
    extern Vec3 fErrors;
    extern Vec3 maxError;
    extern Vec3 maxErrorDirection;

    std::string toString();
}
