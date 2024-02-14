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

#include "Core.h"
#include "AutoMigrateConfig.hpp"
#include "BusyRetryConfig.hpp"
#include "FTS5AuxiliaryFunctionTemplate.hpp"
#include "FTSConst.h"
#include "FileManager.hpp"
#include "Global.hpp"
#include "Notifier.hpp"
#include "OneOrBinaryTokenizer.hpp"
#include "PinyinTokenizer.hpp"
#include "SQLite.h"
#include "StringView.hpp"
#include "SubstringMatchInfo.hpp"

namespace WCDB {

#pragma mark - Core
Core& Core::shared()
{
    static Core* s_core = new Core;
    return *s_core;
}

Core::Core()
// Database
: m_databasePool(this)
, m_tokenizerModules(std::make_shared<TokenizerModules>())
, m_auxiliaryFunctionModules(std::make_shared<AuxiliaryFunctionModules>())
, m_operationQueue(std::make_shared<OperationQueue>(OperationQueueName, this))
// Checkpoint
, m_autoCheckpointConfig(std::make_shared<AutoCheckpointConfig>(m_operationQueue))
// Backup
, m_autoBackupConfig(std::make_shared<AutoBackupConfig>(m_operationQueue))
// Migration
, m_autoMigrateConfig(std::make_shared<AutoMigrateConfig>(m_operationQueue))
// Trace
, m_globalSQLTraceConfig(std::make_shared<ShareableSQLTraceConfig>())
, m_globalPerformanceTraceConfig(std::make_shared<ShareablePerformanceTraceConfig>())
//Merge
, m_AutoMergeFTSConfig(std::make_shared<AutoMergeFTSIndexConfig>(m_operationQueue))
// Config
, m_configs({
  { StringView(GlobalSQLTraceConfigName), m_globalSQLTraceConfig, Configs::Priority::Highest },
  { StringView(GlobalPerformanceTraceConfigName), m_globalPerformanceTraceConfig, Configs::Priority::Highest },
  { StringView(BusyRetryConfigName), std::make_shared<BusyRetryConfig>(), Configs::Priority::Highest },
  { StringView(BasicConfigName), std::make_shared<BasicConfig>(), Configs::Priority::Higher },
  })
{
    Global::initialize();

    Global::shared().setNotificationForLog(
    NotifierLoggerName,
    std::bind(&Core::globalLog, this, std::placeholders::_1, std::placeholders::_2));

    Notifier::shared().setNotificationForPreprocessing(
    NotifierPreprocessorName,
    std::bind(&Core::preprocessError, this, std::placeholders::_1));

    m_operationQueue->run();

    //config FTS
    registerTokenizer(BuiltinTokenizer::OneOrBinary,
                      FTS3TokenizerModuleTemplate<OneOrBinaryTokenizer>::specialize());
    registerTokenizer(BuiltinTokenizer::LegacyOneOrBinary,
                      FTS3TokenizerModuleTemplate<OneOrBinaryTokenizer>::specialize());
    registerTokenizer(
    BuiltinTokenizer::Verbatim,
    FTS5TokenizerModuleTemplate<OneOrBinaryTokenizer>::specializeWithContext(nullptr));
    registerTokenizer(
    BuiltinTokenizer::Pinyin,
    FTS5TokenizerModuleTemplate<PinyinTokenizer>::specializeWithContext(nullptr));

    registerAuxiliaryFunction(
    BuiltinAuxiliaryFunction::SubstringMatchInfo,
    FTS5AuxiliaryFunctionTemplate<SubstringMatchInfo>::specializeWithContext(nullptr));
}

Core::~Core()
{
    Global::shared().setNotificationForLog(NotifierLoggerName, nullptr);
    Notifier::shared().setNotificationForPreprocessing(NotifierPreprocessorName, nullptr);
}

#pragma mark - Database
RecyclableDatabase Core::getOrCreateDatabase(const UnsafeStringView& path)
{
    return m_databasePool.getOrCreate(path);
}

void Core::purgeDatabasePool()
{
    m_databasePool.purge();
}

void Core::databaseDidCreate(InnerDatabase* database)
{
    WCTAssert(database != nullptr);

    database->setConfigs(m_configs);

    enableAutoCheckpoint(database, true);
}

void Core::setThreadedDatabase(const UnsafeStringView& path)
{
    m_associateDatabases.getOrCreate() = path;
}

void Core::preprocessError(Error& error)
{
    auto& infos = error.infos;

    if (error.getPath().size() == 0 && m_associateDatabases.getOrCreate().size() > 0) {
        error.infos.insert_or_assign(ErrorStringKeyPath, m_associateDatabases.getOrCreate());
    }
    if (error.getPath().length() == 0 && error.getAssociatePath().length() > 0) {
        error.infos.insert_or_assign(ErrorStringKeyPath, error.getAssociatePath());
        error.infos.erase(ErrorStringKeyAssociatePath);
    }
    if (error.getPath().compare(error.getAssociatePath()) == 0) {
        error.infos.erase(ErrorStringKeyAssociatePath);
    }

    auto iter = infos.find(UnsafeStringView(ErrorStringKeyPath));
    if (iter != infos.end() && iter->second.getType() == Value::Type::Text) {
        auto tag = m_databasePool.getTag(iter->second.textValue());
        if (tag.isValid()) {
            error.infos.insert_or_assign(ErrorIntKeyTag, (long) tag);
        }
    }
}

#pragma mark - Tokenizer
void Core::registerTokenizer(const UnsafeStringView& name, const TokenizerModule& module)
{
    m_tokenizerModules->add(name, module);
}

bool Core::tokenizerExists(const UnsafeStringView& name) const
{
    return m_tokenizerModules->get(name) != nullptr;
}

std::shared_ptr<Config> Core::tokenizerConfig(const UnsafeStringView& tokenizeName)
{
    return std::make_shared<TokenizerConfig>(tokenizeName, m_tokenizerModules);
}

#pragma mark - AuxiliaryFunction
void Core::registerAuxiliaryFunction(const UnsafeStringView& name,
                                     const FTS5AuxiliaryFunctionModule& module)
{
    m_auxiliaryFunctionModules->add(name, module);
}

bool Core::auxiliaryFunctionExists(const UnsafeStringView& name) const
{
    return m_auxiliaryFunctionModules->get(name) != nullptr;
}

std::shared_ptr<Config>
Core::auxiliaryFunctionConfig(const UnsafeStringView& auxiliaryFunctionName)
{
    return std::make_shared<AuxiliaryFunctionConfig>(
    auxiliaryFunctionName, m_auxiliaryFunctionModules);
}

#pragma mark - Operation
Optional<bool> Core::migrationShouldBeOperated(const UnsafeStringView& path)
{
    RecyclableDatabase database = m_databasePool.getOrCreate(path);
    Optional<bool> done = false; // mark as no error if database is not referenced.
    if (database != nullptr) {
        done = database->stepMigration(true);
    }
    return done;
}

void Core::backupShouldBeOperated(const UnsafeStringView& path)
{
    RecyclableDatabase database = m_databasePool.getOrCreate(path);
    if (database != nullptr) {
        database->backup(true);
    }
}

void Core::checkpointShouldBeOperated(const UnsafeStringView& path)
{
    RecyclableDatabase database = m_databasePool.getOrCreate(path);
    if (database != nullptr) {
        database->checkpoint(true);
    }
}

void Core::integrityShouldBeChecked(const UnsafeStringView& path)
{
    RecyclableDatabase database = m_databasePool.getOrCreate(path);
    if (database != nullptr) {
        database->checkIntegrity(true);

        std::set<StringView> sourcePaths = database->getPathsOfSourceDatabases();
        for (const UnsafeStringView& sourcePath : sourcePaths) {
            RecyclableDatabase sourceDatabase = m_databasePool.getOrCreate(sourcePath);
            if (sourceDatabase != nullptr) {
                sourceDatabase->checkIntegrity(true);
            }
        }
    }
}

void Core::purgeShouldBeOperated()
{
    purgeDatabasePool();
}

bool Core::isFileObservedCorrupted(const UnsafeStringView& path)
{
    return m_operationQueue->isFileObservedCorrupted(path);
}

void Core::setNotificationWhenDatabaseCorrupted(const UnsafeStringView& path,
                                                const CorruptedNotification& notification)
{
    OperationQueue::CorruptionNotification underlyingNotification = nullptr;
    if (notification != nullptr) {
        underlyingNotification
        = [this, notification](const UnsafeStringView& path, uint32_t corruptedIdentifier) {
              RecyclableDatabase database = m_databasePool.getOrCreate(path);
              if (database == nullptr) {
                  return;
              }
              database->blockade();
              do {
                  auto exists = FileManager::fileExists(path);
                  if (!exists.succeed()) {
                      // I/O error
                      break;
                  }
                  if (!exists.value()) {
                      // it's already not existing
                      break;
                  }
                  auto identifier = FileManager::getFileIdentifier(path);
                  if (!identifier.succeed()) {
                      // I/O error
                      break;
                  }
                  if (identifier.value() != corruptedIdentifier) {
                      // file is changed.
                      break;
                  }
                  notification(database.get());
              } while (false);
              database->unblockade();
          };
    }
    m_operationQueue->setNotificationWhenCorrupted(path, underlyingNotification);
}

#pragma mark - Checkpoint
void Core::enableAutoCheckpoint(InnerDatabase* database, bool enable)
{
    if (enable) {
        database->setConfig(
        AutoCheckpointConfigName, m_autoCheckpointConfig, Configs::Priority::Highest);
        m_operationQueue->registerAsRequiredCheckpoint(database->getPath());
    } else {
        database->removeConfig(AutoCheckpointConfigName);
        m_operationQueue->registerAsNoCheckpointRequired(database->getPath());
    }
}

#pragma mark - Backup
void Core::enableAutoBackup(InnerDatabase* database, bool enable)
{
    WCTAssert(database != nullptr);
    if (enable) {
        database->setConfig(
        AutoBackupConfigName, m_autoBackupConfig, WCDB::Configs::Priority::Highest);
        m_operationQueue->registerAsRequiredBackup(database->getPath());
    } else {
        database->removeConfig(AutoBackupConfigName);
        m_operationQueue->registerAsNoBackupRequired(database->getPath());
    }
}

#pragma mark - Migration
void Core::enableAutoMigration(InnerDatabase* database, bool enable)
{
    WCTAssert(database != nullptr);
    if (enable) {
        database->setConfig(
        AutoMigrateConfigName, m_autoMigrateConfig, WCDB::Configs::Priority::Highest);
        m_operationQueue->registerAsRequiredMigration(database->getPath());
        m_operationQueue->asyncMigrate(database->getPath());
    } else {
        database->removeConfig(AutoMigrateConfigName);
        m_operationQueue->registerAsNoMigrationRequired(database->getPath());
    }
}

#pragma mark - Merge FTS Index
void Core::enableAutoMergeFTSIndex(InnerDatabase* database, bool enable)
{
    WCTAssert(database != nullptr);
    if (enable) {
        database->setConfig(AutoMergeFTSIndexConfigName,
                            m_AutoMergeFTSConfig,
                            WCDB::Configs::Priority::Highest);
        m_operationQueue->registerAsRequiredMergeFTSIndex(database->getPath());
        m_operationQueue->asyncMergeFTSIndex(database->getPath(), nullptr, nullptr);
    } else {
        database->removeConfig(AutoMergeFTSIndexConfigName);
        m_operationQueue->registerAsNoMergeFTSIndexRequired(database->getPath());
    }
}

Optional<bool> Core::mergeFTSIndexShouldBeOperated(const UnsafeStringView& path,
                                                   TableArray newTables,
                                                   TableArray modifiedTables)
{
    RecyclableDatabase database = m_databasePool.getOrCreate(path);
    Optional<bool> done = false; // mark as no error if database is not referenced.
    if (database != nullptr) {
        done = database->mergeFTSIndex(newTables, modifiedTables);
    }
    return done;
}

#pragma mark - Trace
void Core::globalLog(int rc, const char* message)
{
    if (Error::rc2c(rc) == Error::Code::Warning) {
        Error error;
        error.setSQLiteCode(rc, message);
        error.level = Error::Level::Warning;
        Notifier::shared().notify(error);
    }
}

void Core::setNotificationForSQLGLobalTraced(const ShareableSQLTraceConfig::Notification& notification)
{
    static_cast<ShareableSQLTraceConfig*>(m_globalSQLTraceConfig.get())->setNotification(notification);
}

void Core::setNotificationWhenPerformanceGlobalTraced(const ShareablePerformanceTraceConfig::Notification& notification)
{
    static_cast<ShareablePerformanceTraceConfig*>(m_globalPerformanceTraceConfig.get())
    ->setNotification(notification);
}

void Core::setNotificationWhenErrorTraced(const Notifier::Callback& notification)
{
    if (notification != nullptr) {
        Notifier::shared().setNotification(
        std::numeric_limits<int>::min(), WCDB::NotifierLoggerName, notification);
    } else {
        Notifier::shared().unsetNotification(WCDB::NotifierLoggerName);
    }
}

void Core::setNotificationWhenErrorTraced(const UnsafeStringView& path,
                                          const Notifier::Callback& notification)
{
    StringView notifierKey
    = StringView::formatted("%s_%s", NotifierLoggerName.data(), path.data());
    if (notification != nullptr) {
        StringView catchedPath = StringView(path);
        Notifier::Callback realNotification = [=](const Error& error) {
            if (error.getPath().length() == 0 || error.getPath().compare(catchedPath) == 0) {
                notification(error);
            }
        };
        Notifier::shared().setNotification(
        std::numeric_limits<int>::min() + 1, notifierKey, realNotification);
    } else {
        Notifier::shared().unsetNotification(notifierKey);
    }
}

#pragma mark - Integrity

void Core::skipIntegrityCheck(const UnsafeStringView& path)
{
    m_operationQueue->skipIntegrityCheck(path);
}

#pragma mark - Config
void Core::setABTestConfig(const UnsafeStringView& configName, const UnsafeStringView& configValue)
{
    LockGuard memoryGuard(m_memory);
    m_abtestConfig[configName] = configValue;
}

void Core::removeABTestConfig(const UnsafeStringView& configName)
{
    LockGuard memoryGuard(m_memory);
    m_abtestConfig.erase(configName);
}

Optional<UnsafeStringView> Core::getABTestConfig(const UnsafeStringView& configName)
{
    SharedLockGuard memoryGuard(m_memory);
    if (m_abtestConfig.find(configName) != m_abtestConfig.end()) {
        return m_abtestConfig[configName];
    }
    return NullOpt;
}

void Core::setDefaultCipherConfiguration(int version)
{
    switch (version) {
    case 1:
        sqlcipher_set_default_hmac_algorithm(0);
        sqlcipher_set_default_kdf_algorithm(0);
        sqlcipher_set_default_kdf_iter(4000);
        sqlcipher_set_default_use_hmac(0);
        break;

    case 2:
        sqlcipher_set_default_hmac_algorithm(0);
        sqlcipher_set_default_kdf_algorithm(0);
        sqlcipher_set_default_kdf_iter(4000);
        sqlcipher_set_default_use_hmac(1);
        break;

    case 3:
        sqlcipher_set_default_hmac_algorithm(0);
        sqlcipher_set_default_kdf_algorithm(0);
        sqlcipher_set_default_kdf_iter(64000);
        sqlcipher_set_default_use_hmac(1);
        break;

    default:
        sqlcipher_set_default_hmac_algorithm(2);
        sqlcipher_set_default_kdf_algorithm(2);
        sqlcipher_set_default_kdf_iter(256000);
        sqlcipher_set_default_use_hmac(1);
        break;
    }
}

bool Core::setDefaultTemporaryDirectory(const UnsafeStringView& dir)
{
    if (dir.length() > 0) {
        if (!FileManager::createDirectoryWithIntermediateDirectories(dir)) {
            return false;
        }
    }
    sqlite3_temp_directory = (char*) StringView::createConstant(dir.data()).data();
    return true;
}

} // namespace WCDB
