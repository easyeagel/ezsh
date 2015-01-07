//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdTextpp.cpp
//
//   Description:  text Preprocessor，文本预处理器
//
//       Version:  1.0
//       Created:  2015年01月07日 16时10分50秒
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
#include<boost/algorithm/string.hpp>
#include<boost/xpressive/xpressive.hpp>

namespace ezsh
{

namespace xpr
{
    using namespace boost::xpressive;
    
    static sregex gsNotComma   =~as_xpr(',');
    static sregex gsNotCommaStr=+~as_xpr(',');
    static sregex gsTextpp=as_xpr("${")
        >> *blank >> gsNotCommaStr
        >> *(*blank >> ',' >> *blank >> gsNotCommaStr)
        >> *blank >> '}';
}

class CmdTextpp:public CmdBaseT<CmdTextpp, FileSetCmdBase<>>
{
    typedef CmdBaseT<CmdTextpp, FileSetCmdBase> BaseThis;
public:
    CmdTextpp()
        :BaseThis("textpp - textpp file or dir")
    {
        opt_.add_options()
            ("force,f",  "ignore nonexistent files and arguments")
            ("define,D", bp::value<std::vector<std::string>>(), "define a macro")
        ;
    }

    static const char* nameGet()
    {
        return "textpp";
    }

    MainReturn doit() override
    {
        const auto& vm=mapGet();

        optionDoit();
        const bool force=vm.count("force") ? true : false;

        auto& files=fileGet();
        files.init(vm);

        for(const auto& file: files.setGet())
        {
            if(!file.isExist() || file.isDir())
            {
                if(force)
                    continue;
                stdErr() << file.total << ": not exist or is dir" << std::endl;
                continue;
            }

            fileOne(file);
        }

        return MainReturn::eGood;
    }

private:
    void optionDoit()
    {
        const auto& vm=mapGet();
        auto itr=vm.find("define");
        if(itr!=vm.end())
        {
            const auto& macros=itr->second.as<std::vector<std::string>>();
            for(const auto& m: macros)
            {
                std::string key, val;
                auto eq=m.find('=');
                if(eq==std::string::npos)
                {
                    key=m;
                } else {
                    key=m.substr(0, eq);
                    val=m.substr(eq+1);
                }
                boost::trim(key);
                boost::trim(val);
                dict_[key]=val;
            }
        }
    }

    void fileOne(const FileUnit& file) const
    {
        bf::ifstream strm(file.total);
        if(!strm)
            return;

        std::string in;
        std::istreambuf_iterator<char> itr(strm);
        std::istreambuf_iterator<char> const end;
        std::copy(itr, end, std::back_inserter(in));

        std::string out;
        xpr::regex_replace(std::back_inserter(out), in.begin(), in.end(), xpr::gsTextpp,
            [this](const xpr::smatch& what) -> std::string
            {
                auto begin = what.nested_results().begin();
                auto end   = what.nested_results().end();

                xpr::sregex_id_filter_predicate name( xpr::gsNotCommaStr.regex_id() );

                std::vector<std::string> result;
                std::for_each(
                    boost::make_filter_iterator(name, begin, end),
                    boost::make_filter_iterator(name, end, end),
                    [&result](const xpr::smatch& m)
                    {
                        result.push_back(m.str());
                    }
                );

                return matchOne(what, result);
            }
        );

        stdOut() << out;
    }

    std::string matchOne(const xpr::smatch& what, const std::vector<std::string>& match) const
    {
        if(match.size()==1) //简单宏替换
        {
            auto const itr=dict_.find(match.front());
            if(itr==dict_.end())
                return what.str();
            return itr->second;
        }

        const auto& opt=match[1];
        if(opt=="include")
        {
            bf::ifstream strm(match[0]);
            if(!strm)
                return what.str();
            std::string in;
            std::istreambuf_iterator<char> itr(strm);
            std::istreambuf_iterator<char> const end;
            std::copy(itr, end, std::back_inserter(in));
            return std::move(in);
        }

        return what.str();
    }

private:
    std::map<std::string, std::string> dict_;
};

namespace
{
static CmdRegisterT<CmdTextpp> gsRegister;
}


}

