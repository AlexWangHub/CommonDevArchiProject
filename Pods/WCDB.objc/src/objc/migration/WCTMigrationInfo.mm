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

#import "WCTMigrationInfo.h"
#import "WCTFoundation.h"
#import "WCTMigrationInfo+Private.h"

@implementation WCTMigrationBaseInfo

- (instancetype)initWithBaseInfo:(const WCDB::MigrationBaseInfo &)info
{
    if (self = [super init]) {
        _table = [NSString stringWithView:info.getTable()];
        _database = [NSString stringWithView:info.getDatabase()];
        _sourceTable = [NSString stringWithView:info.getSourceTable()];
        _sourceDatabase = [NSString stringWithView:info.getSourceDatabase()];
    }
    return self;
}

@end

@implementation WCTMigrationUserInfo

- (void)setSourceTable:(NSString *)table
{
    _sourceTable = table;
}

- (void)setSourceDatabase:(NSString *)database
{
    _sourceDatabase = [database wcdb_stringByStandardizingPath];
}

@end
