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



void OptBase::help(std::ostream& strm)
{
    strm << opt_ << std::endl;
    for(auto& c: components_)
        c->shortHelp(strm);
}

void OptBase::parse(int ac, const char* const* av)
{
    for(auto& c: components_)
        c->options(optComponents_, optPos_);

    bp::command_line_parser parser(ac, av);
    parser.options(opt_);
    parser.options(optComponents_);
    parser.positional(optPos_);
    parser.style(bp::command_line_style::default_style);

    bp::store(parser.run(), vm_);
    bp::notify(vm_);
}

MainReturn OptBase::init(const ContextSPtr& context)
{
    context_=context;
    return MainReturn::eGood;
}

OptBase::~OptBase()
{}



}  // namespace ezsh

