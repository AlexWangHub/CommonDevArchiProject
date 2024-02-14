//
// Created by sanhuazhang on 2019/05/26
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

#include "AutoBackupConfig.hpp"
#include "Assertion.hpp"
#include "InnerHandle.hpp"
#include "StringView.hpp"

namespace WCDB {

AutoBackupOperator::~AutoBackupOperator() = default;

AutoBackupConfig::AutoBackupConfig(const std::shared_ptr<AutoBackupOperator> &operator_)
: Config(), m_identifier(StringView::formatted("Backup-%p", this)), m_operator(operator_)
{
    WCTAssert(m_operator != nullptr);
}

AutoBackupConfig::~AutoBackupConfig() = default;

bool AutoBackupConfig::invoke(InnerHandle *handle)
{
    handle->setNotificationWhenCheckpointed(
    m_identifier,
    std::bind(&AutoBackupConfig::onCheckpointed, this, std::placeholders::_1));

    return true;
}

bool AutoBackupConfig::uninvoke(InnerHandle *handle)
{
    handle->setNotificationWhenCheckpointed(m_identifier, nullptr);

    return true;
}

void AutoBackupConfig::onCheckpointed(const UnsafeStringView &path)
{
    m_operator->asyncBackup(path);
}

} //namespace WCDB
