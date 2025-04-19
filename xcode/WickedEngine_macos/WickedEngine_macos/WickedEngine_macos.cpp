//
//  WickedEngine_macos.cpp
//  WickedEngine_macos
//
//  Created by James on 19.04.25.
//

#include <iostream>
#include "WickedEngine_macos.hpp"
#include "WickedEngine_macosPriv.hpp"

void WickedEngine_macos::HelloWorld(const char * s)
{
    WickedEngine_macosPriv *theObj = new WickedEngine_macosPriv;
    theObj->HelloWorldPriv(s);
    delete theObj;
};

void WickedEngine_macosPriv::HelloWorldPriv(const char * s) 
{
    std::cout << s << std::endl;
};

