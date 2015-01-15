//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  error.cpp
//
//   Description:  
//
//       Version:  1.0
//       Created:  2015年01月15日 09时28分31秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include"error.hpp"

namespace ezsh
{

struct ErrorCodeTrait
{
    EzshError::Code_t ec;
    const char* msg;
};

static ErrorCodeTrait errorTraitDict[]=
{
#define MacroDefine(CppEC, CppMsg)\
    {EzshError::CppEC, CppMsg},

    MacroDefine(eGood,  "app: good, no error")

#undef MacroDefine
};

std::string EzshError::message(int ev) const
{
    if(ev>=EzshError::eCount)
        return "未知错误";
    auto const dictCount=sizeof(errorTraitDict)/sizeof(errorTraitDict[0]);
    auto end=errorTraitDict+dictCount;
    auto itr=std::find_if(errorTraitDict, end,
        [ev](const ErrorCodeTrait& err)->bool
        {
            return err.ec==static_cast<EzshError::Code_t>(ev);
        }
    );

    if(itr==end)
        return "未知错误";

    return itr->msg;
}

}

