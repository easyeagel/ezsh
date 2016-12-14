//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdContext.cpp
//
//   Description:  context命令
//
//       Version:  1.0
//       Created:  2015年01月06日 11时37分57秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<boost/algorithm/string/trim.hpp>

#include"parser.hpp"
#include"option.hpp"
#include"context.hpp"

namespace ezsh
{

class CmdContext: public CmdBaseT<CmdContext>
{
    typedef CmdBaseT<CmdContext> BaseThis;
    typedef std::vector<char*> CmdLine;
public:
    CmdContext()
        :BaseThis("context - contrl current context")
    {
        opt_.add_options()
            ("set",    bp::value<std::vector<std::string>>()->multitoken(), "set context var")
            ("unset",  bp::value<std::vector<std::string>>()->multitoken(), "unset this params")
            ("setif",  bp::value<std::vector<std::string>>()->multitoken(), "set context var, if the var not exist")
            ("list",   bp::value<std::vector<std::string>>()->multitoken(), "set context list var")
            ("listif", bp::value<std::vector<std::string>>()->multitoken(), "set context list var, if the var not exist")
            ("echo",   bp::value<std::vector<std::string>>()->multitoken(), "echo this params")
            ("file",   bp::value<std::vector<std::string>>()->multitoken(), "set context var, value is from file")
            ("export", bp::value<std::vector<std::string>>()->multitoken(), "export context var up")
        ;
    }

    static const char* nameGet()
    {
        return "context";
    }

    void doit()
    {
        const auto& vm=mapGet();

        ContextVisitor visitor(*contextGet());
        auto itr=vm.find("set");
        if(itr!=vm.end())
            visitor.setDo(itr->second.as<std::vector<std::string>>());

        itr=vm.find("setif");
        if(itr!=vm.end())
            visitor.setIfDo(itr->second.as<std::vector<std::string>>());

        itr=vm.find("list");
        if(itr!=vm.end())
            visitor.setListDo(itr->second.as<std::vector<std::string>>());

        itr=vm.find("listif");
        if(itr!=vm.end())
            visitor.setIfListDo(itr->second.as<std::vector<std::string>>());

        itr=vm.find("echo");
        if(itr!=vm.end())
            visitor.echoDo(itr->second.as<std::vector<std::string>>());

        itr=vm.find("unset");
        if(itr!=vm.end())
            visitor.unsetDo(itr->second.as<std::vector<std::string>>());

        itr=vm.find("file");
        if(itr!=vm.end())
            fileDoit(itr->second.as<std::vector<std::string>>(), visitor);

        itr=vm.find("export");
        if(itr!=vm.end())
            visitor.exportDo(itr->second.as<std::vector<std::string>>());
    }

    void doDry()
    {
        BaseThis::doDry();
        doit();
    }

private:
    void fileDoit(const std::vector<std::string>& files, ContextVisitor& visitor)
    {
        std::string content;
        std::vector<std::string> contents;
        for(auto& line: files)
        {
            const auto pair=simpleSplit(line, '=');
            if(pair.first.empty() || pair.second.empty())
                continue;

            //读取文件内容
            Path path=pair.second;
            bf::ifstream strm(path.path());
            if(!strm)
            {
                contextGet()->stdErr() << "open file failed: " << path << std::endl;
                continue;
            }

            content = pair.first;
            content += '=';

            std::istreambuf_iterator<char> itr(strm);
            std::istreambuf_iterator<char> const end;
            std::copy(itr, end, std::back_inserter(content));

            contents.emplace_back(content);
        }

        visitor.setDo(contents);
    }
};

namespace
{
    static CmdRegisterT<CmdContext> gsCmdRegister;
}

}  // namespace ezsh

