//
//  NSString+Interact.h
//  BNSubscribeHelperProject
//
//  Created by blinblin on 2022/3/27.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSString (Interact)

+ (NSString *)interactCount:(NSUInteger)interactCount;

+ (NSString *)trimString:(NSString *)nsText;

@end

NS_ASSUME_NONNULL_END
