﻿//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  context.hpp
//
//   Description:  执行环境
//
//       Version:  1.0
//       Created:  2014年12月26日 17时28分52秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<cassert>

#include<map>
#include<stack>
#include<memory>
#include<string>
#include<vector>
#include<iostream>
#include<boost/variant.hpp>

#include"filesystem.hpp"

namespace ezsh
{

class Variable:
    public boost::variant<
        std::string,
        std::vector<std::string>
    >
{};

class Context;
class ContextStack;
typedef std::shared_ptr<Context> ContextSPtr;

class StdStream
{
public:
    StdStream(std::wostream& w)
        :wstrm_(w)
    {}

    template<typename Value>
    StdStream& operator<<(const Value& val)
    {
        wstrm_ << val;
        return *this;
    }

    StdStream& operator<<(char c)
    {
        wstrm_ << WCharConverter::from(c);
        return *this;
    }

    StdStream& operator<<(const char* str)
    {
        wstrm_ << WCharConverter::from(str);
        return *this;
    }

    StdStream& operator<<(const std::string& str)
    {
        wstrm_ << WCharConverter::from(str);
        return *this;
    }

    StdStream& operator<<( std::wostream& (*func)(std::wostream&) )
    {
        wstrm_ << func;
        return *this;
    }

    StdStream& operator<<(const Path& path)
    {
        wstrm_ << WCharConverter::from(path.native());
        return *this;
    }

private:
    std::wostream& wstrm_;
};

class StdOutStream: public StdStream
{
public:
    StdOutStream()
        :StdStream(std::wcout)
    {}
};

class StdErrStream: public StdStream
{
public:
    StdErrStream()
        :StdStream(std::wcerr)
    {}
};

class Context
{
    friend class ContextStack;

    Context()=default;
    Context(const ContextSPtr& f)
        :front_(f)
    {}

public:
    const ContextSPtr& frontGet() const
    {
        return front_;
    }

    StdOutStream& stdOut()
    {
        return stdOut_;
    }

    StdErrStream& stdErr()
    {
        return stdErr_;
    }

private:
    ContextSPtr front_;
    std::map<std::string, Variable> variables_;

    StdOutStream stdOut_;
    StdErrStream stdErr_;
};

class ContextStack
{
    ContextStack()
    {
        stack_.emplace_back(ContextSPtr(new Context));
    }
public:
    const ContextSPtr& top() const
    {
        assert(!stack_.empty());
        return stack_.back();
    }

    template<typename C=Context>
    const ContextSPtr& alloc()
    {
        const ContextSPtr ptr(new C(top()));
        stack_.emplace_back(ptr);
        return top();
    }

    template<typename C=Context>
    const ContextSPtr& create()
    {
        return ContextSPtr (new C(top()));
    }

    void pop()
    {
        //最后一个Context是主Context，永远不能被弹出
        if(stack_.size()>1)
            stack_.pop_back();
    }

    static ContextStack& instance()
    {
        static ContextStack gs;
        return gs;
    }

    const ContextSPtr& mainContextGet() const
    {
        return stack_.front();
    }

private:
    std::deque<ContextSPtr> stack_;
};



}  // namespace ezsh

