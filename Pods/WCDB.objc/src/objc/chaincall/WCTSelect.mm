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

#import "WCTSelect.h"
#import "Assertion.hpp"
#import "WCTChainCall+Private.h"
#import "WCTHandle.h"
#import "WCTORM.h"
#import "WCTSelectable+Private.h"
#import "WCTTryDisposeGuard.h"

@implementation WCTSelect {
    WCTResultColumns _resultColumns;
    Class<WCTTableCoding> m_class;
}

- (void)willPrepare:(WCDB::StatementSelect &)statement
{
    if (statement.syntax().orderingTerms.empty()) {
        WCTAssert(m_class != nil && [m_class respondsToSelector:@selector(objectRelationalMapping)]);
        const WCTBinding &binding = [m_class objectRelationalMapping];
        const WCDB::StatementCreateTable &createTable = binding.statementTable;
        if (!createTable.syntax().withoutRowid) {
            statement.order(WCDB::OrderingTerm::ascendingRowid());
        }
    }
}

- (instancetype)fromTable:(NSString *)tableName
{
    _statement.from(tableName);
    return self;
}

- (instancetype)onResultColumns:(const WCTResultColumns &)resultColumns
{
    m_class = resultColumns.front().getColumnBinding().getClass();
    _resultColumns = resultColumns;
    _statement.select(_resultColumns);
    return self;
}

- (instancetype)ofClass:(Class)cls
{
    return [self onResultColumns:[cls allProperties]];
}

- (NSArray /* <WCTObject*> */ *)allObjects
{
    WCTTryDisposeGuard tryDisposeGuard(self);
    if (![self lazyPrepare]) {
        [self saveChangesAndError:NO];
        return nil;
    }
    NSArray *objects = [_handle allObjectsOnResultColumns:_resultColumns];
    [self saveChangesAndError:objects != nil];
    return objects;
}

- (id /* WCTObject* */)firstObject
{
    WCTTryDisposeGuard tryDisposeGuard(self);
    if (![self lazyPrepare]) {
        [self saveChangesAndError:NO];
        return nil;
    }
    BOOL succeed = false;
    if (!(succeed = [_handle step]) || [_handle done]) {
        [self saveChangesAndError:succeed];
        return nil;
    }
    [self saveChangesAndError:succeed];
    return [_handle extractObjectOnResultColumns:_resultColumns];
}

@end
