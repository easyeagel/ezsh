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
#include<boost/filesystem.hpp>
#include<SQLiteCpp/SQLiteCpp.h>

#include"option.hpp"

namespace ezsh
{

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

    void save()
    {
        SQLite::Transaction tran(*sqlite_);

        sqliteReset();
        for(const auto& dir: dirs_)
        {
            eleSave(true, dir);

            DirRItr itr(dir);
            DirRItr const end;
            for(; itr!=end; ++itr)
            {
                const auto& status=itr->status();
                eleSave(status.type()==boost::filesystem::directory_file, itr->path());
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

    void eleSave(bool dir, const Path& path)
    {
        const auto& str=path.string();
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
        std::ifstream file(path.c_str());
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
            return "magicRedFiles";
        return table_.c_str();
    }

private:
    std::string table_;
    std::vector<std::string> dirs_;
    std::shared_ptr<SQLite::Database> sqlite_;
    std::shared_ptr<SQLite::Statement> query_;
};

class CmdSQLiteImport: public OptBaseT<CmdSQLiteImport>
{
    typedef OptBaseT<CmdSQLiteImport> BaseThis;
public:
    CmdSQLiteImport()
        :BaseThis("sqliteImport - import file or dir to sqlite database")
    {
        opt_.add_options()
            ("table",  bp::value<std::string>(), "table name of sqlite database")
            ("db",     bp::value<std::string>()->required(), "sqlite database")
            ("file,f", bp::value<std::vector<std::string>>()->required(), "file or dir to import")
        ;
        optPos_.add("file", -1);
    }

    static const char* nameGet()
    {
        return "sqliteImport";
    }

    MainReturn doit() override
    {
        const auto& vm=mapGet();

        const auto& db=vm["db"].as<std::string>();
        const auto& files=vm["file"].as<std::vector<std::string>>();

        SqlitePacket sp;
        if(0==vm.count("table"))
            sp.init(db, files);
        else
            sp.init(db, vm["table"].as<std::string>(), files);

        sp.save();

        return MainReturn::eGood;
    }

};

namespace
{
static OptRegisterT<CmdSQLiteImport> gsRegister;
}

}

