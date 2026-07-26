#ifndef _OBJBASE_H
#define _OBJBASE_H

/* A root class for the Objective-C test cases.
 *
 * These tests were written against the Object class that libobjc used to
 * supply, and instantiate their machines with [[Foo alloc] init]. Since gcc
 * 4.x, objc/Object.h has been cut down to nothing but -class and -isEqual:,
 * with a comment directing users to a Foundation library for a real root
 * class. Rather than take a dependency on GNUstep for six test cases, provide
 * the two methods they actually need.
 *
 * The implementation lives in the header because runtests compiles each test
 * case as a single translation unit. */

#include <objc/Object.h>
#include <objc/runtime.h>

@interface TestObject : Object
+ (id) alloc;
- (id) init;
@end

@implementation TestObject
+ (id) alloc { return class_createInstance( self, 0 ); }
- (id) init { return self; }
@end

#endif
