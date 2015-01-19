//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  parser.cpp
//
//   Description:  
//
//       Version:  1.0
//       Created:  2015年01月09日 11时52分15秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include"parser.hpp"

#include<boost/xpressive/xpressive.hpp>
#include<boost/algorithm/string/trim.hpp>

namespace ezsh
{

namespace xpr
{
    using namespace boost::xpressive;
    
    //变量名或宏名
    sregex gsVarName   = (alpha|'_') >> keep(*(alnum|'_'));
    sregex gsMacroName = gsVarName;
}

void ReplacePattern::init(const std::vector<std::string>& what)
{
    Operator op;
    std::for_each(what.begin(), what.end(),
        [this, &op](const std::string& str)
        {
            if(str.empty())
                return;

            switch(str[0])
            {
                case '@':
                {
                    if(!op.name.empty())
                        operators_.emplace_back(std::move(op));
                    op.params.clear();
                    op.name=str.substr(1);
                    return;
                }
                case '*':
                    op.params.emplace_back(Param{eValue, str.substr(1)});
                    break;
                case '+':
                    op.params.emplace_back(Param{eValue, str.substr(1)});
                    break;
                default:
                    op.params.emplace_back(Param{eLiteral, str});
                    break;
            }

            if(op.name.empty())
                op.name="replace";
        }
    );

    if(!op.name.empty())
        operators_.emplace_back(std::move(op));
}

void ReplacePattern::split(ErrorCode& , const std::string& source, ReplacePattern& dest)
{
    auto b=source.begin();
    b += 2; //跳过 ${

    auto e=source.end()-1;
    if(source.back()=='~')
    {
        dest.needSplit_=true;
        e -= 1;
    }


    std::vector<std::string> token;
    for(;;)
    {
        if(b>=e)
            break;
        auto w=std::find(b, e, ',');
        token.emplace_back(b, w);
        b=w+1;
        boost::algorithm::trim(token.back());
    }

    dest.init(token);
}

}


