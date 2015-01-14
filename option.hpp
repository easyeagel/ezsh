//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  easydb.cpp
//
//   Description:  easyAdmin 主程序
//
//       Version:  1.0
//       Created:  2013年03月05日 17时22分44秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================

#pragma once

#include<cstdio>

#include<map>
#include<list>
#include<memory>
#include<thread>
#include<utility>
#include<iostream>
#include<initializer_list>
#include<boost/program_options.hpp>
#include<boost/asio/io_service.hpp>

#include"context.hpp"

namespace ezsh
{

class TaskPool
{
    TaskPool();
    typedef boost::asio::io_service IOService;
public:
    static TaskPool& instance();

    template<typename Call>
    void post(Call&& call)
    {
        service_.post(std::move(call));
    }

    static void stop();

private:
    void run();

private:
    IOService service_;
    std::vector<std::thread> threads_;
    std::unique_ptr<IOService::work> work_;

private:
    static bool stared_;
};

struct MainReturn
{
    enum Value
    {
        eGood=0,
        eGroupDone,


        eBadStart=64,
        eNotAllowed,
        eParamInvalid,
        eUnkownCommand,
    };

    bool good() const
    {
        return val_<=eBadStart;
    }

    bool bad() const
    {
        return !good();
    }

    MainReturn()=default;
    MainReturn(Value val)
        :val_(val)
    {}

    template<typename Int=Value>
    Int get() const
    {
        return static_cast<Int>(val_);
    }

private:
    Value val_=eGood;
};

namespace bp=boost::program_options;

class OptionComponent
{
public:
    virtual void longHelp (std::ostream& strm) = 0;
    virtual void shortHelp(std::ostream& strm) = 0;
    virtual void options(bp::options_description& opt, bp::positional_options_description& pos) = 0;

    virtual ~OptionComponent()=default;
};

typedef std::shared_ptr<OptionComponent> OptionComponentSPtr;

class TaskBase
{
public:
    virtual MainReturn taskDoit() = 0;

    const ContextSPtr& contextGet() const
    {
        return context_;
    }

    virtual MainReturn init(const ContextSPtr& context)
    {
        context_=context;
        return MainReturn::eGood;
    }

    StdOutStream& stdOut() const
    {
        return contextGet()->stdOut();
    }

    StdErrStream& stdErr() const
    {
        return contextGet()->stdErr();
    }

    virtual ~TaskBase()=default;

protected:
    ContextSPtr context_;
};

class CmdBase: public TaskBase
{
protected:
    CmdBase(const char* msg)
        :opt_(msg)
    {}

    CmdBase()=default;

public:
    typedef bp::variables_map VarMap;
    typedef std::function<void (const VarMap&)> AfterParseCall;

    virtual void help(std::ostream& strm=std::cerr);
    virtual void parse(StrCommandLine&& cl);

    const VarMap& mapGet() const
    {
        return vm_;
    }

    template<typename Call>
    void afterParseCall(Call&& call)
    {
        afterParseCalls_.emplace_back(std::move(call));
    }

    void componentPush(const OptionComponentSPtr& com)
    {
        components_.push_back(com);
    }

private:
    VarMap vm_;
    bp::options_description optAll_;
    std::list<AfterParseCall> afterParseCalls_;

protected:
    bp::options_description opt_;
    bp::positional_options_description optPos_;
    std::vector<OptionComponentSPtr> components_;
};

template<typename Obj, typename Base=CmdBase>
class CmdBaseT: public Base
{
    Obj& objGet()
    {
        return static_cast<Obj&>(*this);
    }

    const Obj& objGet() const
    {
        return static_cast<Obj&>(*this);
    }
public:
    CmdBaseT(const char* msg)
        :Base(msg)
    {}

    template<typename... Args>
    CmdBaseT(Args&&... args)
        :Base(std::forward<Args&&>(args)...)
    {}

    static std::unique_ptr<CmdBase> create()
    {
        return std::unique_ptr<CmdBase>(new Obj);
    }

    static const char* moduleGet()
    {
        return "core";
    }

    MainReturn taskDoit()
    {
        return objGet().doit();
    }

};

class OptionOneAndOnly
{
public:
    OptionOneAndOnly()=default;

    void add(std::initializer_list<std::string> list)
    {
        add(list.begin(), list.end());
    }

    template<typename Itr>
    void add(Itr b, Itr e)
    {
        opts_.insert(opts_.end(), b, e);
    }

    void doit(CmdBase& cb)
    {
        cb.afterParseCall(std::bind(&OptionOneAndOnly::check, this, std::placeholders::_1));
    }

    const std::string& oneGet() const
    {
        return *one_;
    }

private:
    void check(const CmdBase::VarMap& vm)
    {
        size_t count=0;
        for(const auto& opt: opts_)
        {
            const auto n=(vm.find(opt)==vm.end()? 0 : 1);
            if(n==0)
                continue;
            count += n;
            one_=std::addressof(opt);
        }

        if(count!=1)
        {
            std::string msg;
            for(const auto& opt: opts_)
            {
                msg += opt;
                msg += ", ";
            }

            msg += "need one and only one";
            throw bp::error(msg);
        }
    }

private:
    std::string const* one_=nullptr;
    std::vector<std::string> opts_;
};

typedef std::function<std::unique_ptr<CmdBase>()> CmdCreate;

struct CmdTrait
{
    std::string name;
    std::string module;

    CmdCreate create;
};

typedef std::shared_ptr<CmdTrait> CmdTraitSPtr;

class CmdDict
{
public:
    typedef std::map<std::string, CmdTraitSPtr> Dict;

    static CmdTraitSPtr find(const std::string& cmd)
    {
        auto itr=dictGet().find(cmd);
        if(itr==dictGet().end())
            return nullptr;
        return std::get<1>(*itr);
    }

    static Dict& dictGet()
    {
        static Dict gs;
        return gs;
    }
};

template<typename Opt>
struct CmdRegisterT
{

CmdRegisterT()
{
    CmdTraitSPtr ptr(new CmdTrait{Opt::nameGet(), Opt::moduleGet(), Opt::create});
    CmdDict::dictGet().emplace(Opt::nameGet(), std::move(ptr));
}

};

class OptionDict
{
public:
    typedef std::map<std::string, OptionComponentSPtr> Dict;

    static OptionComponentSPtr find(const std::string& cmd)
    {
        auto itr=dictGet().find(cmd);
        if(itr==dictGet().end())
            return nullptr;
        return std::get<1>(*itr);
    }

    static Dict& dictGet()
    {
        static Dict gs;
        return gs;
    }
};

template<typename Option>
struct OptionRegisterT
{

OptionRegisterT()
{
    OptionDict::dictGet().emplace(Option::nameGet(), Option::componentGet());
}

};

class Environment
{
    Environment();
public:
    static const Environment& instance()
    {
        static Environment gs;
        return gs;
    }

    Path pathFile(const Path& file) const;

private:
    std::vector<std::string> paths_;
    std::map<std::string, std::string> maps_;
};



}


