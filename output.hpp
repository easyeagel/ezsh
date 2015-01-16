//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  output.hpp
//
//   Description:  输出控制
//
//       Version:  1.0
//       Created:  2015年01月08日 15时17分38秒
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
#include"fileset.hpp"
#include"filesystem.hpp"

namespace ezsh
{

class OutPut
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
        return "output";
    }

    void rewrite(const FileUnit& src, FileUnit& dest) const;

private:
    void afterParse(const bp::variables_map& vm);

private:
    bool tree_=false;
    bool extPop_=false;
    bool inplace_=false;
    std::string extReplace_;
    std::string extAdd_;
    std::string outDir_;
    std::string outFile_;
};

template<typename Base=CmdBase>
class OutPutCmdBase: public Base
{
public:
    OutPutCmdBase(const char* msg)
        :Base(msg)
    {
        output_.config(*this);
    }

    OutPut& outGet()
    {
        return output_;
    }

    const OutPut& outGet() const
    {
        return output_;
    }

private:
    OutPut output_;
};

}




