//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  error.hpp
//
//   Description:  
//
//       Version:  1.0
//       Created:  2015年01月15日 09时26分42秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<core/msvc.hpp>
#include<core/error.hpp>

namespace ezsh
{

using core::ErrorCode;

class EzshError: public core::details::ECatBase
{
    enum Snatch { eCodeSnatch=8 };

    EzshError()=default;
public:
    static const EzshError& instance()
    {
        static EzshError gs;
        return gs;
    }

    typedef enum
    {
        eGood=0,

        eCount
    }Code_t;

    static ErrorCode ecMake(Code_t ec)
    {
        return ErrorCode(ec, instance());
    }

    static ErrorCode ecMake(Code_t ec, const std::string& msg)
    {
        return ErrorCode(msg, ec, instance());
    }

    template<typename Value>
    static ErrorCode ecMake(Value ec)
    {
        return ErrorCode(static_cast<Code_t>(ec), instance());
    }

    template<typename Value>
    static ErrorCode ecMake(Value ec, const std::string& msg)
    {
        return ErrorCode(msg, static_cast<Code_t>(ec), instance());
    }

    template<typename Value>
    static bool good(Value val)
    {
        return val==static_cast<Value>(eGood);
    }

    template<typename Value>
    static bool bad(Value val)
    {
        return !good(val);
    }

    const char* name() const noexcept(true)
    {
        return "ezshError";
    }

    std::string message(int ec) const;
};


}

