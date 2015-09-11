//
//  statement.hpp
//  fibio
//
//  Created by Chen Xu on 15/09/11.
//  Copyright (c) 2015 0d0a.com. All rights reserved.
//

#ifndef fibio_db_mysql_statement_hpp
#define fibio_db_mysql_statement_hpp

#include <cppconn/statement.h>
#include <cppconn/resultset.h>

namespace fibio {
namespace db {
namespace mysql {

class MySQL_Statement : public sql::Statement
{
public:
    virtual ~MySQL_Statement();

    virtual sql::Connection* getConnection() override;

    virtual void cancel() override;

    virtual void clearWarnings() override;

    virtual void close() override;

    virtual bool execute(const sql::SQLString& sql) override;

    virtual sql::ResultSet* executeQuery(const sql::SQLString& sql) override;

    virtual int executeUpdate(const sql::SQLString& sql) override;

    virtual size_t getFetchSize() override;

    virtual unsigned int getMaxFieldSize() override;

    virtual uint64_t getMaxRows() override;

    virtual bool getMoreResults() override;

    virtual unsigned int getQueryTimeout() override;

    virtual sql::ResultSet* getResultSet() override;

    virtual sql::ResultSet::enum_type getResultSetType() override;

    virtual uint64_t getUpdateCount() override;

    virtual const sql::SQLWarning* getWarnings() override;

    virtual void setCursorName(const sql::SQLString& name) override;

    virtual void setEscapeProcessing(bool enable) override;

    virtual void setFetchSize(size_t rows) override;

    virtual void setMaxFieldSize(unsigned int max) override;

    virtual void setMaxRows(unsigned int max) override;

    virtual void setQueryTimeout(unsigned int seconds) override;

    virtual Statement* setResultSetType(sql::ResultSet::enum_type type) override;

private:
    MySQL_Statement(sql::Connection* conn,
                    sql::Statement* impl,
                    std::shared_ptr<fibers::foreign_thread_pool> executor);
    sql::Connection* conn_;
    std::unique_ptr<sql::Statement> impl_;
    std::shared_ptr<fibers::foreign_thread_pool> executor_;
    friend class MySQL_Connection;
};

} // End of namespace mysql
} // End of namespace db
} // End of namespace fibio

#endif // !defined(fibio_db_mysql_statement_hpp)
