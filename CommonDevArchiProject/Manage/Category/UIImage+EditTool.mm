//
//  UIImage+EditTool.m
//  BNBitcoinIndexApp
//
//  Created by binbinwang on 2021/12/5.
//

#import "UIImage+EditTool.h"
#import "SVGKImage.h"

@implementation UIImage (EditTool)

- (UIImage *)resizeConstraint:(CGSize)constraintSize {
    UIImage *img = self;
    CGFloat imgSizeRatio = img.size.height / img.size.width;
    CGSize finalConstraintSize = CGSizeZero;
    if (img.size.width <= constraintSize.width && img.size.height < constraintSize.height) {
        // 宽和高都没有超过约束分辨率，直接return
        return img;
    } else if (img.size.width >= constraintSize.width && img.size.height < constraintSize.height) {
        // 宽超过了约束，高没有超过，那就约束宽的分辨率
        finalConstraintSize = CGSizeMake(constraintSize.width, constraintSize.width * imgSizeRatio);
    } else if (img.size.width <= constraintSize.width && img.size.height >= constraintSize.height) {
        // 高超过了约束，宽没有超过
        finalConstraintSize = CGSizeMake(constraintSize.height / imgSizeRatio, constraintSize.height);
    } else {
        // 宽和高都超过了约束
        // 这里优先使用配置的宽/高分辨率都差不多，考虑到视频号发横图的比较多，如果默认采用配置宽的分辨率，那么换算后的高的分辨率是一定不会超过配置值的（视频号允许发图的宽高比例范围在:16:9 ~ 3:3.5）
        // 所以这里优先使用配置的高的分辨率
        if (constraintSize.height / imgSizeRatio <= constraintSize.width) {
            finalConstraintSize = CGSizeMake(constraintSize.height / imgSizeRatio, constraintSize.height);
        } else {
            finalConstraintSize = CGSizeMake(constraintSize.width, constraintSize.width * imgSizeRatio);
        }
    }
    // 裁剪宽高取整，如果是小数会出现白边问题
    finalConstraintSize = CGSizeMake(floor(finalConstraintSize.width), floor(finalConstraintSize.height));
    UIGraphicsBeginImageContextWithOptions(finalConstraintSize, NO, 0);
    [img drawInRect:CGRectMake(0, 0, finalConstraintSize.width, finalConstraintSize.height)];
    UIImage *newImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return newImage;
}

-(UIImage*)changeColor:(UIColor*)color {
    //获取画布
    UIGraphicsBeginImageContextWithOptions(self.size, NO, 0.0f);
    //画笔沾取颜色
    [color setFill];
    
    CGRect bounds = CGRectMake(0, 0, self.size.width, self.size.height);
    UIRectFill(bounds);
    //绘制一次
    [self drawInRect:bounds blendMode:kCGBlendModeOverlay alpha:1.0f];
    //再绘制一次
    [self drawInRect:bounds blendMode:kCGBlendModeDestinationIn alpha:1.0f];
    //获取图片
    UIImage *img = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return img;
}

+ (UIImage *)svgImageNamed:(NSString *)name size:(CGSize)size tintColor:(UIColor *)tintColor {
    SVGKImage *svgImage = [SVGKImage imageNamed:name];
    svgImage.size = size;
    CGRect rect = CGRectMake(0, 0, svgImage.size.width, svgImage.size.height);
    CGImageAlphaInfo alphaInfo = CGImageGetAlphaInfo(svgImage.UIImage.CGImage);
    BOOL opaque = alphaInfo == kCGImageAlphaNoneSkipLast
    || alphaInfo == kCGImageAlphaNoneSkipFirst
    || alphaInfo == kCGImageAlphaNone;
    UIGraphicsBeginImageContextWithOptions(svgImage.size, opaque, svgImage.scale);
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextTranslateCTM(context, 0, svgImage.size.height);
    CGContextScaleCTM(context, 1.0, -1.0);
    CGContextSetBlendMode(context, kCGBlendModeNormal);
    CGContextClipToMask(context, rect, svgImage.UIImage.CGImage);
    CGContextSetFillColorWithColor(context, tintColor.CGColor);
    CGContextFillRect(context, rect);
    UIImage *imageOut = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return imageOut;
}

@end
