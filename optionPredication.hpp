//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  optionPredication.hpp
//
//   Description:  前置断言
//
//       Version:  1.0
//       Created:  2015年01月29日 09时51分14秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include"option.hpp"

namespace ezsh
{

class Predication
{
    class Component: public OptionComponent
    {
    public:
        void longHelp (std::ostream& strm) override;
        void shortHelp(std::ostream& strm) override;
        void options(bp::options_description& opt, bp::positional_options_description& pos) override;
    };

public:
    static OptionComponentSPtr componentGet()
    {
        static OptionComponentSPtr ptr;
        if(!ptr)
            ptr.reset(new Component);
        return ptr;
    }

    void config(CmdBase& cmd);
    void init(const bp::variables_map& dict);

    static const char* nameGet()
    {
        return "predication";
    }

    bool isPassed() const
    {
        return isPassed_;
    }

private:
    bool isPassed_=false;
};

template<typename Base=CmdBase>
class PredicationCmdBase: public Base
{
public:
    PredicationCmdBase(const char* msg)
        :Base(msg)
    {
        predications_.config(*this);
    }

    bool isPassed() const
    {
        return predications_.isPassed();
    }

protected:
    void predicationInit(const bp::variables_map& vm)
    {
        predications_.init(vm);
    }

private:
    Predication predications_;
};

}


