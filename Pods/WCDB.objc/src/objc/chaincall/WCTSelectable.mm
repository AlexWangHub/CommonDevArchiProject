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

#import "WCTChainCall+Private.h"
#import "WCTHandle+Private.h"
#import "WCTHandle.h"
#import "WCTSelectable+Private.h"

@implementation WCTSelectable

- (instancetype)initWithHandle:(WCTHandle *)handle
{
    if (self = [super initWithHandle:handle]) {
        [handle setWriteHint:NO];
    }
    return self;
}

- (WCDB::StatementSelect &)statement
{
    return _statement;
}

- (BOOL)lazyPrepare
{
    if (![_handle isPrepared]) {
        [self willPrepare:_statement];
        return [_handle prepare:_statement];
    }
    return YES;
}

- (void)willPrepare:(WCDB::StatementSelect &)statement
{
    WCDB_UNUSED(statement);
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

@end
