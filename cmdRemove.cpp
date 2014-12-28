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

namespace ezsh
{

class CmdRemove:public OptBaseT<CmdRemove>
{
    typedef OptBaseT<CmdRemove> BaseThis;
public:
    CmdRemove()
        :BaseThis("remove - remove file or dir")
    {
        opt_.add_options()
            ("force,f",     "ignore nonexistent files and arguments")
            ("recursive,r", "remove dir recursively")
            ("file", bp::value<std::vector<std::string>>()->required(), "file or dir to remove")
        ;
        optPos_.add("file", -1);
    }

    static const char* nameGet()
    {
        return "remove";
    }

    MainReturn doit() override
    {
        namespace bf=boost::filesystem;

        const auto& vm=mapGet();
        const bool force=vm.count("force") ? true : false;
        const bool recursive=vm.count("recursive") ? true : false;
        const auto& files=vm["file"].as<std::vector<std::string>>();
        for(const auto& file: files)
        {
            const bool exist=bf::exists(file);
            if(exist==false)
            {
                if(force)
                    continue;
                context_->stdCErr() << file << ": not exist" << std::endl;
                continue;
            }

            boost::system::error_code ec;
            const bool isDir=bf::is_directory(file);
            if(isDir==false)
            {
                bf::remove(file, ec);
                if(ec && !force)
                    context_->stdCErr() << file << ": " << ec.message() << std::endl;
                continue;
            }

            if(recursive)
            {
                bf::remove_all(file, ec);
                if(ec && !force)
                    context_->stdCErr() << file << ": " << ec.message() << std::endl;
                continue;
            }

            bf::remove(file, ec);
            if(ec && !force)
                context_->stdCErr() << file << ": " << ec.message() << std::endl;
        }

        return MainReturn::eGood;
    }

};

namespace
{
static OptRegisterT<CmdRemove> gsRegister;
}


}

