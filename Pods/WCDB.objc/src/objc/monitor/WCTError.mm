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

#import "WCTError.h"
#import "CoreConst.h"
#import "WCTConvertible.h"
#import "WCTDatabase+Monitor.h"
#import "WCTError+Private.h"
#import "WCTFoundation.h"

NSErrorDomain const WCTErrorDomain = @"WCDB";

NSErrorUserInfoKey const WCTErrorKeyPath = @WCDB_ERROR_STRING_KEY_PATH;
NSErrorUserInfoKey const WCTErrorKeySQL = @WCDB_ERROR_STRING_KEY_SQL;
NSErrorUserInfoKey const WCTErrorKeyTag = @WCDB_ERROR_INT_KEY_TAG;
NSErrorUserInfoKey const WCTErrorKeyExtendedCode = @WCDB_ERROR_INT_KEY_EXTCODE;

@implementation WCTError

- (instancetype)initWithCode:(WCTErrorCode)code
                       level:(WCTErrorLevel)level
                     message:(NSString *)message
                    userInfo:(NSDictionary<NSErrorUserInfoKey, id> *)userInfo
{
    if (self = [super initWithDomain:WCTErrorDomain
                                code:(NSInteger) code
                            userInfo:userInfo]) {
        _message = message;
        _level = (WCTErrorLevel) level;
    }
    return self;
}

- (instancetype)initWithError:(const WCDB::Error &)error
{
    NSMutableDictionary *userInfo = [[NSMutableDictionary alloc] init];
    if (error.infos.size() > 0) {
        for (const auto &info : error.infos) {
            switch (info.second.getType()) {
            case WCDB::Value::Type::Text:
                [userInfo setObject:[NSString stringWithView:info.second.textValue()]
                             forKey:[NSString stringWithView:info.first]];
                break;
            case WCDB::Value::Type::Float:
                [userInfo setObject:[NSNumber numberWithDouble:info.second.floatValue()] forKey:[NSString stringWithView:info.first]];
                break;
            case WCDB::Value::Type::Integer:
                [userInfo setObject:[NSNumber numberWithLongLong:info.second.intValue()] forKey:[NSString stringWithView:info.first]];
                break;
            case WCDB::Value::Type::BLOB:
                [userInfo setObject:[NSData dataWithBytes:info.second.blobValue().buffer() length:info.second.blobValue().size()] forKey:[NSString stringWithView:info.first]];
                break;
            default:
                [userInfo setObject:[NSNull null] forKey:[NSString stringWithView:info.first]];
                break;
            }
        }
    }

    return [self initWithCode:(WCTErrorCode) error.code() level:(WCTErrorLevel) error.level message:[NSString stringWithView:error.getMessage()] userInfo:userInfo];
}

- (BOOL)isOK
{
    return self.code == WCTErrorCodeOK;
}

- (BOOL)isCorruption
{
    return self.code == WCTErrorCodeCorrupt || self.code == WCTErrorCodeNotADatabase;
}

- (NSString *)stringForKey:(NSErrorUserInfoKey)key
{
    id value = [self.userInfo objectForKey:key];
    if (value != nil && [value isKindOfClass:NSString.class]) {
        return value;
    }
    return nil;
}

- (NSNumber *)numberForKey:(NSErrorUserInfoKey)key
{
    id value = [self.userInfo objectForKey:key];
    if (value != nil && [value isKindOfClass:NSNumber.class]) {
        return value;
    }
    return nil;
}

- (NSString *)description
{
    NSMutableString *description = nil;
    if (self.code != WCTErrorCodeOK
        && self.code != WCTErrorCodeRow
        && self.code != WCTErrorCodeDone) {
        description = [[NSMutableString alloc] initWithFormat:@"[%s: %ld, %@]", WCDB::Error::levelName((WCDB::Error::Level) self.level), (long) self.code, self.message];
        [self.userInfo enumerateKeysAndObjectsUsingBlock:^(NSErrorUserInfoKey key, id value, BOOL *) {
            if ([value isKindOfClass:NSNull.class]) {
                [description appendFormat:@", %@", key];
            } else {
                [description appendFormat:@", %@: %@", key, value];
            }
        }];
    }
    return description;
}

@end

@implementation WCTError (Path)

- (NSString *)path
{
    return [self stringForKey:WCTErrorKeyPath];
}

@end

@implementation WCTError (Tag)

- (WCTTag)tag
{
    static_assert(WCTInvalidTag == 0, "");
    return (WCTTag) [self numberForKey:WCTErrorKeyTag].intValue;
}

@end

@implementation WCTError (SQL)

- (NSString *)sql
{
    return [self stringForKey:WCTErrorKeySQL];
}

@end

@implementation WCTError (ExtendedCode)

- (WCTErrorExtendedCode)extendedCode
{
    return (WCTErrorExtendedCode) [self numberForKey:WCTErrorKeyExtendedCode].intValue;
}

@end
