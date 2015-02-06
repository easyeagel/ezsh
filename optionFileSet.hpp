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

#include<regex>
#include<vector>

#include"option.hpp"
#include"filesystem.hpp"

namespace ezsh
{

struct FileUnit
{
    FileUnit()=default;
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

    bool isChild(const FileUnit& parent) const
    {
        return total.isChild(parent.total);
    }

    void doneSet() const
    {
        const_cast<bool&>(done)=true;
    }

    bool done=false;
    bool scaned;

    Path self;
    Path base;
    Path total;
    uint64_t size=0;
    std::time_t ctime=0;
    bf::file_status status;

    bool operator<(const FileUnit& o) const
    {
        return total<o.total;
    }
};

class FileSet
{
    class Component: public OptionComponent
    {
    public:
        void longHelp (std::ostream& strm) override;
        void shortHelp(std::ostream& strm) override;
        void options(bp::options_description& opt, bp::positional_options_description& pos) override;
    };

    enum:size_t {eRecursiveDefault=static_cast<size_t>(-1)};

public:
    static OptionComponentSPtr componentGet()
    {
        static OptionComponentSPtr ptr;
        if(!ptr)
            ptr.reset(new Component);
        return ptr;
    }

    void config(CmdBase& cmd);

    void init(const bp::variables_map& dict);

    void scan()
    {
        scan([this](FileUnit&& u)
            {
                sets_.emplace_back(std::move(u));
                return true;
            }
        );
    }

    template<typename Call>
    void scan(Call&& call)
    {
        if(recursive_==0)
            return;

        for(const auto& file: files_)
        {
            FileUnit fu(file);
            if(!fu.isDir())
                continue;

            typedef bf::recursive_directory_iterator DirItr;
            for(auto itr=DirItr(fu.total), end=DirItr(); itr!=end; ++itr)
            {
                if(static_cast<size_t>(itr.level())>=recursive_)
                {
                    itr.pop();
                    if(itr==end)
                        return;
                    continue;
                }

                FileUnit u(FileUnit::sub(itr->path(), fu.self), fu.self, true);
                if(!isRight(u))
                    continue;

                if(call(std::move(u))==false)
                    return;
            }
        }

    }

    template<typename Call>
    void loop(Call&& call)
    {
        for(const auto& f: setGet())
        {
            if(call(FileUnit(f))==false)
                return;
        }

        scan(std::move(call));
    }

    size_t recursiveGet() const
    {
        return recursive_;
    }

    bool isRecursive() const
    {
        return recursive_==eRecursiveDefault;
    }

    std::vector<FileUnit>& setGet()
    {
        return sets_;
    }

    bool isRight(const FileUnit& fu) const;

    const std::vector<FileUnit>& setGet() const
    {
        return sets_;
    }

    static const char* nameGet()
    {
        return "fileset";
    }

    void subtreeDone(const FileUnit& u);

private:
    void afterParse(const bp::variables_map& vm);
    void sizeEqual(const std::vector<std::string>& param, bool n);

private:
    size_t recursive_=-1;
    std::vector<FileUnit> sets_;

    std::vector<std::string> files_;

    std::vector<std::string> glob_;
    std::vector<std::string> globNot_;

    std::vector<std::wregex> includes_;
    std::vector<std::wregex> excludes_;

    std::vector<std::function<bool (const FileUnit&)>> predications_;
};

template<typename Base=CmdBase>
class FileSetCmdBase: public Base
{
public:
    FileSetCmdBase(const char* msg)
        :Base(msg)
    {
        files_.config(*this);
    }

    FileSet& fileGet()
    {
        return files_;
    }

    const FileSet& fileGet() const
    {
        return files_;
    }

private:
    FileSet files_;
};

}


