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

#include"optionOutPut.hpp"

namespace ezsh
{
void OutPut::Component::options(bp::options_description& opt, bp::positional_options_description& )
{
    opt.add_options()
        ("outTree",    "keep tree")

        ("outInplace",    "inplace dir")
        ("outDir",        bp::value<std::string>(), "all out place a dir")
        ("outDirInto",    bp::value<std::string>(), "all out into a dir")
        ("outFile",       bp::value<std::string>(), "all out to file")

        ("outExtPop",     "extension will remove")
        ("outExtAdd",     bp::value<std::string>(), "extension will add")
        ("outExtReplace", bp::value<std::string>(), "extension will replace")
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

    modeOnly_.add({"outDir", "outDirInto", "outFile", "outInplace"});
    modeOnly_.doit(cmd);
}

void OutPut::modeInit(const std::string& str, const bp::variables_map& vm)
{
    bool dirCreate=false;
    if(str=="outInplace")
    {
        mode_=eModeInplace;
        tree_=true;
    } else if(str=="outDir") {
        mode_=eModeDir;
        dirCreate=true;
    }else if(str=="outDirInto") {
        mode_=eModeDirInto;
        tree_=true;
        dirCreate=true;
    } else if(str=="outFile") {
        mode_=eModeFile;
    }

    auto itr=vm.find(str);
    if(itr!=vm.end())
        modeStr_=itr->second.as<std::string>();

    if(dirCreate)
    {
        Path dir(modeStr_);
        if(!dir.empty() && !bf::exists(dir.path()))
            bf::create_directories(dir.path());
    }
}

void OutPut::init(const bp::variables_map& vm)
{
    modeInit(modeOnly_.oneGet(), vm);

    extPop_=(vm.count("outExtPop")>0);

    auto itr=vm.find("outExtAdd");
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

    if(vm.count("outTree")>=1)
        tree_=true;
}

void OutPut::rewrite(const FileUnit& src, FileUnit& dest, bool dirCreate) const
{
    Path self=src.self;
    if(!extReplace_.empty())
        self.replace_extension(extReplace_);

    if(extPop_)
        self=self.stem();

    if(!extAdd_.empty())
        self += Path(extAdd_);

    if(tree_==false)
        self=self.filename();

    switch(mode_)
    {
        case eModeInplace:
            dest=FileUnit(self, src.base);
            break;
        case eModeDir:
            dest=FileUnit(self, modeStr_);
            break;
        case eModeDirInto:
        {
            if(src.base.empty())
                dest=FileUnit(self, modeStr_);
            else
                dest=FileUnit(src.base.filename()/self, modeStr_);
            break;
        }
        case eModeFile:
            dest=FileUnit(modeStr_);
            break;
    }

    if(dirCreate==true)
    {
        const auto& p=dest.total.parent_path();
        if(!p.empty() && !bf::exists(p))
            bf::create_directories(p);
    }

}

namespace
{
    static ezsh::OptionRegisterT<ezsh::OutPut> gsOptionOutPut;
}

}

