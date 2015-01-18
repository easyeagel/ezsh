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
    sregex gsVarName   = (alpha|'_') >> *(alnum|'_');
    sregex gsMacroName = gsVarName;

    //统一替换模式: ${@operator, *value, +list}~
    sregex gsNotComma   =~(set=',','}');
    sregex gsNotCommaStr=+gsNotComma;
    sregex gsReplacePattern=as_xpr("${")
        >> *blank >> gsNotCommaStr >> *blank
        >> *(',' >> *blank >> gsNotCommaStr >> *blank)
        >> *blank >> '}'
        >> repeat<0,1>('~');
}

void ReplacePattern::init(const xpr::smatch& what)
{
    if(what.str().back()=='~')
        needSplit_=true;

    using namespace xpr;

    auto begin = what.nested_results().begin();
    auto end   = what.nested_results().end();

    sregex_id_filter_predicate name(gsNotCommaStr.regex_id());

    Operator op;
    std::for_each(
        boost::make_filter_iterator(name, begin, end),
        boost::make_filter_iterator(name, end, end),
        [this, &op](const smatch& m)
        {
            const auto& str=m.str();
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

}


