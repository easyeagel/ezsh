﻿//  Copyright [2014] <lgb (LiuGuangBao)>
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

#include"error.hpp"
#include"context.hpp"

namespace ezsh
{

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

class TaskBase: public core::ErrorBase
{
public:
    virtual void taskDoit() = 0;

    const ContextSPtr& contextGet() const
    {
        return context_;
    }

    virtual void init(const ContextSPtr& context)
    {
        context_=context;
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
    enum ErrorAttitude_t
    {
        eErrorIgnore, ///< 忽略错误，不设置错误码
        eErrorQuiet,  ///< 对错误保持安静，并设置错误码
        eErrorReport, ///< 报告错误，并继续
        eErrorBreak,  ///< 执行错误，并中止
    };

    CmdBase()=default;
    CmdBase(const char* msg, bool ne=false);

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

    static void setDry(bool v=true)
    {
        dry_=v;
    }

    void errorSet(const ErrorCode& ec)
    {
        if(errorAtt_>eErrorIgnore)
            ecSet(ec);
    }

    bool errorBreak() const
    {
        return errorAtt_>=eErrorBreak;
    }

    void init(const ContextSPtr& context) override;

    const bp::options_description& baseOptionGet() const
    {
        return baseOpt_;
    }

private:
    VarMap vm_;
    bp::options_description optAll_;
    std::list<AfterParseCall> afterParseCalls_;

protected:
    ErrorAttitude_t errorAtt_=eErrorReport;
    bp::options_description opt_;
    bp::positional_options_description optPos_;
    std::vector<OptionComponentSPtr> components_;

protected:
    static bool dry_;
    bp::options_description baseOpt_;
};

namespace details
{
template<typename Call, typename... Args>
struct CallN;

template<typename Call, typename Arg>
struct CallN<Call,Arg>
{
    template<typename... T>
    static bool call(T&&... t)
    {
        return Call::template doit<Arg>(std::forward<T&&>(t)...);
    }
};

template<typename Call, typename Arg, typename... Args>
struct CallN<Call,Arg,Args...>
{
    template<typename... T>
    static bool call(T&&... t)
    {
        return CallN<Call, Arg>::call(std::forward<T&&>(t)...)
            ||  CallN<Call, Args...>::call(std::forward<T&&>(t)...);
    }
};

}

template<typename Obj, typename Base=CmdBase>
class CmdBaseT: public Base
{
    Obj& objGet()
    {
        return static_cast<Obj&>(*this);
    }

    const Obj& objGet() const
    {
        return static_cast<const Obj&>(*this);
    }

    struct Print
    {
        template<typename T>
        static bool doit(StdStream& strm, const boost::any& any)
        {
            const auto t=boost::any_cast<T>(&any);
            if(t==nullptr)
                return false;
            strm << *t << std::endl;
            return true;
        }
    };

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

    void taskDoit()
    {
        if(Base::dry_==false)
        {
            objGet().doit();
            return this->contextGet()->commandFinish();
        }

        //简单打印，不执行操作
        return objGet().doDry();
    }

    void doDry()
    {
        this->stdOut() << objGet().nameGet() << '\n';
        const auto& vm=this->mapGet();
        for(const auto& v: vm)
        {
            this->stdOut() << '\t' << v.first << ": ";

            const auto& any=v.second.value();

            const auto list=boost::any_cast<std::vector<std::string>>(&any);
            if(list!=nullptr)
            {
                const auto& l=*list;
                const auto size=l.size()-1;

                this->stdOut() << "{";
                for(size_t i=0; i<size; ++i)
                    this->stdOut() << l[i] << ", ";
                this->stdOut() << l[size] << "}" << std::endl;
                continue;
            }

            const auto ret=details::CallN
                <Print, std::string, int, unsigned, size_t, double>::call(this->stdOut(), any);
            if(ret==true)
                continue;

            this->stdOut() << std::endl;
        }
    }

    StdErrStream& errorReport() const
    {
        auto& o=this->stdErr();
        o << objGet().nameGet();
        return o;
    }
private:
};

class OptionOneAndOnly
{
public:
    OptionOneAndOnly()=default;

    OptionOneAndOnly(bool v)
        :canIngore_(v)
    {}

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

    bool good() const
    {
        return one_!=nullptr;
    }

    bool bad() const
    {
        return !good();
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

        if(canIngore_)
        {
            if(count==0 || count==1)
                return;
            return error();
        }

        if(count!=1)
            return error();
    }

    void error()
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

private:
    bool canIngore_=false;
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


