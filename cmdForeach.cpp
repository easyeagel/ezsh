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
    CmdForeach(Script&& c)
        :BaseThis(std::move(c))
    {}

    static const char* nameGet()
    {
        return "foreach";
    }

    MainReturn doit(const ContextSPtr& ) override
    {
        return MainReturn::eGood;
    }

};

}

