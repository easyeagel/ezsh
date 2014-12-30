//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdRemove.cpp
//
//   Description:  删除命令
//
//       Version:  1.0
//       Created:  2014年12月25日 10时44分35秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<boost/filesystem.hpp>

#include"option.hpp"
#include"fileset.hpp"

namespace ezsh
{

namespace bf=boost::filesystem;

class CmdRemove:public CmdBaseT<CmdRemove>
{
    typedef CmdBaseT<CmdRemove> BaseThis;
public:
    CmdRemove()
        :BaseThis("remove - remove file or dir")
    {
        opt_.add_options()
            ("force,f", "ignore nonexistent files and arguments")
        ;

        components_.push_back(FileSet::componentGet());
    }

    static const char* nameGet()
    {
        return "remove";
    }

    MainReturn doit() override
    {
        const auto& vm=mapGet();

        files_.init(vm);

        const bool force=vm.count("force") ? true : false;
        for(const auto& file: files_.setGet())
        {
            if(!file.isExist())
            {
                if(force)
                    continue;
                context_->stdCErr() << file.total << ": not exist" << std::endl;
                continue;
            }

            boost::system::error_code ec;
            if(!file.isDir())
            {
                bf::remove(file.total, ec);
                if(ec && !force)
                    context_->stdCErr() << file.total << ": " << ec.message() << std::endl;
                continue;
            }

            if(files_.isRecursive())
            {
                bf::remove_all(file.total, ec);
                if(ec && !force)
                    context_->stdCErr() << file.total << ": " << ec.message() << std::endl;
                continue;
            }

            bf::remove(file.total, ec);
            if(ec && !force)
                context_->stdCErr() << file.total << ": " << ec.message() << std::endl;
        }

        return MainReturn::eGood;
    }

private:
    FileSet files_;
};

namespace
{
static CmdRegisterT<CmdRemove> gsRegister;
}


}

