//
// Created by sanhuazhang on 2019/05/23
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

#include "MigrateHandle.hpp"
#include "Assertion.hpp"
#include "CoreConst.h"
#include "Time.hpp"
#include <cmath>

namespace WCDB {

MigrateHandle::MigrateHandle()
: m_migratingInfo(nullptr)
, m_migrateStatement(getStatement())
, m_removeMigratedStatement(getStatement())
, m_samplePointing(0)
{
}

MigrateHandle::~MigrateHandle()
{
    finalizeMigrationStatement();
    returnStatement(m_migrateStatement);
    returnStatement(m_removeMigratedStatement);
}

bool MigrateHandle::reAttach(const UnsafeStringView& newPath, const Schema& newSchema)
{
    WCTAssert(!isInTransaction());
    WCTAssert(!isPrepared());

    bool succeed = true;
    if (!m_attached.syntax().isTargetingSameSchema(newSchema.syntax())) {
        succeed = detach() && attach(newPath, newSchema);
    }
    m_migratingInfo = nullptr;
    finalizeMigrationStatement();
    return succeed;
}

bool MigrateHandle::attach(const UnsafeStringView& newPath, const Schema& newSchema)
{
    WCTAssert(!isInTransaction());
    WCTAssert(!isPrepared());
    WCTAssert(m_attached.syntax().isMain());

    bool succeed = true;
    if (!newSchema.syntax().isMain()) {
        UnsafeData cipherKey = getCipherKey();
        StatementAttach attach = StatementAttach().attach(newPath).as(newSchema);
        if (cipherKey.size() == 0) {
            succeed = executeStatement(attach);
        } else {
            attach.key(WCDB::BindParameter(1));
            HandleStatement handleStatement(this);
            succeed = handleStatement.prepare(attach);
            if (succeed) {
                handleStatement.bindBLOB(cipherKey);
                succeed = handleStatement.step();
                handleStatement.finalize();
            }
        }
        if (succeed) {
            m_attached = newSchema;
        }
    }
    return succeed;
}

bool MigrateHandle::detach()
{
    WCTAssert(!isInTransaction());
    WCTAssert(!isPrepared());

    bool succeed = true;
    if (!m_attached.syntax().isMain()) {
        succeed = execute(WCDB::StatementDetach().detach(m_attached));
        if (succeed) {
            m_attached = Schema::main();
        }
    }
    return succeed;
}

#pragma mark - Stepper
Optional<std::set<StringView>> MigrateHandle::getAllTables()
{
    Column name("name");
    Column type("type");
    StringView pattern
    = StringView::formatted("%s%%", Syntax::builtinTablePrefix.data());
    return getValues(StatementSelect()
                     .select(name)
                     .from(TableOrSubquery::master())
                     .where(type == "table" && name.notLike(pattern)),
                     0);
}

bool MigrateHandle::dropSourceTable(const MigrationInfo* info)
{
    WCTAssert(info != nullptr);
    bool succeed = false;
    if (reAttach(info->getSourceDatabase(), info->getSchemaForSourceDatabase())) {
        m_migratingInfo = info;
        succeed = execute(m_migratingInfo->getStatementForDroppingSourceTable());
    }
    return succeed;
}

Optional<bool> MigrateHandle::migrateRows(const MigrationInfo* info)
{
    WCTAssert(info != nullptr);
    auto exists = tableExists(info->getTable());
    if (!exists.succeed()) {
        return NullOpt;
    }

    if (!exists.value()) {
        return true;
    }

    if (m_migratingInfo != info) {
        if (!reAttach(info->getSourceDatabase(), info->getSchemaForSourceDatabase())) {
            return NullOpt;
        }
        m_migratingInfo = info;
    }

    if (!m_migrateStatement->isPrepared()
        && !m_migrateStatement->prepare(m_migratingInfo->getStatementForMigratingOneRow())) {
        return NullOpt;
    }

    if (!m_removeMigratedStatement->isPrepared()
        && !m_removeMigratedStatement->prepare(
        m_migratingInfo->getStatementForDeletingMigratedOneRow())) {
        return NullOpt;
    }

    double timeIntervalWithinTransaction = calculateTimeIntervalWithinTransaction();
    SteadyClock beforeTransaction = SteadyClock::now();
    Optional<bool> migrated;
    if (runTransaction([&migrated, &beforeTransaction, &timeIntervalWithinTransaction, this](
                       InnerHandle*) -> bool {
            double cost = 0;
            do {
                migrated = migrateRow();
                cost = SteadyClock::timeIntervalSinceSteadyClockToNow(beforeTransaction);
            } while (migrated.succeed() && !migrated.value()
                     && cost < timeIntervalWithinTransaction);
            timeIntervalWithinTransaction = cost;
            return migrated.succeed();
        })) {
        // update only if succeed
        double timeIntervalWholeTranscation
        = SteadyClock::timeIntervalSinceSteadyClockToNow(beforeTransaction);
        addSample(timeIntervalWithinTransaction, timeIntervalWholeTranscation);

        WCTAssert(migrated.succeed());
        return migrated;
    }
    return NullOpt;
}

Optional<bool> MigrateHandle::migrateRow()
{
    WCTAssert(m_migrateStatement->isPrepared() && m_removeMigratedStatement->isPrepared());
    WCTAssert(isInTransaction());
    Optional<bool> migrated;
    m_migrateStatement->reset();
    m_removeMigratedStatement->reset();
    if (m_migrateStatement->step()) {
        if (getChanges() != 0) {
            if (m_removeMigratedStatement->step()) {
                migrated = false;
            }
        } else {
            migrated = true;
        }
    }
    return migrated;
}

void MigrateHandle::finalizeMigrationStatement()
{
    m_migrateStatement->finalize();
    m_removeMigratedStatement->finalize();
}

#pragma mark - Sample
MigrateHandle::Sample::Sample()
: timeIntervalWithinTransaction(0), timeIntervalWholeTransaction(0)
{
}

void MigrateHandle::addSample(double timeIntervalWithinTransaction, double timeIntervalForWholeTransaction)
{
    WCTAssert(timeIntervalWithinTransaction > 0);
    WCTAssert(timeIntervalForWholeTransaction > 0);
    WCTAssert(m_samplePointing < numberOfSamples);
    WCTAssert(timeIntervalForWholeTransaction > timeIntervalWithinTransaction);

    Sample& sample = m_samples[m_samplePointing];
    sample.timeIntervalWithinTransaction = timeIntervalWithinTransaction;
    sample.timeIntervalWholeTransaction = timeIntervalForWholeTransaction;
    ++m_samplePointing;
    if (m_samplePointing >= numberOfSamples) {
        m_samplePointing = 0;
    }
}

double MigrateHandle::calculateTimeIntervalWithinTransaction() const
{
    double totalTimeIntervalWithinTransaction = 0;
    double totalTimeIntervalWholeTransaction = 0;
    for (const auto& sample : m_samples) {
        if (sample.timeIntervalWithinTransaction > 0
            && sample.timeIntervalWholeTransaction > 0) {
            totalTimeIntervalWithinTransaction += sample.timeIntervalWithinTransaction;
            totalTimeIntervalWholeTransaction += sample.timeIntervalWholeTransaction;
        }
    }
    double timeIntervalWithinTransaction = MigrateMaxExpectingDuration * totalTimeIntervalWithinTransaction
                                           / totalTimeIntervalWholeTransaction;
    if (timeIntervalWithinTransaction > MigrateMaxExpectingDuration
        || timeIntervalWithinTransaction <= 0 || std::isnan(timeIntervalWithinTransaction)) {
        timeIntervalWithinTransaction = MigrateMaxInitializeDuration;
    }
    return timeIntervalWithinTransaction;
}

#pragma mark - Info Initializer
Optional<bool> MigrateHandle::sourceTableExists(const MigrationUserInfo& userInfo)
{
    Schema schema = userInfo.getSchemaForSourceDatabase();
    if (!reAttach(userInfo.getSourceDatabase(), schema)) {
        return NullOpt;
    }
    return tableExists(schema, userInfo.getSourceTable());
}

Optional<std::pair<bool, std::set<StringView>>>
MigrateHandle::getColumnsOfUserInfo(const MigrationUserInfo& userInfo)
{
    auto exists = tableExists(Schema::main(), userInfo.getTable());
    if (!exists.succeed()) {
        return NullOpt;
    }
    bool integerPrimary = false;
    std::set<StringView> names;
    if (exists.value()) {
        auto optionalMetas = getTableMeta(Schema::main(), userInfo.getTable());
        if (!optionalMetas.succeed()) {
            return NullOpt;
        }
        auto& metas = optionalMetas.value();
        integerPrimary = ColumnMeta::getIndexOfIntegerPrimary(metas) >= 0;
        for (const auto& meta : metas) {
            names.emplace(meta.name);
        }
    }
    return std::make_pair(integerPrimary, names);
}

const StringView& MigrateHandle::getDatabasePath() const
{
    return getPath();
}

} // namespace WCDB
