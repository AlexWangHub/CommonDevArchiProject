//
//  NSString+Interact.m
//  BNSubscribeHelperProject
//
//  Created by blinblin on 2022/3/27.
//

#import "NSString+Interact.h"

@implementation NSString (Interact)

+ (NSString *)interactCount:(NSUInteger)interactCount {
    NSString *resultStr = nil;
        if (interactCount <= 9999) {
            resultStr = [NSString stringWithFormat:@"%ld", interactCount];
        } else if (interactCount < 100000) { //小于1亿
            NSString *numberResult = [self legalFormatWithOriginalValue:interactCount divider:10000];
            resultStr = [NSString stringWithFormat:@"%@%@", numberResult, @"万"];
        } else {
            resultStr = [NSString stringWithFormat:@"10%@+", @"万"];
        }

    return resultStr;
}

+ (NSString *)legalFormatWithOriginalValue:(NSUInteger)originalValue divider:(NSUInteger)divider {
    return [NSString legalFormatWithOriginalValue:originalValue divider:divider decimalPlaces:1];
}

//decimalPlaces 保留几位小数
+ (NSString *)legalFormatWithOriginalValue:(NSUInteger)originalValue divider:(NSUInteger)divider decimalPlaces:(NSUInteger)decimalPlaces {
    NSUInteger resultValue = originalValue / divider;

    float value = (originalValue - resultValue * divider) * 1.0 / divider;
    NSUInteger floatValue = value * pow(10, decimalPlaces);

    NSString *resultString = [NSString stringWithFormat:@"%lu", resultValue];
    if (floatValue > 0) {
        resultString = [resultString stringByAppendingFormat:@".%lu", (unsigned long)floatValue];
    }

    return resultString;
}

+ (NSString *)trimString:(NSString *)nsText {
    NSString *resultString = [nsText stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    return resultString == nil ? @"" : resultString;
}

@end
