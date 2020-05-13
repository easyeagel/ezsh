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

#include<core/server.hpp>

#include"error.hpp"
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
    return ret;
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
        :wstrm_(&w)
    {}

    StdStream(std::wostream* w)
        :wstrm_(w)
    {}

    std::wostream* get() const
    {
        return wstrm_;
    }

    void set(std::wostream* w)
    {
        wstrm_=w;
    }

    template<typename Value>
    StdStream& operator<<(const Value& val)
    {
        if(quiet_==false)
            *wstrm_ << val;
        return *this;
    }

    StdStream& operator<<(char c)
    {
        if(quiet_==false)
            *wstrm_ << core::WCharConverter::from(c);
        return *this;
    }

    StdStream& operator<<(const char* str)
    {
        if(quiet_==false)
            *wstrm_ << core::WCharConverter::from(str);
        return *this;
    }

    StdStream& operator<<(const std::string& str)
    {
        if(quiet_==false)
            *wstrm_ << core::WCharConverter::from(str);
        return *this;
    }

    StdStream& operator<<( std::wostream& (*func)(std::wostream&) )
    {
        if(quiet_==false)
            *wstrm_ << func;
        return *this;
    }

    StdStream& operator<<(const Path& path)
    {
        if(quiet_==false)
            *wstrm_ << core::WCharConverter::from(path.native());
        return *this;
    }

    StdStream& operator<<(const std::tm& tm)
    {
        if(quiet_==false)
        {
            char buf[256];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H:%M:%S", &tm);
            *this << buf;
        }

        return *this;
    }

    void quietSet(bool v=true)
    {
        quiet_=v;
    }

    bool quietGet() const
    {
        return quiet_;
    }

    void write(const char* b, size_t n)
    {
        const auto& tmp=core::WCharConverter::from(b, n);
        wstrm_->write(tmp.data(), tmp.size());
    }

    void write(const std::wstring& str)
    {
        wstrm_->write(str.data(), str.size());
    }
private:
    bool quiet_=false;
    std::wostream* wstrm_;
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
    typedef std::function<void (Context& ctx)> ContextCall;

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

    void set(const std::string& name, const VarSPtr& val);

    void setif(const std::string& name, const VarSPtr& val);

    void unset(const std::string& name)
    {
        vars_.erase(name);
    }

    template<typename Val>
    void set(const std::string& name, const Val& val)
    {
        const VarSPtr ptr=new Variable(val);
        set(name, ptr);
    }

    template<typename Val>
    void setif(const std::string& name, const Val& val)
    {
        auto ptr=get(name);
        if(ptr)
            return;
        set(name, val);
    }

    void exportdo(const std::string& name);

    VarSPtr get(const std::string& name) const;
    std::string stringGet(const std::string& name) const;
    std::string stringGet(const std::string& name, const std::string& def) const;

    //xxxx${varName, N}xxxx
    void cmdlineReplace(ErrorCode& ec, const StrCommandLine& cmd, StrCommandLine& dest) const;

    template<typename C=Context>
    ContextSPtr alloc()
    {
        return ContextSPtr(new C(shared_from_this()));
    }

    template<typename... Args>
    void yield(Args&&... args)
    {
        return coroutine_.yield(std::forward<Args&&>(args)...);
    }

    void resume()
    {
        coroutine_.resume();
    }

    template<typename... Args>
    void start(Args&&... args)
    {
        return coroutine_.start(std::forward<Args&&>(args)...);
    }

    template<typename Call>
    void commandFinishCall(Call&& call)
    {
        commandFinishCall_.emplace_back(std::move(call));
    }

    void commandFinish()
    {
        for(auto& call: commandFinishCall_)
            call(*this);
        commandFinishCall_.clear();
    }

private:
    ContextSPtr front_;
    std::map<std::string, VarSPtr> vars_;

    StdOutStream stdOut_;
    StdErrStream stdErr_;

    core::CoroutineContext coroutine_;

    std::list<ContextCall> commandFinishCall_;
};

class ContextVisitor
{
    ContextVisitor(const ContextVisitor&)=delete;
public:
    ContextVisitor(Context& ctx)
        :ctx_(ctx)
    {}

    void setDo(const std::vector<std::string>& sets);
    void setIfDo(const std::vector<std::string>& sets);
    void setListDo(const std::vector<std::string>& sets);
    void setIfListDo(const std::vector<std::string>& sets);
    void unsetDo(const std::vector<std::string>& sets);
    void echoDo(const std::vector<std::string>& echos);
    void exportDo(const std::vector<std::string>& echos);

private:
    Context& ctx_;
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

class SysVar
{
public:
    enum
    {
        eVarNull,
        eVarOS,
        eVarDelimiter,
        eVarLineBreak,

        eVarEnumCount
    };

    struct VarInfo
    {
        int var;
        std::string name;
        std::string value;
    };

    static const VarInfo& infoGet(unsigned var)
    {
        if(var>=dict_.size())
            return dict_[eVarNull];
        return dict_[var];
    }

    typedef std::array<VarInfo, eVarEnumCount> Dict;

    static const Dict& dictGet()
    {
        return dict_;
    }

private:
    static Dict dict_;
};



}  // namespace ezsh

