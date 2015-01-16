//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  fileset.cpp
//
//   Description:  文件集合操作
//
//       Version:  1.0
//       Created:  2014年12月29日 16时00分33秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include"fileset.hpp"

#include<cctype>

#include<deque>
#include<locale>
#include<algorithm>

#include"glob.hpp"
#include"encode.hpp"

namespace ezsh
{

FileUnit::FileUnit(const Path& selfIn, const Path& baseIn, bool scanedIn)
    :scaned(scanedIn), self(selfIn), base(baseIn), total(normalize(baseIn/selfIn))
{
    refresh();
}

void FileUnit::refresh()
{
    boost::system::error_code ec;
    status=bf::status(total, ec);
    if(status.type()==bf::file_type::file_not_found)
        return;

    ec.clear();
    size=bf::file_size(total, ec);
    if(size==static_cast<uint64_t>(-1) || ec)
        size=0;
    ctime=bf::last_write_time(total);
}

Path FileUnit::normalize(const Path& path)
{
    if(path.empty())
        return path;

    //删除路径中多余的 . ..
    std::deque<Path> stk;
    for(auto itr=path.begin(), end=path.end(); itr!=end; ++itr)
    {
        if(*itr==".")
            continue;

        if(*itr==".." && !stk.empty() && stk.back()!="..")
        {
            stk.pop_back();
            continue;
        }

        stk.push_back(*itr);
    }

    Path ret;
    for(auto& p: stk)
        ret /= p;

    if(ret.empty())
        return ".";
    return ret;
}

Path FileUnit::sub(const Path& path, const Path& base)
{
    auto count=std::distance(base.begin(), base.end());
    auto itr=path.begin(), end=path.end();
    while(count-- && itr!=end)
        ++itr;

    Path ret;
    for(; itr!=end; ++itr)
        ret /= *itr;
    if(ret.empty())
        return Path(".");
    return ret;
}

static uint64_t sizeRead(const std::string& input)
{
    uint64_t const K=1024, M=1024*1024, G=1024*1024*1024;

    auto msg=input.c_str();

    const char* str=nullptr;
    unsigned n=0, k=0, m=0, g=0;
    for(; *msg; msg += 1)
    {
        if(*msg>='0' && *msg<='9')
        {
            if(str==nullptr)
                str=msg;
            continue;
        } else if(std::isspace(*msg)) {
            continue;
        } else {
            if(str==nullptr)
                throw bp::error("invalid file size pattern");

            switch(*msg)
            {
                case 'G':
                case 'g':
                    g=std::strtoul(str, nullptr, 10);
                    break;
                case 'M':
                case 'm':
                    m=std::strtoul(str, nullptr, 10);
                    break;
                case 'K':
                case 'k':
                    k=std::strtoul(str, nullptr, 10);
                    break;
            }

            str=nullptr;
        }
    }

    if(str!=nullptr)
        n=std::strtoul(str, nullptr, 0);

    return g*G + m*M + k*K + n;
}

void FileSet::sizeEqual(const std::vector<std::string>& sizes, bool n)
{
    std::vector<uint64_t> sz;
    for(auto s: sizes)
        sz.push_back(sizeRead(s));

    predications_.emplace_back([sz, n](const FileUnit& u)
        {
            return std::any_of(sz.begin(), sz.end(), [&u, n](uint64_t s){ return n ? s!=u.size : s==u.size; });
        }
    );
}

void FileSet::afterParse(const bp::variables_map& vm)
{
    auto itr=vm.find("fsSizeEqual");
    if(itr!=vm.end())
        sizeEqual(itr->second.as<std::vector<std::string>>(), false);

    itr=vm.find("fsSizeNotEqual");
    if(itr!=vm.end())
        sizeEqual(itr->second.as<std::vector<std::string>>(), true);

#define MD(CppParam, CppOpt) \
    itr=vm.find(#CppParam); \
    if(itr!=vm.end()) \
    { \
        const auto sz=sizeRead(itr->second.as<std::string>()); \
        predications_.emplace_back([sz](const FileUnit& u) \
            { \
                return u.size CppOpt sz; \
            } \
        ); \
    }

    MD(fsSizeLess,         <  )
    MD(fsSizeGreater,      >  )
    MD(fsSizeLessEqual,    <= )
    MD(fsSizeGreaterEqual, >= )

#undef MD
}

void FileSet::Component::options(bp::options_description& opt, bp::positional_options_description& pos)
{
    opt.add_options()
        ("fsFile",              bp::value<std::vector<std::string>>(), "files to set")
        ("fsGlob",              bp::value<std::vector<std::string>>(), "glob parttern include in set")
        ("fsGlobNot",           bp::value<std::vector<std::string>>(), "glob parttern not include in set")
        ("fsInclude",           bp::value<std::vector<std::string>>(), "regex parttern included in set")
        ("fsExclude",           bp::value<std::vector<std::string>>(), "regex parttern excluded from set")

        ("fsSizeEqual",         bp::value<std::vector<std::string>>(), "size equal to, may many times")
        ("fsSizeNotEqual",      bp::value<std::vector<std::string>>(), "size not equal to, may many times")

        ("fsSizeLess",          bp::value<std::string>(), "size less than")
        ("fsSizeGreater",       bp::value<std::string>(), "size greater than")
        ("fsSizeLessEqual",     bp::value<std::string>(), "size less or equal to")
        ("fsSizeGreaterEqual",  bp::value<std::string>(), "size greater or equal to")

        ("fsRecursive", bp::value<size_t>()
                 ->default_value(0)
                 ->implicit_value(eRecursiveDefault),
             "operate recursively")
    ;
    pos.add("fsFile", -1);
}

void FileSet::Component::longHelp (std::ostream& strm)
{
    bp::options_description opt;
    bp::positional_options_description pos;
    options(opt, pos);
    strm << "fileset - select files with parttern or regex\n";
    strm << opt << std::endl;
}

void FileSet::Component::shortHelp(std::ostream& strm)
{
    strm << "  *fileset - rules for file select, \"option fileset\" for details" << std::endl;
}

void FileSet::init(const bp::variables_map& vm)
{
    recursive_=vm["fsRecursive"].as<size_t>();

    auto itr =vm.find("fsFile");
    if(itr!=vm.end())
        files_=itr->second.as<std::vector<std::string>>();

    itr =vm.find("fsGlob");
    if(itr!=vm.end())
        glob_=itr->second.as<std::vector<std::string>>();

    itr =vm.find("fsGlobNot");
    if(itr!=vm.end())
        globNot_=itr->second.as<std::vector<std::string>>();

    itr =vm.find("fsInclude");
    if(itr!=vm.end())
    {
        auto const& t=itr->second.as<std::vector<std::string>>();
        includes_.reserve(t.size());
        for(const auto& r: t )
            includes_.emplace_back(WCharConverter::from(r));
    }

    itr =vm.find("fsExclude");
    if(itr!=vm.end())
    {
        auto const& t=itr->second.as<std::vector<std::string>>();
        includes_.reserve(t.size());
        for(const auto& r: t )
            excludes_.emplace_back(WCharConverter::from(r));
    }

    for(const auto& file: files_)
    {
        FileUnit fu(file);
        if(!isRight(fu))
            continue;
        sets_.insert(std::move(fu));
    }
}

bool FileSet::isRight(const FileUnit& fu) const
{
	if (!globNot_.empty() || !glob_.empty())
	{
		const auto& file = fu.self.has_parent_path() ? fu.self.filename() : fu.self.path();
		const auto& utf8File = WCharConverter::to(file.native());

		for (const auto& g : glob_)
		{
			if (!Glob::match(g, utf8File))
				return false;
		}

		for (const auto& g : globNot_)
		{
			if (Glob::match(g, utf8File))
				return false;
		}
	}
  

    //------------------------------------------------------------------
	if (!includes_.empty() || !excludes_.empty())
	{
		const auto& wpath = WCharConverter::from(fu.self.native());

		for (const auto& reg : includes_)
		{
			if (!std::regex_search(wpath, reg))
				return false;
		}

		for (const auto& reg : excludes_)
		{
			if (std::regex_search(wpath, reg))
				return false;
		}
	}

    for(const auto& prd: predications_)
    {
        if(!prd(fu))
            return false;
    }
	
    return true;
}

void FileSet::config(CmdBase& cmd)
{
    cmd.componentPush(componentGet());
    cmd.afterParseCall(std::bind(&FileSet::afterParse, this, std::placeholders::_1));
}

namespace
{
    static ezsh::OptionRegisterT<ezsh::FileSet> gsOptionFileSet;
}
}  // namespace ezsh


