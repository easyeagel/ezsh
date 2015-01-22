//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cmdSQLiteImport.cpp
//
//   Description:  把文件或目录导入sqlite数据库
//
//       Version:  1.0
//       Created:  2014年12月25日 11时05分09秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<string>
#include<vector>
#include<fstream>
#include<sstream>
#include<core/encode.hpp>
#include<SQLiteCpp/SQLiteCpp.h>

#include"option.hpp"
#include"fileset.hpp"

namespace ezsh
{

namespace bf=boost::filesystem;

class SqlitePacket
{
    typedef boost::filesystem::path Path;
    typedef boost::filesystem::recursive_directory_iterator DirRItr;
public:
    SqlitePacket()=default;

    void init(const std::string& db, std::vector<std::string> dirs)
    {
        dirs_=std::move(dirs);
        sqlite_.reset(new SQLite::Database(db, SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE));
    }

    void init (const std::string& db, const std::string& tab, std::vector<std::string> dirs)
    {
        table_=tab;
        dirs_=std::move(dirs);
        sqlite_.reset(new SQLite::Database(db, SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE));
    }

    void noBaseSet(bool v=true)
    {
        noBase_=v;
    }

    void save()
    {
        SQLite::Transaction tran(*sqlite_);

        sqliteReset();
        for(const auto& dir: dirs_)
        {
            if(noBase_==false)
                eleSave(true, dir, Path());

            const auto path=pathCreate(dir);
            if(!bf::exists(path))
                continue;

            DirRItr itr(path);
            DirRItr const end;
            for(; itr!=end; ++itr)
            {
                const auto& status=itr->status();
                eleSave(status.type()==boost::filesystem::directory_file, itr->path(), path);
            }
        }

        tran.commit();
    }

private:
    void sqliteReset()
    {
        std::stringstream stm;
        stm << "create table if not exists "
            << table()
            << " (name blob primary key, type integer, value blob);";

        sqlite_->exec(stm.str());

        std::string sql = "insert or replace into ";
        sql += table();
        sql += " (name, type, value) values(:1, :2, :3);";

        query_.reset(new SQLite::Statement(*sqlite_, sql));
    }

    void eleSave(bool dir, const Path& path, const Path& base)
    {
        //目录统一使用UTF8保存
        const auto& str=core::WCharConverter::to(noBase_==false ? path.string() : FileUnit::sub(path, base).string());
        query_->bind(1, static_cast<const void*>(str.c_str()), str.size());

        if(dir)
        {
            query_->bind(2, 1);
            query_->bind(3, static_cast<const void*>(""), 0);
            query_->exec();
            query_->reset();
            return;
        }

        const auto fz=boost::filesystem::file_size(path);
        std::string content(static_cast<size_t>(fz), '\0');
        bf::ifstream file(path);
        file.read(const_cast<char*>(content.data()), fz);

        query_->bind(2, 0);
        query_->bind(3, static_cast<const void*>(content.data()), content.size());

        query_->exec();
        query_->reset();

        return;
    }

    const char* table()
    {
        if(table_.empty())
            return "ezshSQLiteImportFiles";
        return table_.c_str();
    }

private:
    bool noBase_=false;
    std::string table_;
    std::vector<std::string> dirs_;
    std::shared_ptr<SQLite::Database> sqlite_;
    std::shared_ptr<SQLite::Statement> query_;
};

class CmdSQLiteImport: public CmdBaseT<CmdSQLiteImport>
{
    typedef CmdBaseT<CmdSQLiteImport> BaseThis;
public:
    CmdSQLiteImport()
        :BaseThis("sqliteImport - import file or dir to sqlite database")
    {
        opt_.add_options()
            ("noBase", "no base")
            ("db",     bp::value<std::string>()->required(), "sqlite database")
            ("table",  bp::value<std::string>(), "table name of sqlite database")
            ("file,f", bp::value<std::vector<std::string>>()->required(), "file or dir to import")
        ;
        optPos_.add("file", -1);
    }

    static const char* nameGet()
    {
        return "sqliteImport";
    }

    void doit()
    {
        const auto& vm=mapGet();

        const auto& db=vm["db"].as<std::string>();
        const auto& files=vm["file"].as<std::vector<std::string>>();

        SqlitePacket sp;
        sp.noBaseSet(vm.count("noBase")>0);
        if(0==vm.count("table"))
            sp.init(db, files);
        else
            sp.init(db, vm["table"].as<std::string>(), files);

        sp.save();
    }

};

namespace
{
static CmdRegisterT<CmdSQLiteImport> gsRegister;
}

}

