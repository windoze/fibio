//
//  database_metadata.cpp
//  fibio
//
//  Created by Chen Xu on 15/09/11.
//  Copyright (c) 2015 0d0a.com. All rights reserved.
//

#include <fibio/fibers/future/async.hpp>
#include "connection.hpp"
#include "database_metadata.hpp"
#include "resultset.hpp"

namespace fibio {
namespace db {
namespace mysql {

MySQL_DatabaseMetaData::MySQL_DatabaseMetaData(
    MySQL_Connection* conn,
    sql::DatabaseMetaData* impl,
    std::shared_ptr<fibers::foreign_thread_pool> executor)
: conn_(conn), impl_(impl), executor_(executor)
{
}

MySQL_DatabaseMetaData::~MySQL_DatabaseMetaData()
{
}

bool MySQL_DatabaseMetaData::allProceduresAreCallable()
{
    return (*executor_)([&]() { return impl_->allProceduresAreCallable(); });
}

bool MySQL_DatabaseMetaData::allTablesAreSelectable()
{
    return (*executor_)([&]() { return impl_->allTablesAreSelectable(); });
}

bool MySQL_DatabaseMetaData::dataDefinitionCausesTransactionCommit()
{
    return (*executor_)([&]() { return impl_->dataDefinitionCausesTransactionCommit(); });
}

bool MySQL_DatabaseMetaData::dataDefinitionIgnoredInTransactions()
{
    return (*executor_)([&]() { return impl_->dataDefinitionIgnoredInTransactions(); });
}

bool MySQL_DatabaseMetaData::deletesAreDetected(int type)
{
    return (*executor_)([&]() { return impl_->deletesAreDetected(type); });
}

bool MySQL_DatabaseMetaData::doesMaxRowSizeIncludeBlobs()
{
    return (*executor_)([&]() { return impl_->doesMaxRowSizeIncludeBlobs(); });
}

sql::ResultSet* MySQL_DatabaseMetaData::getAttributes(const sql::SQLString& catalog,
                                                      const sql::SQLString& schemaPattern,
                                                      const sql::SQLString& typeNamePattern,
                                                      const sql::SQLString& attributeNamePattern)
{
    sql::ResultSet* rs = (*executor_)([&]() {
        return impl_->getAttributes(catalog, schemaPattern, typeNamePattern, attributeNamePattern);
    });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

sql::ResultSet* MySQL_DatabaseMetaData::getBestRowIdentifier(const sql::SQLString& catalog,
                                                             const sql::SQLString& schema,
                                                             const sql::SQLString& table,
                                                             int scope,
                                                             bool nullable)
{
    sql::ResultSet* rs = (*executor_)(
        [&]() { return impl_->getBestRowIdentifier(catalog, schema, table, scope, nullable); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

sql::ResultSet* MySQL_DatabaseMetaData::getCatalogs()
{
    sql::ResultSet* rs = (*executor_)([&]() { return impl_->getCatalogs(); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

const sql::SQLString& MySQL_DatabaseMetaData::getCatalogSeparator()
{
    return (*executor_)([&]() -> const sql::SQLString& { return impl_->getCatalogSeparator(); });
}

const sql::SQLString& MySQL_DatabaseMetaData::getCatalogTerm()
{
    return (*executor_)([&]() -> const sql::SQLString& { return impl_->getCatalogTerm(); });
}

sql::ResultSet* MySQL_DatabaseMetaData::getColumnPrivileges(const sql::SQLString& catalog,
                                                            const sql::SQLString& schema,
                                                            const sql::SQLString& table,
                                                            const sql::SQLString& columnNamePattern)
{
    sql::ResultSet* rs = (*executor_)(
        [&]() { return impl_->getColumnPrivileges(catalog, schema, table, columnNamePattern); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

sql::ResultSet* MySQL_DatabaseMetaData::getColumns(const sql::SQLString& catalog,
                                                   const sql::SQLString& schemaPattern,
                                                   const sql::SQLString& tableNamePattern,
                                                   const sql::SQLString& columnNamePattern)
{
    sql::ResultSet* rs = (*executor_)([&]() {
        return impl_->getColumns(catalog, schemaPattern, tableNamePattern, columnNamePattern);
    });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

sql::Connection* MySQL_DatabaseMetaData::getConnection()
{
    return conn_;
}

sql::ResultSet* MySQL_DatabaseMetaData::getCrossReference(const sql::SQLString& primaryCatalog,
                                                          const sql::SQLString& primarySchema,
                                                          const sql::SQLString& primaryTable,
                                                          const sql::SQLString& foreignCatalog,
                                                          const sql::SQLString& foreignSchema,
                                                          const sql::SQLString& foreignTable)
{
    sql::ResultSet* rs = (*executor_)([&]() {
        return impl_->getCrossReference(primaryCatalog,
                                        primarySchema,
                                        primaryTable,
                                        foreignCatalog,
                                        foreignSchema,
                                        foreignTable);
    });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

unsigned int MySQL_DatabaseMetaData::getDatabaseMajorVersion()
{
    return (*executor_)([&]() { return impl_->getDatabaseMajorVersion(); });
}

unsigned int MySQL_DatabaseMetaData::getDatabaseMinorVersion()
{
    return (*executor_)([&]() { return impl_->getDatabaseMinorVersion(); });
}

unsigned int MySQL_DatabaseMetaData::getDatabasePatchVersion()
{
    return (*executor_)([&]() { return impl_->getDatabasePatchVersion(); });
}

const sql::SQLString& MySQL_DatabaseMetaData::getDatabaseProductName()
{
    return (*executor_)([&]() -> const sql::SQLString& { return impl_->getDatabaseProductName(); });
}

sql::SQLString MySQL_DatabaseMetaData::getDatabaseProductVersion()
{
    return (*executor_)([&]() { return impl_->getDatabaseProductVersion(); });
}

int MySQL_DatabaseMetaData::getDefaultTransactionIsolation()
{
    return (*executor_)([&]() { return impl_->getDefaultTransactionIsolation(); });
}

unsigned int MySQL_DatabaseMetaData::getDriverMajorVersion()
{
    return (*executor_)([&]() { return impl_->getDriverMajorVersion(); });
}

unsigned int MySQL_DatabaseMetaData::getDriverMinorVersion()
{
    return (*executor_)([&]() { return impl_->getDriverMinorVersion(); });
}

unsigned int MySQL_DatabaseMetaData::getDriverPatchVersion()
{
    return (*executor_)([&]() { return impl_->getDriverPatchVersion(); });
}

const sql::SQLString& MySQL_DatabaseMetaData::getDriverName()
{
    return (*executor_)([&]() -> const sql::SQLString& { return impl_->getDriverName(); });
}

const sql::SQLString& MySQL_DatabaseMetaData::getDriverVersion()
{
    return (*executor_)([&]() -> const sql::SQLString& { return impl_->getDriverVersion(); });
}

sql::ResultSet* MySQL_DatabaseMetaData::getExportedKeys(const sql::SQLString& catalog,
                                                        const sql::SQLString& schema,
                                                        const sql::SQLString& table)
{
    sql::ResultSet* rs
        = (*executor_)([&]() { return impl_->getExportedKeys(catalog, schema, table); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

const sql::SQLString& MySQL_DatabaseMetaData::getExtraNameCharacters()
{
    return (*executor_)([&]() -> const sql::SQLString& { return impl_->getExtraNameCharacters(); });
}

const sql::SQLString& MySQL_DatabaseMetaData::getIdentifierQuoteString()
{
    return (*executor_)(
        [&]() -> const sql::SQLString& { return impl_->getIdentifierQuoteString(); });
}

sql::ResultSet* MySQL_DatabaseMetaData::getImportedKeys(const sql::SQLString& catalog,
                                                        const sql::SQLString& schema,
                                                        const sql::SQLString& table)
{
    sql::ResultSet* rs
        = (*executor_)([&]() { return impl_->getImportedKeys(catalog, schema, table); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

sql::ResultSet* MySQL_DatabaseMetaData::getIndexInfo(const sql::SQLString& catalog,
                                                     const sql::SQLString& schema,
                                                     const sql::SQLString& table,
                                                     bool unique,
                                                     bool approximate)
{
    sql::ResultSet* rs = (*executor_)(
        [&]() { return impl_->getIndexInfo(catalog, schema, table, unique, approximate); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

unsigned int MySQL_DatabaseMetaData::getCDBCMajorVersion()
{
    return (*executor_)([&]() { return impl_->getCDBCMajorVersion(); });
}

unsigned int MySQL_DatabaseMetaData::getCDBCMinorVersion()
{
    return (*executor_)([&]() { return impl_->getCDBCMinorVersion(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxBinaryLiteralLength()
{
    return (*executor_)([&]() { return impl_->getMaxBinaryLiteralLength(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxCatalogNameLength()
{
    return (*executor_)([&]() { return impl_->getMaxCatalogNameLength(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxCharLiteralLength()
{
    return (*executor_)([&]() { return impl_->getMaxCharLiteralLength(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxColumnNameLength()
{
    return (*executor_)([&]() { return impl_->getMaxColumnNameLength(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxColumnsInGroupBy()
{
    return (*executor_)([&]() { return impl_->getMaxColumnsInGroupBy(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxColumnsInIndex()
{
    return (*executor_)([&]() { return impl_->getMaxColumnsInIndex(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxColumnsInOrderBy()
{
    return (*executor_)([&]() { return impl_->getMaxColumnsInOrderBy(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxColumnsInSelect()
{
    return (*executor_)([&]() { return impl_->getMaxColumnsInSelect(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxColumnsInTable()
{
    return (*executor_)([&]() { return impl_->getMaxColumnsInTable(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxConnections()
{
    return (*executor_)([&]() { return impl_->getMaxConnections(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxCursorNameLength()
{
    return (*executor_)([&]() { return impl_->getMaxCursorNameLength(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxIndexLength()
{
    return (*executor_)([&]() { return impl_->getMaxIndexLength(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxProcedureNameLength()
{
    return (*executor_)([&]() { return impl_->getMaxProcedureNameLength(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxRowSize()
{
    return (*executor_)([&]() { return impl_->getMaxRowSize(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxSchemaNameLength()
{
    return (*executor_)([&]() { return impl_->getMaxSchemaNameLength(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxStatementLength()
{
    return (*executor_)([&]() { return impl_->getMaxStatementLength(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxStatements()
{
    return (*executor_)([&]() { return impl_->getMaxStatements(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxTableNameLength()
{
    return (*executor_)([&]() { return impl_->getMaxTableNameLength(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxTablesInSelect()
{
    return (*executor_)([&]() { return impl_->getMaxTablesInSelect(); });
}

unsigned int MySQL_DatabaseMetaData::getMaxUserNameLength()
{
    return (*executor_)([&]() { return impl_->getMaxUserNameLength(); });
}

const sql::SQLString& MySQL_DatabaseMetaData::getNumericFunctions()
{
    return (*executor_)([&]() -> const sql::SQLString& { return impl_->getNumericFunctions(); });
}

sql::ResultSet* MySQL_DatabaseMetaData::getPrimaryKeys(const sql::SQLString& catalog,
                                                       const sql::SQLString& schema,
                                                       const sql::SQLString& table)
{
    sql::ResultSet* rs
        = (*executor_)([&]() { return impl_->getPrimaryKeys(catalog, schema, table); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

sql::ResultSet*
MySQL_DatabaseMetaData::getProcedureColumns(const sql::SQLString& catalog,
                                            const sql::SQLString& schemaPattern,
                                            const sql::SQLString& procedureNamePattern,
                                            const sql::SQLString& columnNamePattern)
{
    sql::ResultSet* rs = (*executor_)([&]() {
        return impl_->getProcedureColumns(
            catalog, schemaPattern, procedureNamePattern, columnNamePattern);
    });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

sql::ResultSet* MySQL_DatabaseMetaData::getProcedures(const sql::SQLString& catalog,
                                                      const sql::SQLString& schemaPattern,
                                                      const sql::SQLString& procedureNamePattern)
{
    sql::ResultSet* rs = (*executor_)(
        [&]() { return impl_->getProcedures(catalog, schemaPattern, procedureNamePattern); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

const sql::SQLString& MySQL_DatabaseMetaData::getProcedureTerm()
{
    return (*executor_)([&]() -> const sql::SQLString& { return impl_->getProcedureTerm(); });
}

int MySQL_DatabaseMetaData::getResultSetHoldability()
{
    return (*executor_)([&]() { return impl_->getResultSetHoldability(); });
}

sql::ResultSet* MySQL_DatabaseMetaData::getSchemas()
{
    sql::ResultSet* rs = (*executor_)([&]() { return impl_->getSchemas(); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

const sql::SQLString& MySQL_DatabaseMetaData::getSchemaTerm()
{
    return (*executor_)([&]() -> const sql::SQLString& { return impl_->getSchemaTerm(); });
}

sql::ResultSet* MySQL_DatabaseMetaData::getSchemaCollation(const sql::SQLString& catalog,
                                                           const sql::SQLString& schemaPattern)
{
    sql::ResultSet* rs
        = (*executor_)([&]() { return impl_->getSchemaCollation(catalog, schemaPattern); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

sql::ResultSet* MySQL_DatabaseMetaData::getSchemaCharset(const sql::SQLString& catalog,
                                                         const sql::SQLString& schemaPattern)
{
    sql::ResultSet* rs
        = (*executor_)([&]() { return impl_->getSchemaCharset(catalog, schemaPattern); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

const sql::SQLString& MySQL_DatabaseMetaData::getSearchStringEscape()
{
    return (*executor_)([&]() -> const sql::SQLString& { return impl_->getSearchStringEscape(); });
}

const sql::SQLString& MySQL_DatabaseMetaData::getSQLKeywords()
{
    return (*executor_)([&]() -> const sql::SQLString& { return impl_->getSQLKeywords(); });
}

int MySQL_DatabaseMetaData::getSQLStateType()
{
    return (*executor_)([&]() { return impl_->getSQLStateType(); });
}

const sql::SQLString& MySQL_DatabaseMetaData::getStringFunctions()
{
    return (*executor_)([&]() -> const sql::SQLString& { return impl_->getStringFunctions(); });
}

sql::ResultSet* MySQL_DatabaseMetaData::getSuperTables(const sql::SQLString& catalog,
                                                       const sql::SQLString& schemaPattern,
                                                       const sql::SQLString& tableNamePattern)
{
    sql::ResultSet* rs = (*executor_)(
        [&]() { return impl_->getSuperTables(catalog, schemaPattern, tableNamePattern); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

sql::ResultSet* MySQL_DatabaseMetaData::getSuperTypes(const sql::SQLString& catalog,
                                                      const sql::SQLString& schemaPattern,
                                                      const sql::SQLString& typeNamePattern)
{
    sql::ResultSet* rs = (*executor_)(
        [&]() { return impl_->getSuperTypes(catalog, schemaPattern, typeNamePattern); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

const sql::SQLString& MySQL_DatabaseMetaData::getSystemFunctions()
{
    return (*executor_)([&]() -> const sql::SQLString& { return impl_->getSystemFunctions(); });
}

sql::ResultSet* MySQL_DatabaseMetaData::getTablePrivileges(const sql::SQLString& catalog,
                                                           const sql::SQLString& schemaPattern,
                                                           const sql::SQLString& tableNamePattern)
{
    sql::ResultSet* rs = (*executor_)(
        [&]() { return impl_->getTablePrivileges(catalog, schemaPattern, tableNamePattern); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

sql::ResultSet* MySQL_DatabaseMetaData::getTables(const sql::SQLString& catalog,
                                                  const sql::SQLString& schemaPattern,
                                                  const sql::SQLString& tableNamePattern,
                                                  std::list<sql::SQLString>& types)
{
    sql::ResultSet* rs = (*executor_)(
        [&]() { return impl_->getTables(catalog, schemaPattern, tableNamePattern, types); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

sql::ResultSet* MySQL_DatabaseMetaData::getTableTypes()
{
    sql::ResultSet* rs = (*executor_)([&]() { return impl_->getTableTypes(); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

sql::ResultSet* MySQL_DatabaseMetaData::getTableCollation(const sql::SQLString& catalog,
                                                          const sql::SQLString& schemaPattern,
                                                          const sql::SQLString& tableNamePattern)
{
    sql::ResultSet* rs = (*executor_)(
        [&]() { return impl_->getTableCollation(catalog, schemaPattern, tableNamePattern); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

sql::ResultSet* MySQL_DatabaseMetaData::getTableCharset(const sql::SQLString& catalog,
                                                        const sql::SQLString& schemaPattern,
                                                        const sql::SQLString& tableNamePattern)
{
    sql::ResultSet* rs = (*executor_)(
        [&]() { return impl_->getTableCharset(catalog, schemaPattern, tableNamePattern); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

const sql::SQLString& MySQL_DatabaseMetaData::getTimeDateFunctions()
{
    return (*executor_)([&]() -> const sql::SQLString& { return impl_->getTimeDateFunctions(); });
}

sql::ResultSet* MySQL_DatabaseMetaData::getTypeInfo()
{
    sql::ResultSet* rs = (*executor_)([&]() { return impl_->getTypeInfo(); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

sql::ResultSet* MySQL_DatabaseMetaData::getUDTs(const sql::SQLString& catalog,
                                                const sql::SQLString& schemaPattern,
                                                const sql::SQLString& typeNamePattern,
                                                std::list<int>& types)
{
    sql::ResultSet* rs = (*executor_)(
        [&]() { return impl_->getUDTs(catalog, schemaPattern, typeNamePattern, types); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

sql::SQLString MySQL_DatabaseMetaData::getURL()
{
    return (*executor_)([&]() { return impl_->getURL(); });
}

sql::SQLString MySQL_DatabaseMetaData::getUserName()
{
    return (*executor_)([&]() { return impl_->getUserName(); });
}

sql::ResultSet* MySQL_DatabaseMetaData::getVersionColumns(const sql::SQLString& catalog,
                                                          const sql::SQLString& schema,
                                                          const sql::SQLString& table)
{
    sql::ResultSet* rs
        = (*executor_)([&]() { return impl_->getVersionColumns(catalog, schema, table); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

bool MySQL_DatabaseMetaData::insertsAreDetected(int type)
{
    return (*executor_)([&]() { return impl_->insertsAreDetected(type); });
}

bool MySQL_DatabaseMetaData::isCatalogAtStart()
{
    return (*executor_)([&]() { return impl_->isCatalogAtStart(); });
}

bool MySQL_DatabaseMetaData::isReadOnly()
{
    return (*executor_)([&]() { return impl_->isReadOnly(); });
}

bool MySQL_DatabaseMetaData::locatorsUpdateCopy()
{
    return (*executor_)([&]() { return impl_->locatorsUpdateCopy(); });
}

bool MySQL_DatabaseMetaData::nullPlusNonNullIsNull()
{
    return (*executor_)([&]() { return impl_->nullPlusNonNullIsNull(); });
}

bool MySQL_DatabaseMetaData::nullsAreSortedAtEnd()
{
    return (*executor_)([&]() { return impl_->nullsAreSortedAtEnd(); });
}

bool MySQL_DatabaseMetaData::nullsAreSortedAtStart()
{
    return (*executor_)([&]() { return impl_->nullsAreSortedAtStart(); });
}

bool MySQL_DatabaseMetaData::nullsAreSortedHigh()
{
    return (*executor_)([&]() { return impl_->nullsAreSortedHigh(); });
}

bool MySQL_DatabaseMetaData::nullsAreSortedLow()
{
    return (*executor_)([&]() { return impl_->nullsAreSortedLow(); });
}

bool MySQL_DatabaseMetaData::othersDeletesAreVisible(int type)
{
    return (*executor_)([&]() { return impl_->othersDeletesAreVisible(type); });
}

bool MySQL_DatabaseMetaData::othersInsertsAreVisible(int type)
{
    return (*executor_)([&]() { return impl_->othersInsertsAreVisible(type); });
}

bool MySQL_DatabaseMetaData::othersUpdatesAreVisible(int type)
{
    return (*executor_)([&]() { return impl_->othersUpdatesAreVisible(type); });
}

bool MySQL_DatabaseMetaData::ownDeletesAreVisible(int type)
{
    return (*executor_)([&]() { return impl_->ownDeletesAreVisible(type); });
}

bool MySQL_DatabaseMetaData::ownInsertsAreVisible(int type)
{
    return (*executor_)([&]() { return impl_->ownInsertsAreVisible(type); });
}

bool MySQL_DatabaseMetaData::ownUpdatesAreVisible(int type)
{
    return (*executor_)([&]() { return impl_->ownUpdatesAreVisible(type); });
}

bool MySQL_DatabaseMetaData::storesLowerCaseIdentifiers()
{
    return (*executor_)([&]() { return impl_->storesLowerCaseIdentifiers(); });
}

bool MySQL_DatabaseMetaData::storesLowerCaseQuotedIdentifiers()
{
    return (*executor_)([&]() { return impl_->storesLowerCaseQuotedIdentifiers(); });
}

bool MySQL_DatabaseMetaData::storesMixedCaseIdentifiers()
{
    return (*executor_)([&]() { return impl_->storesMixedCaseIdentifiers(); });
}

bool MySQL_DatabaseMetaData::storesMixedCaseQuotedIdentifiers()
{
    return (*executor_)([&]() { return impl_->storesMixedCaseQuotedIdentifiers(); });
}

bool MySQL_DatabaseMetaData::storesUpperCaseIdentifiers()
{
    return (*executor_)([&]() { return impl_->storesUpperCaseIdentifiers(); });
}

bool MySQL_DatabaseMetaData::storesUpperCaseQuotedIdentifiers()
{
    return (*executor_)([&]() { return impl_->storesUpperCaseQuotedIdentifiers(); });
}

bool MySQL_DatabaseMetaData::supportsAlterTableWithAddColumn()
{
    return (*executor_)([&]() { return impl_->supportsAlterTableWithAddColumn(); });
}

bool MySQL_DatabaseMetaData::supportsAlterTableWithDropColumn()
{
    return (*executor_)([&]() { return impl_->supportsAlterTableWithDropColumn(); });
}

bool MySQL_DatabaseMetaData::supportsANSI92EntryLevelSQL()
{
    return (*executor_)([&]() { return impl_->supportsANSI92EntryLevelSQL(); });
}

bool MySQL_DatabaseMetaData::supportsANSI92FullSQL()
{
    return (*executor_)([&]() { return impl_->supportsANSI92FullSQL(); });
}

bool MySQL_DatabaseMetaData::supportsANSI92IntermediateSQL()
{
    return (*executor_)([&]() { return impl_->supportsANSI92IntermediateSQL(); });
}

bool MySQL_DatabaseMetaData::supportsBatchUpdates()
{
    return (*executor_)([&]() { return impl_->supportsBatchUpdates(); });
}

bool MySQL_DatabaseMetaData::supportsCatalogsInDataManipulation()
{
    return (*executor_)([&]() { return impl_->supportsCatalogsInDataManipulation(); });
}

bool MySQL_DatabaseMetaData::supportsCatalogsInIndexDefinitions()
{
    return (*executor_)([&]() { return impl_->supportsCatalogsInIndexDefinitions(); });
}

bool MySQL_DatabaseMetaData::supportsCatalogsInPrivilegeDefinitions()
{
    return (*executor_)([&]() { return impl_->supportsCatalogsInPrivilegeDefinitions(); });
}

bool MySQL_DatabaseMetaData::supportsCatalogsInProcedureCalls()
{
    return (*executor_)([&]() { return impl_->supportsCatalogsInProcedureCalls(); });
}

bool MySQL_DatabaseMetaData::supportsCatalogsInTableDefinitions()
{
    return (*executor_)([&]() { return impl_->supportsCatalogsInTableDefinitions(); });
}

bool MySQL_DatabaseMetaData::supportsColumnAliasing()
{
    return (*executor_)([&]() { return impl_->supportsColumnAliasing(); });
}

bool MySQL_DatabaseMetaData::supportsConvert()
{
    return (*executor_)([&]() { return impl_->supportsConvert(); });
}

bool MySQL_DatabaseMetaData::supportsConvert(int fromType, int toType)
{
    return (*executor_)([&]() { return impl_->supportsConvert(); });
}

bool MySQL_DatabaseMetaData::supportsCoreSQLGrammar()
{
    return (*executor_)([&]() { return impl_->supportsCoreSQLGrammar(); });
}

bool MySQL_DatabaseMetaData::supportsCorrelatedSubqueries()
{
    return (*executor_)([&]() { return impl_->supportsCorrelatedSubqueries(); });
}

bool MySQL_DatabaseMetaData::supportsDataDefinitionAndDataManipulationTransactions()
{
    return (*executor_)(
        [&]() { return impl_->supportsDataDefinitionAndDataManipulationTransactions(); });
}

bool MySQL_DatabaseMetaData::supportsDataManipulationTransactionsOnly()
{
    return (*executor_)([&]() { return impl_->supportsDataManipulationTransactionsOnly(); });
}

bool MySQL_DatabaseMetaData::supportsDifferentTableCorrelationNames()
{
    return (*executor_)([&]() { return impl_->supportsDifferentTableCorrelationNames(); });
}

bool MySQL_DatabaseMetaData::supportsExpressionsInOrderBy()
{
    return (*executor_)([&]() { return impl_->supportsExpressionsInOrderBy(); });
}

bool MySQL_DatabaseMetaData::supportsExtendedSQLGrammar()
{
    return (*executor_)([&]() { return impl_->supportsExtendedSQLGrammar(); });
}

bool MySQL_DatabaseMetaData::supportsFullOuterJoins()
{
    return (*executor_)([&]() { return impl_->supportsFullOuterJoins(); });
}

bool MySQL_DatabaseMetaData::supportsGetGeneratedKeys()
{
    return (*executor_)([&]() { return impl_->supportsGetGeneratedKeys(); });
}

bool MySQL_DatabaseMetaData::supportsGroupBy()
{
    return (*executor_)([&]() { return impl_->supportsGroupBy(); });
}

bool MySQL_DatabaseMetaData::supportsGroupByBeyondSelect()
{
    return (*executor_)([&]() { return impl_->supportsGroupByBeyondSelect(); });
}

bool MySQL_DatabaseMetaData::supportsGroupByUnrelated()
{
    return (*executor_)([&]() { return impl_->supportsGroupByUnrelated(); });
}

bool MySQL_DatabaseMetaData::supportsIntegrityEnhancementFacility()
{
    return (*executor_)([&]() { return impl_->supportsIntegrityEnhancementFacility(); });
}

bool MySQL_DatabaseMetaData::supportsLikeEscapeClause()
{
    return (*executor_)([&]() { return impl_->supportsLikeEscapeClause(); });
}

bool MySQL_DatabaseMetaData::supportsLimitedOuterJoins()
{
    return (*executor_)([&]() { return impl_->supportsLimitedOuterJoins(); });
}

bool MySQL_DatabaseMetaData::supportsMinimumSQLGrammar()
{
    return (*executor_)([&]() { return impl_->supportsMinimumSQLGrammar(); });
}

bool MySQL_DatabaseMetaData::supportsMixedCaseIdentifiers()
{
    return (*executor_)([&]() { return impl_->supportsMixedCaseIdentifiers(); });
}

bool MySQL_DatabaseMetaData::supportsMixedCaseQuotedIdentifiers()
{
    return (*executor_)([&]() { return impl_->supportsMixedCaseQuotedIdentifiers(); });
}

bool MySQL_DatabaseMetaData::supportsMultipleOpenResults()
{
    return (*executor_)([&]() { return impl_->supportsMultipleOpenResults(); });
}

bool MySQL_DatabaseMetaData::supportsMultipleResultSets()
{
    return (*executor_)([&]() { return impl_->supportsMultipleResultSets(); });
}

bool MySQL_DatabaseMetaData::supportsMultipleTransactions()
{
    return (*executor_)([&]() { return impl_->supportsMultipleTransactions(); });
}

bool MySQL_DatabaseMetaData::supportsNamedParameters()
{
    return (*executor_)([&]() { return impl_->supportsNamedParameters(); });
}

bool MySQL_DatabaseMetaData::supportsNonNullableColumns()
{
    return (*executor_)([&]() { return impl_->supportsNonNullableColumns(); });
}

bool MySQL_DatabaseMetaData::supportsOpenCursorsAcrossCommit()
{
    return (*executor_)([&]() { return impl_->supportsOpenCursorsAcrossCommit(); });
}

bool MySQL_DatabaseMetaData::supportsOpenCursorsAcrossRollback()
{
    return (*executor_)([&]() { return impl_->supportsOpenCursorsAcrossRollback(); });
}

bool MySQL_DatabaseMetaData::supportsOpenStatementsAcrossCommit()
{
    return (*executor_)([&]() { return impl_->supportsOpenStatementsAcrossCommit(); });
}

bool MySQL_DatabaseMetaData::supportsOpenStatementsAcrossRollback()
{
    return (*executor_)([&]() { return impl_->supportsOpenStatementsAcrossRollback(); });
}

bool MySQL_DatabaseMetaData::supportsOrderByUnrelated()
{
    return (*executor_)([&]() { return impl_->supportsOrderByUnrelated(); });
}

bool MySQL_DatabaseMetaData::supportsOuterJoins()
{
    return (*executor_)([&]() { return impl_->supportsOuterJoins(); });
}

bool MySQL_DatabaseMetaData::supportsPositionedDelete()
{
    return (*executor_)([&]() { return impl_->supportsPositionedDelete(); });
}

bool MySQL_DatabaseMetaData::supportsPositionedUpdate()
{
    return (*executor_)([&]() { return impl_->supportsPositionedUpdate(); });
}

bool MySQL_DatabaseMetaData::supportsResultSetConcurrency(int type, int concurrency)
{
    return (*executor_)([&]() { return impl_->supportsResultSetConcurrency(type, concurrency); });
}

bool MySQL_DatabaseMetaData::supportsResultSetHoldability(int holdability)
{
    return (*executor_)([&]() { return impl_->supportsResultSetHoldability(holdability); });
}

bool MySQL_DatabaseMetaData::supportsResultSetType(int type)
{
    return (*executor_)([&]() { return impl_->supportsResultSetType(type); });
}

bool MySQL_DatabaseMetaData::supportsSavepoints()
{
    return (*executor_)([&]() { return impl_->supportsSavepoints(); });
}

bool MySQL_DatabaseMetaData::supportsSchemasInDataManipulation()
{
    return (*executor_)([&]() { return impl_->supportsSchemasInDataManipulation(); });
}

bool MySQL_DatabaseMetaData::supportsSchemasInIndexDefinitions()
{
    return (*executor_)([&]() { return impl_->supportsSchemasInIndexDefinitions(); });
}

bool MySQL_DatabaseMetaData::supportsSchemasInPrivilegeDefinitions()
{
    return (*executor_)([&]() { return impl_->supportsSchemasInPrivilegeDefinitions(); });
}

bool MySQL_DatabaseMetaData::supportsSchemasInProcedureCalls()
{
    return (*executor_)([&]() { return impl_->supportsSchemasInProcedureCalls(); });
}

bool MySQL_DatabaseMetaData::supportsSchemasInTableDefinitions()
{
    return (*executor_)([&]() { return impl_->supportsSchemasInTableDefinitions(); });
}

bool MySQL_DatabaseMetaData::supportsSelectForUpdate()
{
    return (*executor_)([&]() { return impl_->supportsSelectForUpdate(); });
}

bool MySQL_DatabaseMetaData::supportsStatementPooling()
{
    return (*executor_)([&]() { return impl_->supportsStatementPooling(); });
}

bool MySQL_DatabaseMetaData::supportsStoredProcedures()
{
    return (*executor_)([&]() { return impl_->supportsStoredProcedures(); });
}

bool MySQL_DatabaseMetaData::supportsSubqueriesInComparisons()
{
    return (*executor_)([&]() { return impl_->supportsSubqueriesInComparisons(); });
}

bool MySQL_DatabaseMetaData::supportsSubqueriesInExists()
{
    return (*executor_)([&]() { return impl_->supportsSubqueriesInExists(); });
}

bool MySQL_DatabaseMetaData::supportsSubqueriesInIns()
{
    return (*executor_)([&]() { return impl_->supportsSubqueriesInIns(); });
}

bool MySQL_DatabaseMetaData::supportsSubqueriesInQuantifieds()
{
    return (*executor_)([&]() { return impl_->supportsSubqueriesInQuantifieds(); });
}

bool MySQL_DatabaseMetaData::supportsTableCorrelationNames()
{
    return (*executor_)([&]() { return impl_->supportsTableCorrelationNames(); });
}

bool MySQL_DatabaseMetaData::supportsTransactionIsolationLevel(int level)
{
    return (*executor_)([&]() { return impl_->supportsTransactionIsolationLevel(level); });
}

bool MySQL_DatabaseMetaData::supportsTransactions()
{
    return (*executor_)([&]() { return impl_->supportsTransactions(); });
}

bool MySQL_DatabaseMetaData::supportsTypeConversion()
{
    return (*executor_)([&]() { return impl_->supportsTypeConversion(); });
} /* SDBC */

bool MySQL_DatabaseMetaData::supportsUnion()
{
    return (*executor_)([&]() { return impl_->supportsUnion(); });
}

bool MySQL_DatabaseMetaData::supportsUnionAll()
{
    return (*executor_)([&]() { return impl_->supportsUnionAll(); });
}

bool MySQL_DatabaseMetaData::updatesAreDetected(int type)
{
    return (*executor_)([&]() { return impl_->updatesAreDetected(type); });
}

bool MySQL_DatabaseMetaData::usesLocalFilePerTable()
{
    return (*executor_)([&]() { return impl_->usesLocalFilePerTable(); });
}

bool MySQL_DatabaseMetaData::usesLocalFiles()
{
    return (*executor_)([&]() { return impl_->usesLocalFiles(); });
}

sql::ResultSet* MySQL_DatabaseMetaData::getSchemata(const sql::SQLString& catalogName)
{
    sql::ResultSet* rs = (*executor_)([&]() { return impl_->getSchemata(catalogName); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

sql::ResultSet* MySQL_DatabaseMetaData::getSchemaObjects(const sql::SQLString& catalogName,
                                                         const sql::SQLString& schemaName,
                                                         const sql::SQLString& objectType,
                                                         bool includingDdl,
                                                         const sql::SQLString& objectName,
                                                         const sql::SQLString& contextTableName)
{
    sql::ResultSet* rs = (*executor_)([&]() {
        return impl_->getSchemaObjects(
            catalogName, schemaName, objectType, includingDdl, objectName, contextTableName);
    });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

sql::ResultSet* MySQL_DatabaseMetaData::getSchemaObjectTypes()
{
    sql::ResultSet* rs = (*executor_)([&]() { return impl_->getSchemaObjectTypes(); });
    return new MySQL_ResultSet(nullptr, rs, executor_);
}

} // End of namespace mysql
} // End of namespace db
} // End of namespace fibio
