//
//  UIView+Extend.m
//  BNBitcoinIndexApp
//
//  Created by binbinwang on 2021/12/5.
//

#import "UIView+Extend.h"
#import "BNMainSubCardView.h"

@implementation UIView (Extend)

- (void)removeAllSubViews {
    for (UIView *subView in self.subviews) {
        [subView removeFromSuperview];
    }
}

- (void)removeSubViewWithTag:(UInt32)uiTag {
    UIView *subView = [self viewWithTag:uiTag];
    while (subView) {
        [subView removeFromSuperview];
        subView = [self viewWithTag:uiTag];
    }
}

- (void)removeSubViewWithClass:(Class)oClass {
    for (UIView *subView in self.subviews) {
        if ([subView isKindOfClass:oClass]) {
            [subView removeFromSuperview];
        }
    }
}

- (UIView *)viewWithClass:(Class)oClass {
    for (UIView *subView in self.subviews) {
        if ([subView isKindOfClass:oClass]) {
            return subView;
        } else {
            UIView *target = [subView viewWithClass:oClass];
            if (target) {
                return target;
            }
        }
    }
    return nil;
}

- (void)setWcOrigin:(CGPoint)aPoint {
    if (isnan(aPoint.x) || isnan(aPoint.y)) {
        return;
    }

    CGRect newframe = self.frame;
    newframe.origin = aPoint;
    self.frame = newframe;
}

- (CGPoint)wcOrigin {
    return self.frame.origin;
}

// Retrieve and set the size
- (CGSize)size {
    return self.frame.size;
}

- (void)setSize:(CGSize)aSize {
    if (isnan(aSize.width) || isnan(aSize.height)) {
        return;
    }

    CGRect newframe = self.frame;
    newframe.size = aSize;
    self.frame = newframe;
}

// Query other frame locations
- (CGPoint)bottomRight {
    CGFloat x = self.frame.origin.x + self.frame.size.width;
    CGFloat y = self.frame.origin.y + self.frame.size.height;
    return CGPointMake(x, y);
}

- (CGPoint)bottomLeft {
    CGFloat x = self.frame.origin.x;
    CGFloat y = self.frame.origin.y + self.frame.size.height;
    return CGPointMake(x, y);
}

- (CGPoint)topRight {
    CGFloat x = self.frame.origin.x + self.frame.size.width;
    CGFloat y = self.frame.origin.y;
    return CGPointMake(x, y);
}

// Retrieve and set height, width, top, bottom, left, right
- (CGFloat)x {
    return self.frame.origin.x;
}

- (void)setX:(CGFloat)x {
    if (isnan(x)) {
        return;
    }

    CGRect newframe = self.frame;
    newframe.origin.x = x;
    self.frame = newframe;
}

- (CGFloat)y {
    return self.frame.origin.y;
}

- (void)setY:(CGFloat)y {
    if (isnan(y)) {
        return;
    }

    CGRect newframe = self.frame;
    newframe.origin.y = y;
    self.frame = newframe;
}

- (CGFloat)centerX {
    return self.center.x;
}

- (void)setCenterX:(CGFloat)centerX {
    if (isnan(centerX)) {
        return;
    }

    self.center = CGPointMake(centerX, self.center.y);
}

- (CGFloat)centerY {
    return self.center.y;
}

- (void)setCenterY:(CGFloat)centerY {
    if (isnan(centerY)) {
        return;
    }

    self.center = CGPointMake(self.center.x, centerY);
}

- (CGFloat)height {
    return self.frame.size.height;
}

- (void)setHeight:(CGFloat)newheight {
    if (isnan(newheight)) {
        return;
    }

    CGRect newframe = self.frame;
    newframe.size.height = newheight;
    self.frame = newframe;
}

- (CGFloat)width {
    return self.frame.size.width;
}

- (void)setWidth:(CGFloat)newwidth {
    if (isnan(newwidth)) {
        return;
    }

    CGRect newframe = self.frame;
    newframe.size.width = newwidth;
    self.frame = newframe;
}

- (CGFloat)top {
    return self.frame.origin.y;
}

- (void)setTop:(CGFloat)newtop {
    if (isnan(newtop)) {
        return;
    }

    CGRect newframe = self.frame;
    newframe.origin.y = newtop;
    self.frame = newframe;
}

- (CGFloat)left {
    return self.frame.origin.x;
}

- (void)setLeft:(CGFloat)newleft {
    if (isnan(newleft)) {
        return;
    }

    CGRect newframe = self.frame;
    newframe.origin.x = newleft;
    self.frame = newframe;
}

- (CGFloat)bottom {
    return self.frame.origin.y + self.frame.size.height;
}

- (void)setBottom:(CGFloat)newbottom {
    if (isnan(newbottom)) {
        return;
    }

    CGRect newframe = self.frame;
    newframe.origin.y = newbottom - self.frame.size.height;
    self.frame = newframe;
}

- (CGFloat)right {
    return self.frame.origin.x + self.frame.size.width;
}

- (void)setRight:(CGFloat)newright {
    if (isnan(newright)) {
        return;
    }

    CGFloat delta = newright - (self.frame.origin.x + self.frame.size.width);
    CGRect newframe = self.frame;
    newframe.origin.x += delta;
    self.frame = newframe;
}

- (UIEdgeInsets)realSafeAreaInsets {
    return self.safeAreaInsets;
}

- (void)setX:(CGFloat)x andY:(CGFloat)y {
    CGRect f = self.frame;
    self.frame = CGRectMake(x, y, f.size.width, f.size.height);
}

// Move via offset
- (void)moveBy:(CGPoint)delta {
    CGPoint newcenter = self.center;
    newcenter.x += delta.x;
    newcenter.y += delta.y;
    self.center = newcenter;
}

// Scaling
- (void)scaleBy:(CGFloat)scaleFactor {
    CGRect newframe = self.frame;
    newframe.size.width *= scaleFactor;
    newframe.size.height *= scaleFactor;
    self.frame = newframe;
}

// Ensure that both dimensions fit within the given size by scaling down
- (void)fitInSize:(CGSize)aSize {
    CGFloat scale;
    CGRect newframe = self.frame;

    if (newframe.size.height && (newframe.size.height > aSize.height)) {
        scale = aSize.height / newframe.size.height;
        newframe.size.width *= scale;
        newframe.size.height *= scale;
    }

    if (newframe.size.width && (newframe.size.width >= aSize.width)) {
        scale = aSize.width / newframe.size.width;
        newframe.size.width *= scale;
        newframe.size.height *= scale;
    }

    self.frame = newframe;
}

- (void)fitTheSubviews {
    CGFloat fWidth = 0;
    CGFloat fHeight = 0;
    for (UIView *subview in self.subviews) {
        fWidth = MAX(fWidth, subview.right);
        fHeight = MAX(fHeight, subview.bottom);
    }
    self.size = CGSizeMake(fWidth, fHeight);
}

- (void)ceilAllSubviews {
    for (UIView *subView in self.subviews) {
        subView.frame = CGRectMake(ceilf(subView.left), ceilf(subView.top), ceilf(subView.width), ceilf(subView.height));
    }
}

- (void)frameIntegral {
    self.frame = CGRectIntegral(self.frame);
}

- (UIView *)filterBottomView {
    __block UIView *bottomView = self.subviews.firstObject;
    [self.subviews enumerateObjectsUsingBlock:^(__kindof UIView * _Nonnull obj, NSUInteger idx, BOOL * _Nonnull stop) {
        if ([obj isKindOfClass:[BNMainSubCardView class]]) {
            bottomView = (obj.bottom > bottomView.bottom) ? obj : bottomView;
        }
    }];
    return bottomView;
}

- (void)removeAllGestureRecognizer {
    if (![self respondsToSelector:@selector(gestureRecognizers)])
        return;

    for (UIGestureRecognizer *gestureRecognizer in self.gestureRecognizers) {
        [self removeGestureRecognizer:gestureRecognizer];
    }
}

@end
