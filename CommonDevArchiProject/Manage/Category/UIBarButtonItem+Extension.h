//
//  UIBarButtonItem+Extension.h
//  DawnwingTech
//
//  Created by BANYAN on 2016/11/25.
//  Copyright © 2016年 BANYAN. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UIBarButtonItem (Extension)

+(UIBarButtonItem *)itemWithTarget:(id)target action:(SEL)action image:(NSString *)image highlightImage:(NSString *)highlightImage;
+(UIBarButtonItem *)itemWithTarget:(id)target action:(SEL)action title:(NSString *)title titleColor:(UIColor *)titleColor;

@end
