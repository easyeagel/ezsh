//  Copyright [2014] <lgb (LiuGuangBao)>
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

#include<ctime>
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

typedef std::vector<char*> CommandLine;
typedef std::vector<std::string> StrCommandLine;

static inline CommandLine cmdlineMake(const StrCommandLine& sc)
{
    CommandLine ret;
    for(auto& s: sc)
        ret.push_back(const_cast<char*>(s.data()));
    return std::move(ret);
}

typedef std::string VarString;
typedef std::vector<std::string> VarList;

class Variable: public boost::variant<VarString, VarList>
{
    typedef boost::variant<VarString, VarList> BaseThis;
public:
    template<typename... Args>
    Variable(Args&&... args)
        :BaseThis(std::forward<Args&&>(args)...)
    {}

};

typedef std::shared_ptr<Variable> VarSPtr;

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

    StdStream& operator<<(const std::tm& tm)
    {
        char buf[256];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H:%M:%S", &tm);
        *this << buf;
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

class Context: public std::enable_shared_from_this<Context>
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

    void set(const std::string& name, const VarSPtr& val)
    {
        vars_[name]=val;
    }

    VarSPtr get(const std::string& name) const;

    //xxxx${varName, N}xxxx
    void replace(const std::string& str, StrCommandLine& dest) const;
    void cmdlineReplace(const StrCommandLine& cmd, StrCommandLine& dest) const;

    template<typename C=Context>
    ContextSPtr alloc()
    {
        return ContextSPtr(new C(shared_from_this()));
    }

private:
    ContextSPtr front_;
    std::map<std::string, VarSPtr> vars_;

    StdOutStream stdOut_;
    StdErrStream stdErr_;
};

class ContextStack
{
    ContextStack();
public:
    const ContextSPtr& top() const
    {
        assert(!stack_.empty());
        return stack_.back();
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

