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

    void stop();

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
};

namespace bp=boost::program_options;

class OptBase
{
protected:
    OptBase(const char* msg)
        :opt_(msg)
    {}

public:
    void help()
    {
        std::cerr << opt_ << std::endl;
    }

    void parse(const ContextSPtr& context, int ac, const char* const* av)
    {
        context_=context;

        bp::command_line_parser parser(ac, av);
        parser.options(opt_);
        parser.positional(optPos_);
        parser.style(bp::command_line_style::default_style);

        bp::store(parser.run(), vm_);
        bp::notify(vm_);
    }

    const bp::variables_map& mapGet() const
    {
        return vm_;
    }

    virtual MainReturn doit() = 0;

    virtual ~OptBase()
    {}

private:
    bp::variables_map vm_;

protected:
    ContextSPtr context_;
    bp::options_description opt_;
    bp::positional_options_description optPos_;
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

};

class CmdDict
{
public:
    typedef std::function<std::unique_ptr<OptBase>()> ObjCreate;
    typedef std::map<std::string, ObjCreate> Dict;

    static ObjCreate find(const std::string& cmd)
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
    CmdDict::dictGet().emplace(Opt::nameGet(), Opt::create);
}

};

}

