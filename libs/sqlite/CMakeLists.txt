

#编译sqlite
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#===============================================================================================
set(sqliteSrc 
    Column.cpp
    Database.cpp
    Statement.cpp
    Transaction.cpp
    sqlite3.c
)
add_library(sqlite STATIC ${sqliteSrc})
target_include_directories(sqlite
    PUBLIC "${CMAKE_CUREENT_DIR}"
)

set_target_properties(sqlite PROPERTIES COMPILE_FLAGS "-w")

