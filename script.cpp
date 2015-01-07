//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  script.cpp
//
//   Description:  脚本基本组件
//
//       Version:  1.0
//       Created:  2015年01月06日 17时15分37秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include"script.hpp"
#include<boost/algorithm/string.hpp>

namespace ezsh
{

void ScriptCommand::cmdlineReplace(const ContextSPtr& context, const StrCommandLine& cmd, StrCommandLine& dest)
{
    const auto sz=cmd.size();
    if(dest.size()!=sz)
        dest.resize(sz);

    for(size_t i=0; i<sz; ++i)
    {
        const auto& s=cmd[i];
        auto& d=dest[i];
        d.clear();
        context->replace(s, d);
    }
}

MainReturn ScriptCommand::execute(const ContextSPtr& context)
{
    const auto& cmd=traitGet()->create();
    return execute(context, *cmd);
}

MainReturn ScriptCommand::execute(const ContextSPtr& context, CmdBase& cmd)
{
    CommandLine args;
    StrCommandLine strArgs;
    cmdlineReplace(context, cmdlineGet(), strArgs);
    std::for_each(strArgs.begin(), strArgs.end(),
        [&args](const std::string& s)
        {
            args.push_back(const_cast<char*>(s.data()));
        }
    );

    try
    {
        cmd.parse(args.size(), const_cast<char**>(args.data()));
    } catch (const boost::program_options::error& ec) {
        std::cerr << ec.what() << std::endl;
        return MainReturn::eParamInvalid;
    }

    auto ret=cmd.init(context);
    if(ret.good())
        ret=cmd.doit();
    return ret;
}

class ScriptLoad
{
public:
    ScriptLoad()=default;
    ScriptLoad(const std::string& end)
        :end_(end)
    {}

    bool load(std::istream& strm, Script& spt)
    {
        for(;;)
        {
            if(firstLine_)
            {
                firstLine_=false;
                if(line_.empty())
                    std::getline(strm, line_);
            } else {
                std::getline(strm, line_);
            }

            if(!strm)
            {
                if(unit_.empty())
                    break;
                return unitDoit(strm, spt);
            }

            boost::trim(line_);
            if(line_.empty() || line_[0]=='#')
            {
                continue;
            } else if(line_[0]=='-') {
                unit_.lineAppend(line_);
                continue;
            } else if(unit_.empty()) {
                unit_.lineAppend(line_);
                continue;
            } else {
                if(unitDoit(strm, spt)==false)
                    return false;
                if(toBreak_)
                    break;
            }
        }

        return true;
    }

    bool unitDoit(std::istream& strm, Script& spt)
    {
        if(unit_.tokenize()==false)
            return false;

        auto trait=CmdDict::find(unit_.cmdGet());
        if(trait!=nullptr)
        {
            unit_.traitSet(trait);

            spt.push(std::move(unit_));
			unit_.reset(); //MSVC 没有正确实现移到构造

            //或达到文件结尾
            if(!strm)
                toBreak_=true;

            unit_.lineAppend(line_);
            return true;
        }

        //找到结束命令
        if(unit_.cmdGet()==end_)
        {
            toBreak_=true;
            return true;
        }

        return groupLoad(strm, spt);
    }

    const std::string& lastLineGet() const
    {
        return line_;
    }

    bool groupLoad(std::istream& strm, Script& spt)
    {
        const auto& gd=CommandGroupDict::instance();
        auto const u=gd.find(unit_.cmdGet());
        if(u==nullptr)
            return false;

        Script group;
        ScriptLoad sl(u->end);
        sl.line_=std::move(line_);
        if(sl.load(strm, group)==false)
            return false;
        spt.push(u->create(std::move(unit_), std::move(group), std::move(sl.unit_)));
        return true;
    }

private:
    bool firstLine_=true;
    bool toBreak_=false;
    std::string end_;
    std::string line_;
    ScriptCommand unit_;
};


bool Script::load(std::istream& strm, Script& spt)
{
    ScriptLoad sl;
    return sl.load(strm, spt);
}

MainReturn Script::execute(const ContextSPtr& context)
{
    for(auto& ptr: script_)
    {
        auto const sc=boost::get<ScriptCommand>(&ptr);
        if(sc!=nullptr)
        {
            const auto ret=sc->execute(context);
            if(ret.bad())
                return ret;
            continue;
        }

        auto const cg=boost::get<CommandGroupBaseSPtr>(&ptr);
        if(cg!=nullptr)
        {
            const auto ret=(*cg)->execute(context);
            if(ret.bad())
                return ret;
            continue;
        }
    }

    return MainReturn::eGood;
}


}

