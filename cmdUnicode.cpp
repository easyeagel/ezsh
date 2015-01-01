//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdUnicode.cpp
//
//   Description:  Unicode 实现工具
//
//       Version:  1.0
//       Created:  2014年12月31日 14时40分42秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<utf8.h>
#include<boost/filesystem/fstream.hpp>

#include"encode.hpp"
#include"option.hpp"
#include"fileset.hpp"

namespace ezsh
{

namespace bf=boost::filesystem;

class CmdUnicode:public CmdBaseT<CmdUnicode>
{
    typedef CmdBaseT<CmdUnicode> BaseThis;
public:
    CmdUnicode()
        :BaseThis("unicode - unicode file or dir")
    {
        opt_.add_options()
            ("bomAdd",                              "add BOM in file header")
            ("bomRemove",                           "remove BOM in file header")
            ("valid",     bp::value<std::string>(), "check files encode: utf8, utf16, utf32")
        ;

        components_.push_back(FileSet::componentGet());
    }

    static const char* nameGet()
    {
        return "unicode";
    }

    MainReturn doit() override
    {
        const auto& vm=mapGet();

        files_.init(vm);

        //const bool valid=vm.count("valid") ? true : false;
        for(const auto& file: files_.setGet())
            one(file);

        files_.scan([this](FileSet::FileUnit&& fu)
            {
                one(fu);
            }
        );

        return MainReturn::eGood;
    }

private:
    void one(const FileSet::FileUnit& file)
    {
        if(!file.isExist())
        {
            context_->stdCErr() << WCharConverter::to(file.total.string()) << ": notExist" << std::endl;
            return;
        }

        boost::system::error_code ec;
        if(file.isDir())
            return;

        bf::ifstream strm(file.total);
        if(!strm)
            return;

        const auto isValid=utf8::is_valid(std::istreambuf_iterator<char>(strm), std::istreambuf_iterator<char>());
        context_->stdCOut()
            << WCharConverter::to(file.total.string())
            << (isValid ? ": valid" : ": invalid") << std::endl;
    }

private:
    FileSet files_;
};

namespace
{
static CmdRegisterT<CmdUnicode> gsRegister;
}


}

