//
//  prepared_statement.hpp
//  fibio
//
//  Created by Chen Xu on 15/09/11.
//  Copyright (c) 2015 0d0a.com. All rights reserved.
//

#ifndef fibio_db_mysql_prepared_statement_hpp
#define fibio_db_mysql_prepared_statement_hpp

#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

namespace fibio {
namespace db {
namespace mysql {

class MySQL_PreparedStatement : public sql::PreparedStatement
{
public:
    virtual ~MySQL_PreparedStatement();

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

    virtual PreparedStatement* setResultSetType(sql::ResultSet::enum_type type) override;

    virtual void clearParameters() override;

    virtual bool execute() override;

    virtual sql::ResultSet *executeQuery() override;

    virtual int executeUpdate() override;

    virtual sql::ResultSetMetaData * getMetaData() override;

    virtual sql::ParameterMetaData * getParameterMetaData() override;

    virtual void setBigInt(unsigned int parameterIndex, const sql::SQLString& value) override;

    virtual void setBlob(unsigned int parameterIndex, std::istream * blob) override;

    virtual void setBoolean(unsigned int parameterIndex, bool value) override;

    virtual void setDateTime(unsigned int parameterIndex, const sql::SQLString& value) override;

    virtual void setDouble(unsigned int parameterIndex, double value) override;

    virtual void setInt(unsigned int parameterIndex, int32_t value) override;

    virtual void setUInt(unsigned int parameterIndex, uint32_t value) override;

    virtual void setInt64(unsigned int parameterIndex, int64_t value) override;

    virtual void setUInt64(unsigned int parameterIndex, uint64_t value) override;

    virtual void setNull(unsigned int parameterIndex, int sqlType) override;

    virtual void setString(unsigned int parameterIndex, const sql::SQLString& value) override;

private:
    MySQL_PreparedStatement(sql::Connection* conn,
                            sql::PreparedStatement* impl,
                            std::shared_ptr<fibers::foreign_thread_pool> executor);
    sql::Connection* conn_;
    std::unique_ptr<sql::PreparedStatement> impl_;
    std::shared_ptr<fibers::foreign_thread_pool> executor_;
    friend class MySQL_Connection;
};

} // End of namespace mysql
} // End of namespace db
} // End of namespace fibio

#endif // !defined(fibio_db_mysql_prepared_statement_hpp)
