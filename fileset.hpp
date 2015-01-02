//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  fileset.hpp
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

#pragma once

#include<set>
#include<regex>

#include"option.hpp"
#include"filesystem.hpp"

namespace ezsh
{

namespace bf=boost::filesystem;

class FileSet
{
    class Component: public OptionComponent
    {
    public:
        void longHelp (std::ostream& strm) override;
        void shortHelp(std::ostream& strm) override;
        void options(bp::options_description& opt, bp::positional_options_description& pos) override;
    };

public:
    struct FileUnit
    {
        FileUnit(const Path& selfIn, const Path& baseIn=Path(), bool scanedIn=false);

        static Path normalize(const Path& path);
        static Path sub(const Path& path, const Path& base);
        void refresh();

        bool isDir() const
        {
            return status.type()==bf::file_type::directory_file;
        }

        bool isExist() const
        {
            return status.type()!=bf::file_type::file_not_found;
        }

        bool scaned;

        Path self;
        Path base;
        Path total;
        bf::file_status status;

        bool operator<(const FileUnit& o) const
        {
            return total<o.total;
        }
    };

    static OptionComponentSPtr componentGet()
    {
        static OptionComponentSPtr ptr;
        if(!ptr)
            ptr.reset(new Component);
        return ptr;
    }

    void init(const bp::variables_map& dict);

    void scan()
    {
        scan([this](FileUnit&& u)
            {
                sets_.insert(std::move(u));
            }
        );
    }

    template<typename Call>
    void scan(Call&& call)
    {
        if(recursive_==false)
            return;

        for(const auto& file: files_)
        {
            FileUnit fu(file);
            if(!fu.isDir())
                continue;
            typedef bf::recursive_directory_iterator DirItr;
            for(auto itr=DirItr(file), end=DirItr(); itr!=end; ++itr)
            {
                FileUnit u(FileUnit::sub(itr->path(), file), file);
                if(!isRight(u))
                    continue;
                call(std::move(u));
            }
        }

    }

    bool isRecursive() const
    {
        return recursive_;
    }

    std::set<FileUnit>& setGet()
    {
        return sets_;
    }

    bool isRight(const FileUnit& fu) const;

    const std::set<FileUnit>& setGet() const
    {
        return sets_;
    }

    static const char* nameGet()
    {
        return "fileset";
    }

private:
    bool recursive_=false;
    std::set<FileUnit> sets_;

    std::vector<std::string> files_;

    std::vector<std::string> glob_;
    std::vector<std::string> globNot_;

    std::vector<std::wregex> includes_;
    std::vector<std::wregex> excludes_;
};

}


