//  Copyright [2015] <lgb (LiuGuangBao)>
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
        ("outTree",   "keep tree")
        ("outExtPop", "extension will remove")
        ("outDir",    bp::value<std::string>(), "all out place a dir")
        ("outExtAdd", bp::value<std::string>(), "extension will add")
        ("outFile",   bp::value<std::string>(), "all out to file")
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
    strm << "  ouput - rules for output, \"option output\" for details\n\n" << std::endl;
}

void OutPut::config(CmdBase& cmd)
{
    cmd.componentPush(componentGet());
}

void OutPut::init(const bp::variables_map& vm)
{
    auto itr=vm.find("outDir");
    if(itr!=vm.end())
        outDir_=itr->second.as<std::string>();

    itr=vm.find("outExtAdd");
    if(itr!=vm.end())
        outDir_="."+itr->second.as<std::string>();

    itr=vm.find("outFile");
    if(itr!=vm.end())
        outFile_=itr->second.as<std::string>();

    if(outDir_.empty())
        tree_=true;

    if(vm.count("outTree")>=1)
        tree_=true;

    extPop_=vm.count("outExtPop")>=1 ? true : false;

    Path dir(outDir_);
    if(!dir.empty() && !bf::exists(dir.path()))
        bf::create_directories(dir.path());
}

void OutPut::rewrite(const FileUnit& src, FileUnit& dest)
{
    if(!outFile_.empty())
    {
        dest=FileUnit(outFile_);
        return;
    }

    Path self=extPop_ ? Path(src.self.stem()) : src.self;
    if(!extAdd_.empty())
        self += Path(extAdd_);
    if(tree_==false)
        self=self.filename();
    if(outDir_.empty())
        dest=FileUnit(self);
    else
        dest=FileUnit(self, outDir_);
}



}



