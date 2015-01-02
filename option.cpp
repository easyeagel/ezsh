//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  option.cpp
//
//   Description:  选项实现
//
//       Version:  1.0
//       Created:  2014年12月26日 14时03分30秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include"option.hpp"

namespace ezsh
{

TaskPool::TaskPool()
    :threads_(std::thread::hardware_concurrency()), work_(new IOService::work(service_))
{
    run();
}

TaskPool& TaskPool::instance()
{
    static TaskPool gs;
    return gs;
}

void TaskPool::run()
{
    for(auto& thd: threads_)
    {
        thd=std::thread([this]()
            {
                service_.run();
            }
        );
    }

    stared_=true;
}

void TaskPool::stop()
{
    if(stared_==false)
        return;

    auto& inst=instance();
    inst.work_.reset();
    for(auto& thd: inst.threads_)
    {
        if(thd.joinable())
            thd.join();
    }
}

bool TaskPool::stared_=false;



void CmdBase::help(std::ostream& strm)
{
    strm << opt_ << std::endl;
    for(auto& c: components_)
        c->shortHelp(strm);
}

void CmdBase::parse(int ac, const char* const* av)
{
    bp::options_description optComponents;
    for(auto& c: components_)
        c->options(optComponents, optPos_);

    bp::command_line_parser parser(ac, av);
    if(optComponents.options().empty())
    {
        parser.options(opt_);
    } else {
        optAll_.add(opt_);
        optAll_.add(optComponents);
        parser.options(optAll_);
    }
    parser.positional(optPos_);
    parser.style(bp::command_line_style::default_style);

    bp::store(parser.run(), vm_);
    bp::notify(vm_);

    for(auto& call: afterParseCalls_)
        call(vm_);
}

MainReturn CmdBase::init(const ContextSPtr& context)
{
    context_=context;
    return MainReturn::eGood;
}

CmdBase::~CmdBase()
{}



}  // namespace ezsh

//实现命令help，option
namespace ezsh
{

class CmdHelp:public CmdBaseT<CmdHelp>
{
    typedef CmdBaseT<CmdHelp> BaseThis;
public:
    CmdHelp()
        :BaseThis("help - show help message")
    {
        opt_.add_options()
            ("long", "show long help")
            ("cmd",  bp::value<std::vector<std::string>>(), "show only this command")
        ;
        optPos_.add("cmd", -1);
    }

    static const char* nameGet()
    {
        return "help";
    }

    MainReturn doit() override
    {
        std::cerr <<
            "ezsh command [options]\n\n";

        for(const auto& u: CmdDict::dictGet())
        {
            const auto& trait=std::get<1>(u);
            const auto& cmd=trait->create();
            cmd->help(std::cerr);
            std::cerr << std::endl;
        }

        return MainReturn::eGood;
    }
};

class CmdOption:public CmdBaseT<CmdOption>
{
    typedef CmdBaseT<CmdOption> BaseThis;
public:
    CmdOption()
        :BaseThis("option - show option message")
    {
        opt_.add_options()
            ("option", bp::value<std::vector<std::string>>(), "show only option")
        ;
        optPos_.add("option", -1);
    }

    static const char* nameGet()
    {
        return "option";
    }

    MainReturn doit() override
    {
        std::cerr << "valid options list: " << std::endl;
        const auto& opts=OptionDict::dictGet();
        for(const auto& opt: opts)
            opt.second->longHelp(std::cerr);
        return MainReturn::eGood;
    }
};

namespace
{
static CmdRegisterT<CmdHelp> gsHelp;
static CmdRegisterT<CmdOption> gsOption;
}


}

