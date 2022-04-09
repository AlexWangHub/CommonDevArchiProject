//
//  UIImage+EditTool.h
//  BNBitcoinIndexApp
//
//  Created by binbinwang on 2021/12/5.
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface UIImage (EditTool)

- (UIImage *)resizeConstraint:(CGSize)constraintSize;
-(UIImage*)changeColor:(UIColor*)color;

+ (UIImage *)svgImageNamed:(NSString *)name size:(CGSize)size tintColor:(UIColor *)tintColor;

@end

NS_ASSUME_NONNULL_END
