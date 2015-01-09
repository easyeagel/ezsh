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

    //替换模式
    sregex gsNotComma   =~(set=',','}');
    sregex gsNotCommaStr=+gsNotComma;
    sregex gsReplacePattern=as_xpr("${")
        >> *blank >> gsNotCommaStr
        >> *(*blank >> ',' >> *blank >> gsNotCommaStr)
        >> *blank >> '}';

}

}


