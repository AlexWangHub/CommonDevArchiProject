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

#import "WCTDelete.h"
#import "Assertion.hpp"
#import "WCTChainCall+Private.h"
#import "WCTHandle+Private.h"
#import "WCTHandle.h"
#import "WCTTryDisposeGuard.h"

@implementation WCTDelete {
    WCDB::StatementDelete _statement;
}

- (instancetype)initWithHandle:(WCTHandle *)handle
{
    if (self = [super initWithHandle:handle]) {
        [handle setWriteHint:YES];
    }
    return self;
}

- (WCDB::StatementDelete &)statement
{
    return _statement;
}

- (instancetype)fromTable:(NSString *)tableName
{
    _statement.deleteFrom(tableName);
    return self;
}

- (instancetype)where:(const WCDB::Expression &)condition
{
    _statement.where(condition);
    return self;
}

- (instancetype)orders:(const WCDB::OrderingTerms &)orders
{
    _statement.orders(orders);
    return self;
}

- (instancetype)limit:(const WCDB::Expression &)limit
{
    _statement.limit(limit);
    return self;
}

- (instancetype)offset:(const WCDB::Expression &)offset
{
    _statement.offset(offset);
    return self;
}

- (BOOL)execute
{
    WCTTryDisposeGuard tryDisposeGuard(self);
    bool succeed = [_handle execute:_statement];
    [self saveChangesAndError:succeed];
    return succeed;
}

@end
