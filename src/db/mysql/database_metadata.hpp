//
//  database_metadata.hpp
//  fibio
//
//  Created by Chen Xu on 15/09/11.
//  Copyright (c) 2015 0d0a.com. All rights reserved.
//

#ifndef fibio_db_mysql_database_metadata_hpp
#define fibio_db_mysql_database_metadata_hpp

#include <cppconn/metadata.h>

namespace fibio {
namespace db {
namespace mysql {

class MySQL_Connection;

class MySQL_DatabaseMetaData : public sql::DatabaseMetaData
{
public:
    virtual ~MySQL_DatabaseMetaData() override;

    virtual bool allProceduresAreCallable() override;

    virtual bool allTablesAreSelectable() override;

    virtual bool dataDefinitionCausesTransactionCommit() override;

    virtual bool dataDefinitionIgnoredInTransactions() override;

    virtual bool deletesAreDetected(int type) override;

    virtual bool doesMaxRowSizeIncludeBlobs() override;

    virtual sql::ResultSet* getAttributes(const sql::SQLString& catalog,
                                          const sql::SQLString& schemaPattern,
                                          const sql::SQLString& typeNamePattern,
                                          const sql::SQLString& attributeNamePattern) override;

    virtual sql::ResultSet* getBestRowIdentifier(const sql::SQLString& catalog,
                                                 const sql::SQLString& schema,
                                                 const sql::SQLString& table,
                                                 int scope,
                                                 bool nullable) override;

    virtual sql::ResultSet* getCatalogs() override;

    virtual const sql::SQLString& getCatalogSeparator() override;

    virtual const sql::SQLString& getCatalogTerm() override;

    virtual sql::ResultSet* getColumnPrivileges(const sql::SQLString& catalog,
                                                const sql::SQLString& schema,
                                                const sql::SQLString& table,
                                                const sql::SQLString& columnNamePattern) override;

    virtual sql::ResultSet* getColumns(const sql::SQLString& catalog,
                                       const sql::SQLString& schemaPattern,
                                       const sql::SQLString& tableNamePattern,
                                       const sql::SQLString& columnNamePattern) override;

    virtual sql::Connection* getConnection() override;

    virtual sql::ResultSet* getCrossReference(const sql::SQLString& primaryCatalog,
                                              const sql::SQLString& primarySchema,
                                              const sql::SQLString& primaryTable,
                                              const sql::SQLString& foreignCatalog,
                                              const sql::SQLString& foreignSchema,
                                              const sql::SQLString& foreignTable) override;

    virtual unsigned int getDatabaseMajorVersion() override;

    virtual unsigned int getDatabaseMinorVersion() override;

    virtual unsigned int getDatabasePatchVersion() override;

    virtual const sql::SQLString& getDatabaseProductName() override;

    virtual sql::SQLString getDatabaseProductVersion() override;

    virtual int getDefaultTransactionIsolation() override;

    virtual unsigned int getDriverMajorVersion() override;

    virtual unsigned int getDriverMinorVersion() override;

    virtual unsigned int getDriverPatchVersion() override;

    virtual const sql::SQLString& getDriverName() override;

    virtual const sql::SQLString& getDriverVersion() override;

    virtual sql::ResultSet* getExportedKeys(const sql::SQLString& catalog,
                                            const sql::SQLString& schema,
                                            const sql::SQLString& table) override;

    virtual const sql::SQLString& getExtraNameCharacters() override;

    virtual const sql::SQLString& getIdentifierQuoteString() override;

    virtual sql::ResultSet* getImportedKeys(const sql::SQLString& catalog,
                                            const sql::SQLString& schema,
                                            const sql::SQLString& table) override;

    virtual sql::ResultSet* getIndexInfo(const sql::SQLString& catalog,
                                         const sql::SQLString& schema,
                                         const sql::SQLString& table,
                                         bool unique,
                                         bool approximate) override;

    virtual unsigned int getCDBCMajorVersion() override;

    virtual unsigned int getCDBCMinorVersion() override;

    virtual unsigned int getMaxBinaryLiteralLength() override;

    virtual unsigned int getMaxCatalogNameLength() override;

    virtual unsigned int getMaxCharLiteralLength() override;

    virtual unsigned int getMaxColumnNameLength() override;

    virtual unsigned int getMaxColumnsInGroupBy() override;

    virtual unsigned int getMaxColumnsInIndex() override;

    virtual unsigned int getMaxColumnsInOrderBy() override;

    virtual unsigned int getMaxColumnsInSelect() override;

    virtual unsigned int getMaxColumnsInTable() override;

    virtual unsigned int getMaxConnections() override;

    virtual unsigned int getMaxCursorNameLength() override;

    virtual unsigned int getMaxIndexLength() override;

    virtual unsigned int getMaxProcedureNameLength() override;

    virtual unsigned int getMaxRowSize() override;

    virtual unsigned int getMaxSchemaNameLength() override;

    virtual unsigned int getMaxStatementLength() override;

    virtual unsigned int getMaxStatements() override;

    virtual unsigned int getMaxTableNameLength() override;

    virtual unsigned int getMaxTablesInSelect() override;

    virtual unsigned int getMaxUserNameLength() override;

    virtual const sql::SQLString& getNumericFunctions() override;

    virtual sql::ResultSet* getPrimaryKeys(const sql::SQLString& catalog,
                                           const sql::SQLString& schema,
                                           const sql::SQLString& table) override;

    virtual sql::ResultSet* getProcedureColumns(const sql::SQLString& catalog,
                                                const sql::SQLString& schemaPattern,
                                                const sql::SQLString& procedureNamePattern,
                                                const sql::SQLString& columnNamePattern) override;

    virtual sql::ResultSet* getProcedures(const sql::SQLString& catalog,
                                          const sql::SQLString& schemaPattern,
                                          const sql::SQLString& procedureNamePattern) override;

    virtual const sql::SQLString& getProcedureTerm() override;

    virtual int getResultSetHoldability() override;

    virtual sql::ResultSet* getSchemas() override;

    virtual const sql::SQLString& getSchemaTerm() override;

    virtual sql::ResultSet* getSchemaCollation(const sql::SQLString& catalog,
                                               const sql::SQLString& schemaPattern) override;

    virtual sql::ResultSet* getSchemaCharset(const sql::SQLString& catalog,
                                             const sql::SQLString& schemaPattern) override;

    virtual const sql::SQLString& getSearchStringEscape() override;

    virtual const sql::SQLString& getSQLKeywords() override;

    virtual int getSQLStateType() override;

    virtual const sql::SQLString& getStringFunctions() override;

    virtual sql::ResultSet* getSuperTables(const sql::SQLString& catalog,
                                           const sql::SQLString& schemaPattern,
                                           const sql::SQLString& tableNamePattern) override;

    virtual sql::ResultSet* getSuperTypes(const sql::SQLString& catalog,
                                          const sql::SQLString& schemaPattern,
                                          const sql::SQLString& typeNamePattern) override;

    virtual const sql::SQLString& getSystemFunctions() override;

    virtual sql::ResultSet* getTablePrivileges(const sql::SQLString& catalog,
                                               const sql::SQLString& schemaPattern,
                                               const sql::SQLString& tableNamePattern) override;

    virtual sql::ResultSet* getTables(const sql::SQLString& catalog,
                                      const sql::SQLString& schemaPattern,
                                      const sql::SQLString& tableNamePattern,
                                      std::list<sql::SQLString>& types) override;

    virtual sql::ResultSet* getTableTypes() override;

    virtual sql::ResultSet* getTableCollation(const sql::SQLString& catalog,
                                              const sql::SQLString& schemaPattern,
                                              const sql::SQLString& tableNamePattern) override;

    virtual sql::ResultSet* getTableCharset(const sql::SQLString& catalog,
                                            const sql::SQLString& schemaPattern,
                                            const sql::SQLString& tableNamePattern) override;

    virtual const sql::SQLString& getTimeDateFunctions() override;

    virtual sql::ResultSet* getTypeInfo() override;

    virtual sql::ResultSet* getUDTs(const sql::SQLString& catalog,
                                    const sql::SQLString& schemaPattern,
                                    const sql::SQLString& typeNamePattern,
                                    std::list<int>& types) override;

    virtual sql::SQLString getURL() override;

    virtual sql::SQLString getUserName() override;

    virtual sql::ResultSet* getVersionColumns(const sql::SQLString& catalog,
                                              const sql::SQLString& schema,
                                              const sql::SQLString& table) override;

    virtual bool insertsAreDetected(int type) override;

    virtual bool isCatalogAtStart() override;

    virtual bool isReadOnly() override;

    virtual bool locatorsUpdateCopy() override;

    virtual bool nullPlusNonNullIsNull() override;

    virtual bool nullsAreSortedAtEnd() override;

    virtual bool nullsAreSortedAtStart() override;

    virtual bool nullsAreSortedHigh() override;

    virtual bool nullsAreSortedLow() override;

    virtual bool othersDeletesAreVisible(int type) override;

    virtual bool othersInsertsAreVisible(int type) override;

    virtual bool othersUpdatesAreVisible(int type) override;

    virtual bool ownDeletesAreVisible(int type) override;

    virtual bool ownInsertsAreVisible(int type) override;

    virtual bool ownUpdatesAreVisible(int type) override;

    virtual bool storesLowerCaseIdentifiers() override;

    virtual bool storesLowerCaseQuotedIdentifiers() override;

    virtual bool storesMixedCaseIdentifiers() override;

    virtual bool storesMixedCaseQuotedIdentifiers() override;

    virtual bool storesUpperCaseIdentifiers() override;

    virtual bool storesUpperCaseQuotedIdentifiers() override;

    virtual bool supportsAlterTableWithAddColumn() override;

    virtual bool supportsAlterTableWithDropColumn() override;

    virtual bool supportsANSI92EntryLevelSQL() override;

    virtual bool supportsANSI92FullSQL() override;

    virtual bool supportsANSI92IntermediateSQL() override;

    virtual bool supportsBatchUpdates() override;

    virtual bool supportsCatalogsInDataManipulation() override;

    virtual bool supportsCatalogsInIndexDefinitions() override;

    virtual bool supportsCatalogsInPrivilegeDefinitions() override;

    virtual bool supportsCatalogsInProcedureCalls() override;

    virtual bool supportsCatalogsInTableDefinitions() override;

    virtual bool supportsColumnAliasing() override;

    virtual bool supportsConvert() override;

    virtual bool supportsConvert(int fromType, int toType) override;

    virtual bool supportsCoreSQLGrammar() override;

    virtual bool supportsCorrelatedSubqueries() override;

    virtual bool supportsDataDefinitionAndDataManipulationTransactions() override;

    virtual bool supportsDataManipulationTransactionsOnly() override;

    virtual bool supportsDifferentTableCorrelationNames() override;

    virtual bool supportsExpressionsInOrderBy() override;

    virtual bool supportsExtendedSQLGrammar() override;

    virtual bool supportsFullOuterJoins() override;

    virtual bool supportsGetGeneratedKeys() override;

    virtual bool supportsGroupBy() override;

    virtual bool supportsGroupByBeyondSelect() override;

    virtual bool supportsGroupByUnrelated() override;

    virtual bool supportsIntegrityEnhancementFacility() override;

    virtual bool supportsLikeEscapeClause() override;

    virtual bool supportsLimitedOuterJoins() override;

    virtual bool supportsMinimumSQLGrammar() override;

    virtual bool supportsMixedCaseIdentifiers() override;

    virtual bool supportsMixedCaseQuotedIdentifiers() override;

    virtual bool supportsMultipleOpenResults() override;

    virtual bool supportsMultipleResultSets() override;

    virtual bool supportsMultipleTransactions() override;

    virtual bool supportsNamedParameters() override;

    virtual bool supportsNonNullableColumns() override;

    virtual bool supportsOpenCursorsAcrossCommit() override;

    virtual bool supportsOpenCursorsAcrossRollback() override;

    virtual bool supportsOpenStatementsAcrossCommit() override;

    virtual bool supportsOpenStatementsAcrossRollback() override;

    virtual bool supportsOrderByUnrelated() override;

    virtual bool supportsOuterJoins() override;

    virtual bool supportsPositionedDelete() override;

    virtual bool supportsPositionedUpdate() override;

    virtual bool supportsResultSetConcurrency(int type, int concurrency) override;

    virtual bool supportsResultSetHoldability(int holdability) override;

    virtual bool supportsResultSetType(int type) override;

    virtual bool supportsSavepoints() override;

    virtual bool supportsSchemasInDataManipulation() override;

    virtual bool supportsSchemasInIndexDefinitions() override;

    virtual bool supportsSchemasInPrivilegeDefinitions() override;

    virtual bool supportsSchemasInProcedureCalls() override;

    virtual bool supportsSchemasInTableDefinitions() override;

    virtual bool supportsSelectForUpdate() override;

    virtual bool supportsStatementPooling() override;

    virtual bool supportsStoredProcedures() override;

    virtual bool supportsSubqueriesInComparisons() override;

    virtual bool supportsSubqueriesInExists() override;

    virtual bool supportsSubqueriesInIns() override;

    virtual bool supportsSubqueriesInQuantifieds() override;

    virtual bool supportsTableCorrelationNames() override;

    virtual bool supportsTransactionIsolationLevel(int level) override;

    virtual bool supportsTransactions() override;

    virtual bool supportsTypeConversion() override; /* SDBC */

    virtual bool supportsUnion() override;

    virtual bool supportsUnionAll() override;

    virtual bool updatesAreDetected(int type) override;

    virtual bool usesLocalFilePerTable() override;

    virtual bool usesLocalFiles() override;

    virtual sql::ResultSet* getSchemata(const sql::SQLString& catalogName = "") override;

    virtual sql::ResultSet* getSchemaObjects(const sql::SQLString& catalogName = "",
                                             const sql::SQLString& schemaName = "",
                                             const sql::SQLString& objectType = "",
                                             bool includingDdl = true,
                                             const sql::SQLString& objectName = "",
                                             const sql::SQLString& contextTableName = "") override;

    virtual sql::ResultSet* getSchemaObjectTypes() override;

private:
    MySQL_DatabaseMetaData(MySQL_Connection* conn,
                           sql::DatabaseMetaData* impl,
                           std::shared_ptr<fibers::foreign_thread_pool> executor);
    MySQL_Connection* conn_;
    sql::DatabaseMetaData* impl_;
    std::shared_ptr<fibers::foreign_thread_pool> executor_;
    friend class MySQL_Connection;
};

} // End of namespace mysql
} // End of namespace db
} // End of namespace fibio

#endif // !defined(fibio_db_mysql_database_metadata_hpp)
