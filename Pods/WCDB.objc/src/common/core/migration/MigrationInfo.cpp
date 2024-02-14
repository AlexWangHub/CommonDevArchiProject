//
// Created by sanhuazhang on 2019/05/02
//

/*
 * Tencent is pleased to support the open source community by making
 * WCDB available.
 *
 * Copyright (C) 2017 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *       https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MigrationInfo.hpp"
#include "Assertion.hpp"
#include "StringView.hpp"

namespace WCDB {

#pragma mark - MigrationBaseInfo
MigrationBaseInfo::MigrationBaseInfo() = default;

MigrationBaseInfo::MigrationBaseInfo(const UnsafeStringView& database,
                                     const UnsafeStringView& table)
: m_database(database), m_table(table)
{
    WCTAssert(!m_database.empty());
    WCTAssert(!m_table.empty());
}

MigrationBaseInfo::~MigrationBaseInfo() = default;

bool MigrationBaseInfo::shouldMigrate() const
{
    return !m_table.empty() && !m_sourceTable.empty();
}

bool MigrationBaseInfo::isCrossDatabase() const
{
    return m_sourceDatabase != m_database;
}

const StringView& MigrationBaseInfo::getTable() const
{
    return m_table;
}

const StringView& MigrationBaseInfo::getDatabase() const
{
    return m_database;
}

const StringView& MigrationBaseInfo::getSourceTable() const
{
    return m_sourceTable;
}

const StringView& MigrationBaseInfo::getSourceDatabase() const
{
    return m_sourceDatabase;
}

const char* MigrationBaseInfo::getSchemaPrefix()
{
    static const char* s_schemaPrefix = "wcdb_migration_";
    return s_schemaPrefix;
}

Schema MigrationBaseInfo::getSchemaForDatabase(const UnsafeStringView& database)
{
    std::ostringstream stream;
    stream << getSchemaPrefix() << database.hash();
    return stream.str();
}

void MigrationBaseInfo::setSource(const UnsafeStringView& table, const UnsafeStringView& database)
{
    WCTRemedialAssert(!table.empty() && (table != m_table || database != m_database),
                      "Invalid migration source.",
                      return;);
    m_sourceTable = table;
    if (!database.empty()) {
        m_sourceDatabase = database;
    } else {
        m_sourceDatabase = m_database;
    }
}

#pragma mark - MigrationUserInfo
MigrationUserInfo::~MigrationUserInfo() = default;

StatementAttach MigrationUserInfo::getStatementForAttachingSchema() const
{
    WCTAssert(isCrossDatabase());
    return StatementAttach().attach(getSourceDatabase()).as(getSchemaForDatabase(getSourceDatabase()));
}

Schema MigrationUserInfo::getSchemaForSourceDatabase() const
{
    if (isCrossDatabase()) {
        return getSchemaForDatabase(getSourceDatabase());
    }
    return Schema::main();
}

#pragma mark - MigrationInfo
MigrationInfo::MigrationInfo(const MigrationUserInfo& userInfo,
                             const std::set<StringView>& uniqueColumns,
                             bool integerPrimaryKey)
: MigrationBaseInfo(userInfo), m_integerPrimaryKey(integerPrimaryKey)
{
    WCTAssert(shouldMigrate());
    WCTAssert(!uniqueColumns.empty());

    // Schema
    if (isCrossDatabase()) {
        m_schemaForSourceDatabase = getSchemaForDatabase(getSourceDatabase());

        m_statementForAttachingSchema
        = StatementAttach().attach(getSourceDatabase()).as(m_schemaForSourceDatabase);
    } else {
        m_schemaForSourceDatabase = Schema::main();
    }

    Column rowid = Column::rowid();
    Columns columns;
    columns.push_back(rowid);
    columns.insert(columns.end(), uniqueColumns.begin(), uniqueColumns.end());

    ResultColumns resultColumns;
    resultColumns.insert(resultColumns.begin(), columns.begin(), columns.end());

    TableOrSubquery sourceTableQuery
    = TableOrSubquery(getSourceTable()).schema(m_schemaForSourceDatabase);
    QualifiedTable qualifiedSourceTable
    = QualifiedTable(getSourceTable()).schema(m_schemaForSourceDatabase);

    // View
    {
        std::ostringstream stream;
        stream << getUnionedViewPrefix() << getTable();
        m_unionedView = StringView(stream.str());

        m_statementForCreatingUnionedView = StatementCreateView()
                                            .createView(m_unionedView)
                                            .temp()
                                            .ifNotExists()
                                            .columns(columns)
                                            .as(StatementSelect()
                                                .select(resultColumns)
                                                .from(TableOrSubquery(getTable()))
                                                .unionAll()
                                                .select(resultColumns)
                                                .from(sourceTableQuery));
    }

    // Migrate
    {
        OrderingTerm descendingRowid = OrderingTerm(rowid).order(Order::DESC);

        m_statementForMigratingOneRow = StatementInsert()
                                        .insertIntoTable(getTable())
                                        .orReplace()
                                        .columns(columns)
                                        .values(StatementSelect()
                                                .select(resultColumns)
                                                .from(sourceTableQuery)
                                                .order(descendingRowid)
                                                .limit(1));

        m_statementForDeletingMigratedOneRow = StatementDelete()
                                               .deleteFrom(qualifiedSourceTable)
                                               .orders(descendingRowid)
                                               .limit(1);
    }

    // Compatible
    {
        if (!m_integerPrimaryKey) {
            m_statementForSelectingMaxRowID
            = StatementSelect()
              .select(rowid.max() + 1)
              .from(TableOrSubquery(m_unionedView).schema(Schema::temp()));
        }

        m_statementForDeletingSpecifiedRow
        = StatementDelete().deleteFrom(qualifiedSourceTable).where(rowid == BindParameter(1));

        m_statementForDroppingSourceTable = StatementDropTable()
                                            .dropTable(getSourceTable())
                                            .schema(m_schemaForSourceDatabase)
                                            .ifExists();
    }
}

MigrationInfo::~MigrationInfo() = default;

#pragma mark - Schema
const Schema& MigrationInfo::getSchemaForSourceDatabase() const
{
    return m_schemaForSourceDatabase;
}

const StringView& MigrationInfo::getUnionedView() const
{
    return m_unionedView;
}

const StatementAttach& MigrationInfo::getStatementForAttachingSchema() const
{
    WCTAssert(isCrossDatabase());
    return m_statementForAttachingSchema;
}

StatementDetach MigrationInfo::getStatementForDetachingSchema(const Schema& schema)
{
    WCTAssert(schema.getDescription().hasPrefix(getSchemaPrefix()));
    return StatementDetach().detach(schema);
}

StatementPragma MigrationInfo::getStatementForSelectingDatabaseList()
{
    return StatementPragma().pragma(Pragma::databaseList());
}

#pragma mark - View
const char* MigrationInfo::getUnionedViewPrefix()
{
    return "wcdb_union_";
}

const StatementCreateView& MigrationInfo::getStatementForCreatingUnionedView() const
{
    return m_statementForCreatingUnionedView;
}

StatementCreateView
MigrationInfo::getStatementForCreatingUnionedView(const Columns& columns) const
{
    StatementCreateView createView = getStatementForCreatingUnionedView();
    Columns newColumns = columns;
    newColumns.push_back(Column::rowid());
    createView.columns(newColumns);
    ResultColumns resultColumns;
    resultColumns.insert(resultColumns.begin(), newColumns.begin(), newColumns.end());
    createView.syntax().select.getOrCreate().select.getOrCreate().resultColumns = resultColumns;
    for (auto& core : createView.syntax().select.getOrCreate().cores) {
        core.resultColumns = resultColumns;
    }
    return createView;
}

const StatementDropView
MigrationInfo::getStatementForDroppingUnionedView(const UnsafeStringView& unionedView)
{
    WCTAssert(unionedView.hasPrefix(getUnionedViewPrefix()));
    return StatementDropView().dropView(unionedView).schema(Schema::temp()).ifExists();
}

StatementSelect MigrationInfo::getStatementForSelectingUnionedView()
{
    Column name("name");
    Column type("type");
    StringView pattern = StringView::formatted("%s%%", getUnionedViewPrefix());
    return StatementSelect()
    .select(name)
    .from(TableOrSubquery::master().schema(Schema::temp()))
    .where(type == "view" && name.like(pattern));
}

#pragma mark - Migrate
const StatementInsert& MigrationInfo::getStatementForMigratingOneRow() const
{
    return m_statementForMigratingOneRow;
}

const StatementDelete& MigrationInfo::getStatementForDeletingMigratedOneRow() const
{
    return m_statementForDeletingMigratedOneRow;
}

const StatementDelete& MigrationInfo::getStatementForDeletingSpecifiedRow() const
{
    return m_statementForDeletingSpecifiedRow;
}

StatementInsert MigrationInfo::getStatementForMigrating(const Syntax::InsertSTMT& stmt) const
{
    StatementInsert statement(stmt);

    auto& syntax = statement.syntax();
    syntax.schema = Schema::main();
    syntax.table = getTable();
    WCTAssert(!syntax.isMultiWrite());

    auto& columns = syntax.columns;
    WCTAssert(!columns.empty());
    columns.insert(columns.end(), Column("rowid"));

    if (!syntax.expressionsValues.empty()) {
        auto& expressions = syntax.expressionsValues;
        WCTAssert(expressions.size() == 1);
        auto& values = *expressions.begin();
        int rowidIndexOfMigratingStatement = getRowIDIndexOfMigratingStatement();
        Expression rowid;
        if (rowidIndexOfMigratingStatement > 0) {
            rowid = BindParameter(rowidIndexOfMigratingStatement);
        } else {
            rowid = m_statementForSelectingMaxRowID;
        }
        values.insert(values.end(), rowid);
    }
    return statement;
}

int MigrationInfo::getRowIDIndexOfMigratingStatement() const
{
    if (m_integerPrimaryKey) {
        return SQLITE_MAX_VARIABLE_NUMBER;
    }
    return 0;
}

StatementUpdate
MigrationInfo::getStatementForLimitedUpdatingTable(const Statement& sourceStatement) const
{
    WCTAssert(sourceStatement.getType() == Syntax::Identifier::Type::UpdateSTMT);
    StatementUpdate statementUpdate((const StatementUpdate&) sourceStatement);

    Syntax::UpdateSTMT& updateSyntax = statementUpdate.syntax();
    StatementSelect select
    = StatementSelect()
      .select(Column::rowid())
      .from(TableOrSubquery(getUnionedView()).schema(Schema::temp()));

    Syntax::SelectSTMT& selectSyntax = select.syntax();
    Syntax::SelectCore& coreSyntax = selectSyntax.select.getOrCreate();

    coreSyntax.condition = updateSyntax.condition;
    updateSyntax.condition.getOrCreate().__is_valid = Syntax::Identifier::invalid;

    selectSyntax.orderingTerms = updateSyntax.orderingTerms;
    updateSyntax.orderingTerms.clear();

    selectSyntax.limit = updateSyntax.limit;
    selectSyntax.limitParameterType = updateSyntax.limitParameterType;
    selectSyntax.limitParameter = updateSyntax.limitParameter;
    updateSyntax.limit.getOrCreate().__is_valid = Syntax::Identifier::invalid;

    statementUpdate.where(Column::rowid().in(select));

    return statementUpdate;
}

StatementDelete
MigrationInfo::getStatementForLimitedDeletingFromTable(const Statement& sourceStatement) const
{
    WCTAssert(sourceStatement.getType() == Syntax::Identifier::Type::DeleteSTMT);
    StatementDelete statementDelete((const StatementDelete&) sourceStatement);

    Syntax::DeleteSTMT& deleteSyntax = statementDelete.syntax();
    StatementSelect select
    = StatementSelect()
      .select(Column::rowid())
      .from(TableOrSubquery(getUnionedView()).schema(Schema::temp()));

    Syntax::SelectSTMT& selectSyntax = select.syntax();
    Syntax::SelectCore& coreSyntax = selectSyntax.select.getOrCreate();

    coreSyntax.condition = deleteSyntax.condition;
    deleteSyntax.condition.getOrCreate().__is_valid = Syntax::Identifier::invalid;

    selectSyntax.orderingTerms = deleteSyntax.orderingTerms;
    deleteSyntax.orderingTerms.clear();

    selectSyntax.limit = deleteSyntax.limit;
    selectSyntax.limitParameterType = deleteSyntax.limitParameterType;
    selectSyntax.limitParameter = deleteSyntax.limitParameter;
    deleteSyntax.limit.getOrCreate().__is_valid = Syntax::Identifier::invalid;

    statementDelete.where(Column::rowid().in(select));

    return statementDelete;
}

StatementDelete
MigrationInfo::getStatementForDeletingFromTable(const Statement& sourceStatement) const
{
    WCTAssert(sourceStatement.getType() == Syntax::Identifier::Type::DropTableSTMT);

    const Syntax::DropTableSTMT& dropTableSyntax
    = ((const StatementDropTable&) sourceStatement).syntax();
    WCDB::QualifiedTable table(dropTableSyntax.table);
    table.syntax().schema = dropTableSyntax.schema;

    return StatementDelete().deleteFrom(table);
}

const StatementDropTable& MigrationInfo::getStatementForDroppingSourceTable() const
{
    return m_statementForDroppingSourceTable;
}

} // namespace WCDB
