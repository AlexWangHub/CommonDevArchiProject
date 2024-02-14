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

#import "Assertion.hpp"
#import "WCTDatabase+Handle.h"
#import "WCTDatabase+Private.h"
#import "WCTDatabase+Transaction.h"
#import "WCTError+Private.h"
#import "WCTHandle+Private.h"
#import "WCTHandle+Table.h"
#import "WCTORM.h"
#import "WCTTable+Private.h"

@implementation WCTDatabase (Table)

- (WCTOptionalBool)tableExists:(NSString *)tableName
{
    return _database->tableExists(tableName);
}

- (BOOL)createTable:(NSString *)tableName
          withClass:(Class<WCTTableCoding>)cls
{
    WCTRemedialAssert(tableName != nil && cls != nil, "Class or table name can't be null.", return NO;);
    WCTHandle *handle = [self getHandle];
    [handle setWriteHint:YES];
    BOOL result = [handle remapTable:tableName toClass:cls];
    [handle invalidate];
    return result;
}

- (WCTTable *)getTable:(NSString *)tableName
             withClass:(Class<WCTTableCoding>)tableClass
{
    WCTRemedialAssert(tableName != nil && tableClass != nil, "Class or table name can't be null.", return nil;);
    return [[WCTTable alloc] initWithDatabase:self tableName:tableName tableClass:tableClass];
}

- (BOOL)createVirtualTable:(NSString *)tableName
                 withClass:(Class<WCTTableCoding>)tableClass
{
    WCTRemedialAssert(tableName != nil && tableClass != nil, "Class or table name can't be null.", return NO;);
    WCTHandle *handle = [self getHandle];
    [handle setWriteHint:YES];
    BOOL result = [handle createVirtualTable:tableName withClass:tableClass];
    [handle invalidate];
    return result;
}

- (BOOL)dropTable:(NSString *)tableName
{
    return _database->execute(WCDB::StatementDropTable().dropTable(tableName).ifExists());
}

- (BOOL)dropIndex:(NSString *)indexName
{
    return _database->execute(WCDB::StatementDropIndex().dropIndex(indexName).ifExists());
}

@end
