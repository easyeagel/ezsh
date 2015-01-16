﻿//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  output.cpp
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

#include"output.hpp"

namespace ezsh
{
void OutPut::Component::options(bp::options_description& opt, bp::positional_options_description& )
{
    opt.add_options()
        ("outDir",        bp::value<std::string>(), "all out place a dir")
        ("outExtAdd",     bp::value<std::string>(), "extension will add")
        ("outExtReplace", bp::value<std::string>(), "extension will replace")
        ("outFile",       bp::value<std::string>(), "all out to file")

        ("outTree",    "keep tree")
        ("outExtPop",  "extension will remove")
        ("outInplace", "inplace dir")
    ;
}

void OutPut::Component::longHelp (std::ostream& strm)
{
    bp::options_description opt;
    bp::positional_options_description pos;
    options(opt, pos);
    strm << "output - rules for output\n";
    strm << opt << std::endl;
}

void OutPut::Component::shortHelp(std::ostream& strm)
{
    strm << "  *ouput - rules for output, \"option output\" for details" << std::endl;
}

void OutPut::config(CmdBase& cmd)
{
    cmd.componentPush(componentGet());
}

void OutPut::init(const bp::variables_map& vm)
{
    inplace_=(vm.count("inplace")>0);

    auto itr=vm.find("outDir");
    if(itr!=vm.end())
        outDir_=itr->second.as<std::string>();

    itr=vm.find("outExtAdd");
    if(itr!=vm.end())
    {
        const auto t=itr->second.as<std::string>();
        if(t.empty() || t.front()!='.')
            extAdd_ += '.';
        extAdd_ += t;
    }

    itr=vm.find("outExtReplace");
    if(itr!=vm.end())
        extReplace_=itr->second.as<std::string>();

    itr=vm.find("outFile");
    if(itr!=vm.end())
        outFile_=itr->second.as<std::string>();

    if(outDir_.empty() || vm.count("outTree")>=1)
        tree_=true;

    extPop_=(vm.count("outExtPop")>0);

    Path dir(outDir_);
    if(!dir.empty() && !bf::exists(dir.path()))
        bf::create_directories(dir.path());
}

void OutPut::rewrite(const FileUnit& src, FileUnit& dest) const
{
    if(!outFile_.empty())
    {
        dest=FileUnit(outFile_);
        return;
    }

    Path self=src.self;
    if(!extReplace_.empty())
        self.replace_extension(extReplace_);

    if(extPop_)
        self=self.stem();

    if(!extAdd_.empty())
        self += Path(extAdd_);

    if(tree_==false)
        self=self.filename();

    if(!outDir_.empty())
        dest=FileUnit(self, outDir_);
    else if(inplace_)
        dest=FileUnit(self, src.base);
    else
        dest=FileUnit(self);

    if(tree_)
    {
        const auto& p=dest.total.parent_path();
        if(!p.empty() && !bf::exists(p))
            bf::create_directories(p);
    }
}

}

