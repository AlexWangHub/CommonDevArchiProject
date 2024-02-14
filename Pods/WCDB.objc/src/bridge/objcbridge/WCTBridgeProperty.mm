//
// Created by qiuwenchen on 2022/4/21.
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

#import "WCTBridgeProperty+CPP.h"
#import <Foundation/Foundation.h>

@interface WCTBridgeProperty () {
    const WCTProperty* m_innerProperty;
}
@end

@implementation WCTBridgeProperty

+ (WCTBridgeProperty*)creatBridgeProperty:(const WCTProperty&)wctProperty
{
    WCTBridgeProperty* property = [[WCTBridgeProperty alloc] init];
    property->m_innerProperty = &wctProperty;
    return property;
}

- (NSString*)propertyName
{
    return [NSString stringWithUTF8String:m_innerProperty->getDescription().data()];
}

- (const WCTProperty&)wctProperty
{
    return *m_innerProperty;
}

- (const void* _Nullable)tableBinding
{
    return self.wctProperty.syntax().getTableBinding();
}

@end
