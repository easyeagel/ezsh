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

#include"option.hpp"
#include"fileset.hpp"

namespace ezsh
{

class CmdRemove:public CmdBaseT<CmdRemove, FileSetCmdBase<>>
{
    typedef CmdBaseT<CmdRemove, FileSetCmdBase> BaseThis;
public:
    CmdRemove()
        :BaseThis("remove - remove file or dir")
    {
        opt_.add_options()
            ("force,f", "ignore nonexistent files and arguments")
        ;
    }

    static const char* nameGet()
    {
        return "remove";
    }

    MainReturn doit()
    {
        auto& files=fileGet();
        const auto& vm=mapGet();

        files.init(vm);
        files.scan();
        const bool force=vm.count("force") ? true : false;
        for(const auto& file: files.setGet())
            fileRemove(file, force);
        return MainReturn::eGood;
    }

private:
    void fileRemove(const FileUnit& file, bool force)
    {
        if(!file.isExist())
        {
            if(!force)
                stdErr() << file.total << ": not exist" << std::endl;
            return;
        }

        boost::system::error_code ec;
        if(!file.isDir())
        {
            bf::remove(file.total, ec);
            if(ec && !force)
                stdErr() << file.total << ": " << ec.message() << std::endl;
            return;
        }

        if(fileGet().isRecursive())
        {
            bf::remove_all(file.total, ec);
            if(ec && !force)
                stdErr() << file.total << ": " << ec.message() << std::endl;
            return;
        }

        bf::remove(file.total, ec);
        if(ec && !force)
            stdErr() << file.total << ": " << ec.message() << std::endl;
    }
};

namespace
{
static CmdRegisterT<CmdRemove> gsRegister;
}


}

