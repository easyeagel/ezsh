//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdForeach.cpp
//
//   Description:  foreach 命令实现
//
//       Version:  1.0
//       Created:  2014年12月28日 13时59分25秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include"option.hpp"
#include"script.hpp"

namespace ezsh
{

class CmdForeach:public CommandGroup<CmdForeach>
{
    typedef CommandGroup<CmdForeach> BaseThis;
public:
    CmdForeach(ScriptCommand&& b, Script&& s, ScriptCommand&& e)
        :BaseThis(std::move(b), std::move(s), std::move(e))
    {}

    static const char* beginGet()
    {
        return "foreach";
    }

    static const char* endGet()
    {
        return "endforeach";
    }

};

namespace
{
    CommandGroupRegister<CmdForeach> gsCommandGroupRegister;
}

}

