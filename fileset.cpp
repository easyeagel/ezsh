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

#include<deque>
#include<locale>
#include<algorithm>

#include"glob.hpp"
#include"encode.hpp"

namespace ezsh
{

FileSet::FileUnit::FileUnit(const bf::path& selfIn, const bf::path& baseIn, bool scanedIn)
    :scaned(scanedIn), self(selfIn), base(baseIn), total(normalize(baseIn/selfIn))
{
    refresh();
}

void FileSet::FileUnit::refresh()
{
    boost::system::error_code ec;
    status=bf::status(total, ec);
}

bf::path FileSet::FileUnit::normalize(const bf::path& path)
{
    if(path.empty())
        return path;

    //删除路径中多余的 . ..
    std::deque<bf::path> stk;
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

    bf::path ret;
    for(auto& p: stk)
        ret /= p;

    if(ret.empty())
        return ".";
    return ret;
}

bf::path FileSet::FileUnit::sub(const bf::path& path, const bf::path& base)
{
    auto count=std::distance(base.begin(), base.end());
    auto itr=path.begin(), end=path.end();
    while(count-- && itr!=end)
        ++itr;

    bf::path ret;
    for(; itr!=end; ++itr)
        ret /= *itr;
    if(ret.empty())
        return bf::path(".");
    return ret;
}

void FileSet::Component::options(bp::options_description& opt, bp::positional_options_description& pos)
{
    opt.add_options()
        ("fsFile",      bp::value<std::vector<std::string>>(), "files to set")
        ("fsGlob",      bp::value<std::vector<std::string>>(), "glob parttern include in set")
        ("fsGlobNot",   bp::value<std::vector<std::string>>(), "glob parttern not include in set")
        ("fsInclude",   bp::value<std::vector<std::string>>(), "regex parttern included in set")
        ("fsExclude",   bp::value<std::vector<std::string>>(), "regex parttern excluded from set")
        ("fsRecursive",                                        "operate recursively")
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
    strm << "  fileset select files with parttern or regex, run \"option fileset\" for details\n\n" << std::endl;
}

void FileSet::init(const bp::variables_map& vm)
{
    recursive_=vm.count("fsRecursive") ? true : false;

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
        FileUnit fu(pathCreate(file));
        if(!isRight(fu))
            continue;
        sets_.insert(std::move(fu));
    }
}

bool FileSet::isRight(const FileUnit& fu) const
{
	if (!globNot_.empty() || !glob_.empty())
	{
		const auto& file = fu.self.has_parent_path() ? fu.self.filename() : fu.self;
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
	
    return true;
}

namespace
{
    static ezsh::OptionRegisterT<ezsh::FileSet> gsOptionFileSet;
}
}  // namespace ezsh


