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
#include<memory>
#include<thread>
#include<utility>
#include<iostream>
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

enum class MainReturn
{
    eGood=0,
    eParamInvalid=1,
    eNotAllowed=2,
};

namespace bp=boost::program_options;

class OptComponent
{
public:
    virtual void longHelp (std::ostream& strm) = 0;
    virtual void shortHelp(std::ostream& strm) = 0;
    virtual void options(bp::options_description& opt, bp::positional_options_description& pos) = 0;

    virtual ~OptComponent()=default;
};

typedef std::shared_ptr<OptComponent> OptComponentSPtr;

class OptBase
{
protected:
    OptBase(const char* msg)
        :opt_(msg)
    {}

public:
    virtual void help(std::ostream& strm=std::cerr);
    virtual void parse(int ac, const char* const* av);
    virtual MainReturn init(const ContextSPtr& context);
    virtual ~OptBase();

    virtual MainReturn doit() = 0;

    const bp::variables_map& mapGet() const
    {
        return vm_;
    }

private:
    bp::variables_map vm_;

protected:
    ContextSPtr context_;

    bp::options_description opt_;
    bp::positional_options_description optPos_;

    bp::options_description optComponents_;
    std::vector<OptComponentSPtr> components_;
};

template<typename Obj>
class OptBaseT: public OptBase
{
public:
    OptBaseT(const char* msg)
        :OptBase(msg)
    {}

    static std::unique_ptr<OptBase> create()
    {
        return std::unique_ptr<OptBase>(new Obj);
    }

    static const char* moduleGet()
    {
        return "core";
    }

};

typedef std::function<void ()> CmdHelp;
typedef std::function<std::unique_ptr<OptBase>()> CmdCreate;

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
struct OptRegisterT
{

OptRegisterT()
{
    CmdTraitSPtr ptr(new CmdTrait{Opt::nameGet(), Opt::moduleGet(), Opt::create});
    CmdDict::dictGet().emplace(Opt::nameGet(), std::move(ptr));
}

};

}

