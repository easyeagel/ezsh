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

}  // namespace ezsh

