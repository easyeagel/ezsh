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
#include<boost/filesystem.hpp>

#include"option.hpp"

namespace ezsh
{

namespace bf=boost::filesystem;

class FileSet
{
    class Component: public OptComponent
    {
    public:
        void longHelp (std::ostream& strm) override
        {
            strm << "  fileset\n\n" << std::endl;
        }

        void shortHelp(std::ostream& strm) override
        {
            strm << "  fileset\n\n" << std::endl;
        }

        void options(bp::options_description& opt, bp::positional_options_description& pos) override;
    };

public:
    struct FileUnit
    {
        FileUnit(const bf::path& selfIn, const bf::path& baseIn=bf::path(), bool scanedIn=false);

        static bf::path normalize(const bf::path& path);
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

        bf::path self;
        bf::path base;
        bf::path total;
        bf::file_status status;

        bool operator<(const FileUnit& o) const
        {
            return total<o.total;
        }
    };

    static OptComponentSPtr componentGet()
    {
        static OptComponentSPtr ptr;
        if(!ptr)
            ptr.reset(new Component);
        return ptr;
    }

    void init(const bp::variables_map& dict);

    void scan();

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
private:
    bool recursive_=false;
    std::set<FileUnit> sets_;

    std::vector<std::string> files_;

    std::vector<std::string> glob_;
    std::vector<std::string> globNot_;

    std::vector<std::regex> includes_;
    std::vector<std::regex> excludes_;
};

}


