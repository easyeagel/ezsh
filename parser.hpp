//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  parser.hpp
//
//   Description:  通用解析装置
//
//       Version:  1.0
//       Created:  2015年01月09日 10时17分31秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<boost/xpressive/xpressive.hpp>
#include<boost/algorithm/string/trim.hpp>

#include"error.hpp"

namespace ezsh
{

namespace xpr
{
    using namespace boost::xpressive;
    
    //变量名或宏名
    extern sregex gsVarName;
    extern sregex gsMacroName;
}

class ReplacePattern
{
public:
    enum Type
    {
        eList,
        eValue,
        eLiteral,
    };

    struct Param
    {
        Type type;
        std::string value;
    };

    struct Operator
    {
        std::string name;
        std::vector<Param> params;
    };

    void init(const std::vector<std::string>& what);

    const std::vector<Operator>& operatorsGet() const
    {
        return operators_;
    }

    bool needSplit() const
    {
        return needSplit_;
    }

    template<typename OutItr, typename InItr, typename Format>
    static OutItr replace(ErrorCode& ec, OutItr out, InItr b, InItr e, Format&& format)
    {
        std::string source;
        ReplacePattern pattern;
        for(;;)
        {
            if(b==e)
                return out;

            switch(*b)
            {
                case '!': //只在 !! 或 !$ 解析转义
                {
                    ++b;
                    if(b==e)
                    {
                        EzshError::ecMake(EzshError::ePatternReplaceFailed);
                        return out;
                    }

                    const auto n=*b;
                    if(n=='!' || n=='$')
                    {
                        *out++=n;
                    } else {
                        *out++='!';
                        *out++=n;
                    }

                    ++b;
                    continue;
                }
                case '$': //模式开头 ${
                {
                    ++b;
                    if(b==e)
                    {
                        EzshError::ecMake(EzshError::ePatternReplaceFailed);
                        return out;
                    }

                    const auto n=*b;
                    if(n!='{')
                    {
                        *out++='$';
                        *out++=n;
                        ++b;
                        continue;
                    }

                    //找到最后的 }
                    source.clear();
                    source.push_back('$');
                    source.push_back('{');
                    for(;;)
                    {
                        ++b;
                        if(b==e)
                        {
                            EzshError::ecMake(EzshError::ePatternReplaceFailed);
                            return out;
                        }

                        source.push_back(*b);
                        if(source.size()>=4*1024)
                        {
                            EzshError::ecMake(EzshError::ePatternReplaceFailed);
                            return out;
                        }

                        if(*b=='}')
                        {
                            ++b;
                            if(b!=e && *b=='~')
                            {
                                source.push_back(*b);
                                ++b;
                            }

                            pattern.reset();
                            split(ec, source, pattern);
                            if(ec.bad())
                                return out;
                            out=format(ec, out, pattern);
                            if(ec.bad())
                                return out;
                            break;
                        }
                    }

                    break;
                }
                default:
                {
                    *out++ = *b++;
                    break;
                }
            }
        }

        return out;
    }

    static void split(ErrorCode& ec, const std::string& source, ReplacePattern& dest);

    void reset()
    {
        needSplit_=false;
        operators_.clear();
    }

private:
    bool needSplit_=false;
    std::vector<Operator> operators_;
};

static inline std::pair<std::string, std::string> simpleSplit(const std::string& str, char del='=')
{
    std::string key, val;
    auto const n=str.find(del);
    if(n==std::string::npos)
    {
        key=str;
    } else {
        key=str.substr(0, n);
        val=str.substr(n+1);
    }

    boost::algorithm::trim(key);
    boost::algorithm::trim(val);

    return std::make_pair(key, val);
}

}



