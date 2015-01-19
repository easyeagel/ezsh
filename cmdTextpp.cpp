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

#include<boost/algorithm/string.hpp>
#include<boost/xpressive/xpressive.hpp>

#include"option.hpp"
#include"output.hpp"
#include"parser.hpp"
#include"fileset.hpp"

namespace ezsh
{

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

    void doit()
    {
        const auto& vm=mapGet();

        optionDoit();
        const bool force=vm.count("force") ? true : false;
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
                OutItr out(std::cout);
                fileOne(strm, out);
            } else {
                bf::ofstream ostrm(outPath.total.path());
                if(!ostrm)
                    continue;
                OutItr out(ostrm);
                fileOne(strm, out);
            }
        }
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

    void fileOne(std::istream& strm, OutItr& out) const
    {
        ErrorCode ec;
        std::istreambuf_iterator<char> itr(strm);
        std::istreambuf_iterator<char> const end;
        out=ReplacePattern::replace(ec, out, itr, end,
            [this](ErrorCode& ec, OutItr out, const ReplacePattern& what) -> OutItr
            {
                matchOne(what, out);
                return out;
            }
        );
    }

    void matchOne(const ReplacePattern& match, OutItr& out) const
    {
        for(const auto& opt: match.operatorsGet())
        {
            if(opt.name=="replace")
            {
                macroReplace(opt, out);
                continue;
            }

            if(opt.name=="include")
            {
                includeReplace(opt, out);
                continue;
            }
        }
    }

    void macroReplace(const ReplacePattern::Operator& opt, OutItr& out) const
    {
        for(const auto& param: opt.params)
        {
            auto const itr=dict_.find(param.value);
            if(itr!=dict_.end())
            {
                auto const& s=itr->second;
                std::copy(s.begin(), s.end(), out);
                continue;
            }
        }
    }

    void includeReplace(const ReplacePattern::Operator& opt, OutItr& out) const
    {
        for(const auto& param: opt.params)
        {
            Path path=simpleReplace(param.value);
            bf::ifstream strm(path.path());
            if(!strm)
                continue;
            fileOne(strm, out);
        }
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

