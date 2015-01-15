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

    MacroDefine(eGood,          "app: good, no error")
    MacroDefine(eGroupDone,     "app: command groud done" )
    MacroDefine(eBadStart,      "app: error code start" )
    MacroDefine(eNotAllowed,    "app: not allowed" )
    MacroDefine(eParamInvalid,  "app: param invalid" )
    MacroDefine(eParamNotExist, "app: param not exist" )
    MacroDefine(eUnkownCommand, "app: unkown command" )

#undef MacroDefine
};

std::string EzshError::message(int ev) const
{
    if(ev>=EzshError::eEnumCount)
        return "unkonw error";

    auto const dictCount=sizeof(errorTraitDict)/sizeof(errorTraitDict[0]);
    auto const end=errorTraitDict+dictCount;
    auto const itr=std::find_if(errorTraitDict, end,
        [ev](const ErrorCodeTrait& err)->bool
        {
            return err.ec==static_cast<EzshError::Code_t>(ev);
        }
    );

    if(itr==end)
        return "unkown error";

    return itr->msg;
}

}

