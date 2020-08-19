// zbar64.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "zbar64.h"


// This is an example of an exported variable
ZBAR64_API int nzbar64=0;

// This is an example of an exported function.
ZBAR64_API int fnzbar64(void)
{
    return 0;
}

// This is the constructor of a class that has been exported.
Czbar64::Czbar64()
{
    return;
}
