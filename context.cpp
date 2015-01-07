//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  context.cpp
//
//   Description:  运行环境实现
//
//       Version:  1.0
//       Created:  2014年12月28日 14时04分03秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include"context.hpp"
#include<boost/xpressive/xpressive.hpp>

namespace ezsh
{

namespace xpr
{
using namespace boost::xpressive;

static sregex gsName= (as_xpr('_') | alpha) >> *(alnum|'_');
static sregex gsVar = as_xpr("${") >> *blank >> (s1=gsName) >> *blank >> '}';

}

VarSPtr Context::get(const std::string& name) const
{
    auto ctx=shared_from_this();
    while(ctx)
    {
        auto const itr=ctx->vars_.find(name);
        if(itr==vars_.end())
        {
            ctx=ctx->frontGet();
            continue;
        }

        return itr->second;
    }

    return nullptr;
}

bool Context::replace(const std::string& str, std::string& dest) const
{
    bool ret=true;
    namespace bx=boost::xpressive;
    bx::regex_replace(std::back_inserter(dest), str.begin(), str.end(), xpr::gsVar,
        [this, &ret](const bx::smatch& match) -> std::string
        {
            const auto& name=match[bx::s1].str();

            //逐层向上查找
            auto ctx=shared_from_this();
            while(ctx)
            {
                auto const itr=ctx->vars_.find(name);
                if(itr==vars_.end())
                {
                    ctx=ctx->frontGet();
                    continue;
                }

                const auto var=itr->second.get();
                auto const strVal=boost::get<VarString>(var);
                if(strVal!=nullptr)
                    return *strVal;

                auto const listVal=boost::get<VarList>(var);
                return listVal->front();
            }

            ret=false;
            return std::string();
        }
    );

    return ret;
}

}



