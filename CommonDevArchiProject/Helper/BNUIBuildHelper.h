//
//  BNUIBuildHelper.h
//  BNBitcoinIndexApp
//
//  Created by binbinwang on 2021/12/5.
//

#import <Foundation/Foundation.h>
#import "MMUIView.h"

NS_ASSUME_NONNULL_BEGIN

@interface BNUIBuildHelper : NSObject

+ (UILabel *)buildLabelWithFont:(UIFont *)font
                      textColor:(UIColor *)textColor
                     textHeight:(CGFloat)textHeight
                    defaultText:(NSString *)defaultText
                       maxWidth:(CGFloat)maxWidth
                  textAlignment:(NSTextAlignment)textAlignment;

+ (UIButton *)buildImageButtonWithFrame:(CGRect)frame
                            normalImage:(UIImage *)normalImage
                            selectImage:(UIImage *)selectImage
                           cornerRadius:(CGFloat)cornerRadius
                                 target:(NSObject *)oTarget
                                 action:(SEL)oSelector;

@end

NS_ASSUME_NONNULL_END
