//
//  BNUIBuildHelper.m
//  BNBitcoinIndexApp
//
//  Created by binbinwang on 2021/12/5.
//

#import "BNUIBuildHelper.h"

@implementation BNUIBuildHelper

+ (UILabel *)buildLabelWithFont:(UIFont *)font
                      textColor:(UIColor *)textColor
                     textHeight:(CGFloat)textHeight
                    defaultText:(NSString *)defaultText
                       maxWidth:(CGFloat)maxWidth
                  textAlignment:(NSTextAlignment)textAlignment {
    UILabel *label = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 0, textHeight)];
    label.font = font;
    label.textColor = textColor;
    label.textAlignment = textAlignment;
    if (defaultText.length > 0) {
        [label setText:defaultText];
        [label sizeToFit];
        label.width = (label.width > maxWidth) ? maxWidth : label.width;
        label.height = textHeight;
    }
    return label;
}

+ (UIButton *)buildImageButtonWithFrame:(CGRect)frame
                            normalImage:(UIImage *)normalImage
                            selectImage:(UIImage *)selectImage
                           cornerRadius:(CGFloat)cornerRadius
                                 target:(NSObject *)oTarget
                                 action:(SEL)oSelector {
    UIButton *button = [[UIButton alloc] initWithFrame:frame];
    if (normalImage) {
        [button setImage:normalImage forState:UIControlStateNormal];
    }
    
    if (selectImage) {
        [button setImage:selectImage forState:UIControlStateSelected];
    }
    
    if (cornerRadius > 0) {
        button.layer.masksToBounds = YES;
        button.layer.cornerRadius = cornerRadius;
    }
    [button addTarget:oTarget action:oSelector forControlEvents:UIControlEventTouchUpInside];
    return button;
}

@end
