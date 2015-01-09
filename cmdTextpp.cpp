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
#include"output.hpp"
#include"fileset.hpp"
#include<boost/algorithm/string.hpp>
#include<boost/xpressive/xpressive.hpp>

namespace ezsh
{

namespace xpr
{
    using namespace boost::xpressive;
    
    static sregex gsMacroName  = (alpha|'_') >> *(alnum|'_');
    static sregex gsNotComma   =~(set=',','}');
    static sregex gsNotCommaStr=+gsNotComma;
    static sregex gsTextpp=as_xpr("${")
        >> *blank >> gsNotCommaStr
        >> *(*blank >> ',' >> *blank >> gsNotCommaStr)
        >> *blank >> '}';
}

class CmdTextpp:public CmdBaseT<CmdTextpp, FileSetCmdBase<OutPutCmdBase<>>>
{
    typedef CmdBaseT<CmdTextpp, FileSetCmdBase> BaseThis;
    typedef std::ostreambuf_iterator<char> OutItr;
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
        std::string out;
        auto itr=vm.find("output");
        if(itr!=vm.end())
            out=itr->second.as<std::string>();

        auto& files=fileGet();
        files.init(vm);

        auto& output=outGet();
        output.init(vm);

        FileUnit outPath;
        for(const auto& file: files.setGet())
        {
            if(!file.isExist() || file.isDir())
            {
                if(force)
                    continue;
                stdErr() << file.total << ": not exist or is dir" << std::endl;
                continue;
            }

            bf::ifstream strm(file.total.path());
            if(!strm)
                continue;

            output.rewrite(file, outPath);
            if(outPath.total.empty())
            {
                fileOne(strm, OutItr(std::cout));
            } else {
                bf::ofstream ostrm(outPath.total.path());
                if(!ostrm)
                    continue;
                fileOne(strm, OutItr(ostrm));
            }
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

                if(!xpr::regex_match(key.begin(), key.end(), xpr::gsMacroName))
                {
                    stdErr() << "warning:notInvalidMacroName: " << key << std::endl;
                    continue;
                }

                dict_[key]=val;
            }
        }
    }

    OutItr fileOne(std::istream& strm, OutItr out) const
    {
        std::string in;
        std::istreambuf_iterator<char> itr(strm);
        std::istreambuf_iterator<char> const end;
        std::copy(itr, end, std::back_inserter(in));

        xpr::regex_replace(out, in.begin(), in.end(), xpr::gsTextpp,
            [this](const xpr::smatch& what, std::ostreambuf_iterator<char>& out) -> OutItr
            {
                auto begin = what.nested_results().begin();
                auto end   = what.nested_results().end();

                xpr::sregex_id_filter_predicate name(xpr::gsNotCommaStr.regex_id());

                std::vector<std::string> result;
                std::for_each(
                    boost::make_filter_iterator(name, begin, end),
                    boost::make_filter_iterator(name, end, end),
                    [&result](const xpr::smatch& m)
                    {
                        result.push_back(m.str());
                    }
                );

                return matchOne(what, result, out);
            }
        );

        return out;
    }

    OutItr matchOne(const xpr::smatch& what, const std::vector<std::string>& match, OutItr out) const
    {
        if(match.size()==1) //简单宏替换
        {
            auto const itr=dict_.find(match.front());
            if(itr!=dict_.end())
            {
                auto const& s=itr->second;
                std::copy(s.begin(), s.end(), out);
                return out;
            }
        } else if(match[1]=="include") {
            Path path=simpleReplace(match[0]);
            bf::ifstream strm(path.path());
            if(strm)
                return fileOne(strm, out);
        }

        const auto& s=what.str();
        std::copy(s.begin(), s.end(), out);
        return out;
    }

    std::string simpleReplace(const std::string& str) const
    {
        std::string out;
        xpr::regex_replace(std::back_inserter(out), str.begin(), str.end(), xpr::gsMacroName,
            [this](const xpr::smatch& what) -> std::string
            {
                auto const itr=dict_.find(what.str());
                if(itr==dict_.end())
                    return what.str();
                return itr->second;
            }
        );
        return std::move(out);
    }

private:
    std::map<std::string, std::string> dict_;
};

namespace
{
static CmdRegisterT<CmdTextpp> gsRegister;
}


}

